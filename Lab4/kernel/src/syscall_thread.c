/** @file   syscall_thread.c
 *
 *  @brief  Function definitions of threading syscalls for creating, scheduling, killing threads, etc..
 *
 *  @date   4/29/21
 *
 *  @author Arden Diakhate-Palme, Neville Chima
 */

#include <stdint.h>
#include <nvic.h>
#include <arm.h>
#include <printk.h>
#include <timer.h>
#include "syscall_thread.h"
#include "syscall_mutex.h"
#include "mpu.h"
#include "syscall.h"

/** @brief Initial XPSR value, all 0s except thumb bit. */
#define XPSR_INIT 0x1000000

/** @brief index of main thread */
#define MAIN_THREAD_IDX 0
/** @brief index of idle thread */
#define IDLE_THREAD_IDX 1
/** @brief index of the first user thread */
#define USER_THREAD_FIRST_IDX 2

/**
 * @brief      Heap high and low pointers.
 */
//@{
extern char 
__thread_u_stacks_low,
__thread_u_stacks_top,
__thread_k_stacks_low,
__thread_k_stacks_top;
//@}


/**
 * @brief  Stack frame upon exception.
 */
typedef struct {
    uint32_t r0;   /**< Register value for r0 */
    uint32_t r1;   /**< Register value for r1 */
    uint32_t r2;   /**< Register value for r2 */
    uint32_t r3;   /**< Register value for r3 */
    uint32_t r12;  /**< Register value for r12 */
    uint32_t lr;   /**< Register value for lr*/
    uint32_t pc;   /**< Register value for pc */
    uint32_t xPSR; /**< Register value for xPSR */
} interrupt_stack_frame;

/**
 * @brief Calle context saved stack frame
 */
typedef struct {
  uint32_t lr;  /**< Register value for lr*/
  uint32_t psp; /**< Register value for psp*/
  uint32_t r11; /**< Register value for r11 */
  uint32_t r10; /**< Register value for r10 */
  uint32_t r9; /**< Register value for r9 */
  uint32_t r8; /**< Register value for r8 */
  uint32_t r7; /**< Register value for r7 */
  uint32_t r6; /**< Register value for r6 */
  uint32_t r5; /**< Register value for r5 */
  uint32_t r4; /**< Register value for r4 */
} callee_stack_frame;

/**
 * @brief  thread-specific kernel datat structures for thread management
 */
typedef struct {
    uint8_t  id;        /**< thread's identifier */    
    thread_state state; /**< thread's current state i.e running or runnable etc*/
    uint32_t static_prio; /**< thread's static priority */
    uint32_t dyn_prio; /**< thread's dynamic priority */
    uint32_t T;    /**< in ticks*/
    uint32_t C;    /**< in ticks*/
    uint32_t tickStart;    /**< in ticks*/
    uint32_t u_stack_high; /**< first address in thread's psp */
    uint32_t k_stack_high; /**< first address in thread's msp */
    uint32_t running_C; /**< computation time thus far in current period*/
    uint32_t last_deadline; /**< last wakeup time for thread */
    uint32_t next_deadline; /**< next wakeup time for thread */
    uint32_t total_C; /**< total computation time since thread initialized*/
    void *psp; /**< address of psp */
    void *msp; /**< address of msp */
    int  svc_status; /**< whether thread was servicing an SVC */
} tcb_t;

/**
 * @brief global kernel data structures for thread management
 */
typedef struct gcb_t {
  uint32_t tick_count; /**< global system clock*/
  uint32_t stack_size; /**< lower limit of stack space per thread*/
  uint32_t max_threads; /**< total number of initializable threads*/
  uint8_t next; /**< tracks number of user threads initialized thus far*/
  uint8_t num_inactive; /**< tracks number of inactive/killed threads */
  uint32_t u_stack_next; /**< pointer to address starting next user stack*/
  uint32_t k_stack_next; /**< pointer to address starting next kernel stack*/
  uint8_t active_id; /**< source of truth for currently running thread */
  tcb_t tcbs[16]; /**< thread control blocks */
  uint32_t num_mutexes; /**< num initialized system mutexes */
  uint32_t max_mutexes; /**< max initializable system mutex */
  kmutex_t mutexes[32]; /**< system mutextes */
} gcb_t;


/** @brief global for memory fault detection and handling */
int mem_fault= 0;

/**
 * @brief      Precalculated values for UB test.
 */
float ub_table[] = {
  0.000, 1.000, .8284, .7798, .7568,
  .7435, .7348, .7286, .7241, .7205,
  .7177, .7155, .7136, .7119, .7106,
  .7094, .7083, .7075, .7066, .7059,
  .7052, .7047, .7042, .7037, .7033,
  .7028, .7025, .7021, .7018, .7015,
  .7012, .7009
};

/** Intialize shared kernel data structure globally */
gcb_t gcb;

/** @brief finds currently executing thread */
tcb_t *get_active_thread();

/** @brief thread scheduler */
tcb_t *get_next_thread();

/** @brief whether the available stack space is enough for thread stacks */
int stack_overflows(uint32_t max_threads, uint32_t stack_size);

/** @brief helper to setup new threads expected stack frame on pendSV interrupt */
void setup_init_stack_frame(tcb_t *thread, void *fn, void *vargp);

/** @brief initializes default and idle threads*/
void set_default_threads(void *idle_fn);

/** @brief indicates whether a task set is schedulable */
int is_schedulable(uint32_t newC, uint32_t newT);

/** @brief updates thread times as global clock changes */
void update_thread_times();

/** @brief switches memory protectino between threads*/
void switch_mem_protect(tcb_t *next_thread);

/** @brief Reference to assembly-defined global function for linker resolution */
extern void thread_kill( void );

/** @brief find any one of the inactive threads */
tcb_t *find_inactive_thread();

/** @brief sets thread is (not) pending to use mutex */
void set_pending_state(kmutex_t *mutex, uint32_t thread_id, uint32_t val);

/** @brief current state of whether thread is pending to use mutex */
int get_pending_state(kmutex_t *mutex, uint32_t thread_id);

/** @brief returns whether another mutex with a priorty <= thread_prio is locked by a different thread */
int locked_geq_prio_mutex(tcb_t *thread);

/** @brief returns next highest dynamic priority after a thread has unlocked mutex */
int get_fallback_prio(tcb_t *thread);

/** @brief returns current priority of a thread */
uint32_t get_curr_prio(uint32_t thread_id);

uint32_t is_using_mutex(uint32_t thread_id);


/**
 * @brief  called when systick counter reaches 0, runs the scheduler, updates thread tick counts
 */
void systick_c_handler(){
  gcb.tick_count++;
  update_thread_times();
  pend_pendsv();
}

/**
 * @brief  contentext swaps between threads and runs the scheduler
 *
 * @param  curr_msp     the current main stack pointer, which is passed in the asm handler
 */
void *pendsv_c_handler(void *curr_msp){ 
    clear_pendsv();


    void *ret_msp = curr_msp;

    /*Use active_id variable instead of thread state 
    as handler is called from various contexts */
    tcb_t *last_thread = &gcb.tcbs[gcb.active_id];

    last_thread->msp = curr_msp;
    
    //Store SVC status to restore later
    int svc_status = get_svc_status();
    if (svc_status){
      last_thread->svc_status = 1;
    } else {
      last_thread->svc_status = 0;
    }

   tcb_t *next_thread = get_next_thread();
    if (next_thread == NULL){
      //All threads have finished executing, return to main
      if (gcb.num_inactive > 0 && gcb.num_inactive == gcb.next - USER_THREAD_FIRST_IDX){
        next_thread = &gcb.tcbs[MAIN_THREAD_IDX];
        timer_disable();
      } else {
        //Schedule idle thread while others are asleep
        next_thread = &gcb.tcbs[IDLE_THREAD_IDX];
      }
    } 

    if(mem_fault) sys_thread_kill();

    if(next_thread->id != last_thread->id && !mem_fault)
        switch_mem_protect(next_thread);

    ret_msp = next_thread->msp;
    gcb.active_id = next_thread->id;

    //Caller function may have/not changed curr thread state
    if (last_thread->state == RUNNING) last_thread->state = RUNNABLE;
    next_thread->state = RUNNING;

    set_svc_status(next_thread->svc_status);
    // printk("Old thread was %d, new thread is %d, arr size is %d\n", last_thread->id, next_thread->id, gcb.next);


    return ret_msp;
}


int sys_thread_init(uint32_t max_threads, uint32_t stack_size, void *idle_fn, 
protection_mode memory_protection, uint32_t max_mutexes){

  if (stack_overflows(max_threads, stack_size)) return -1;

  /** set all threads to inactive, and set IDs*/
  gcb.max_threads = max_threads;
  gcb.stack_size = 1 << mm_log2ceil_size(stack_size);
  gcb.tick_count = 0;
  gcb.next = 0;
  gcb.num_mutexes = 0;
  gcb.max_mutexes = max_mutexes;
  gcb.active_id = MAIN_THREAD_IDX;
  gcb.num_inactive = 0;
  gcb.u_stack_next = (uint32_t)&__thread_u_stacks_low;
  gcb.k_stack_next = (uint32_t)&__thread_k_stacks_low;
  set_default_threads(idle_fn);

  if(memory_protection == PER_THREAD) mm_enable();
  else mm_disable();

  return 0;
}


/**
 * @brief  Enables a memory protection region. Regions must be aligned!
 *
 * @param  fn           pointer to the thread function
 * @param  prio         the thread's priority
 * @param  C            the thread's worst-case runtime complexity in ticks
 * @param  T            the thread's period 
 * @param  vargp        pointer to the arguments with which the thread function should be run
 *                            0 otherwise.
 * @return 0 on success, -1 on failure
 */
int sys_thread_create(void *fn, uint32_t prio, uint32_t C, uint32_t T, void *vargp){
    if (!is_schedulable(C, T)) return -1;
    if ((int)gcb.next - USER_THREAD_FIRST_IDX - (int)gcb.num_inactive + 1 > (int)gcb.max_threads) return -1;
    
    tcb_t *new_thread;

    //Utilize unitialized thread if any
    if ((int)gcb.next - USER_THREAD_FIRST_IDX < (int)gcb.max_threads){
      new_thread = &gcb.tcbs[gcb.next];
      new_thread->id = gcb.next++;
      
      //MSP and PSP stacks setup
      gcb.k_stack_next += gcb.stack_size * 4;
      new_thread->msp = (void*)gcb.k_stack_next;
      new_thread->k_stack_high = gcb.k_stack_next;

      gcb.u_stack_next += gcb.stack_size * 4;
      new_thread->psp = (void*)gcb.u_stack_next;
      new_thread->u_stack_high = gcb.u_stack_next;
    } else {
      //Utilize deactivated thread data structure and maintain some properties
      new_thread = find_inactive_thread();
      gcb.num_inactive--;
      new_thread->msp = (void *)new_thread->k_stack_high;
      new_thread->psp = (void *)new_thread->u_stack_high;
    }
      
    new_thread->C = C;
    new_thread->T = T;
    new_thread->running_C = 0;
    new_thread->total_C = 0;
    new_thread->last_deadline = gcb.tick_count;
    new_thread->next_deadline = gcb.tick_count + T;
    new_thread->static_prio = prio;
    new_thread->dyn_prio = new_thread->static_prio;
    new_thread->state = RUNNABLE;
    new_thread->svc_status = 0;

    setup_init_stack_frame(new_thread, fn, vargp);

    return 0;
}

/**
 * @brief  syscall to get the running thread's current effective priority 
 * @param  frequency the frequency in Hz with which the scheduler should be run
 * @return 0 on success -1 on error
*/
int sys_scheduler_start(uint32_t frequency){
    uint32_t systickDiv= 16000000/frequency;
    timer_start(systickDiv);
	pend_pendsv();
	return 0; 
}

/**
 * @brief  syscall to get the running thread's current effective priority 
 * @return the running thread's priority
*/
uint32_t sys_get_priority(){
  return get_curr_prio(gcb.active_id);
}


/**
 * @brief  syscall to get the current time since the scheduler started
 * @return the time passed since the scheduler started (unit of ticks)
*/
uint32_t sys_get_time(){
    return gcb.tick_count;
}

/**
 * @brief  syscall to get the current time since the thread initially ran in ticks
 * @return the time since the thread initially ran
*/
uint32_t sys_thread_time(){
  return gcb.tcbs[gcb.active_id].total_C;
}

/**
 * @brief  syscall to kill the currently thread 
*/
void sys_thread_kill(){
  tcb_t *curr_thread= &gcb.tcbs[gcb.active_id];
  if(curr_thread->id == MAIN_THREAD_IDX){
      sys_exit(1);
      return;
  }

  if(curr_thread->id == IDLE_THREAD_IDX){
      breakpoint();
      return;
  }

  //if a memory fault occurs un-set it
  if(mem_fault) mem_fault=0;

  curr_thread->state= INACTIVE;
  gcb.num_inactive++;
  pend_pendsv();
}

/**
 * @brief  waits until the next period of the system clock
*/
void sys_wait_until_next_period(){
  //No op for idle thread
 tcb_t *curr_thread = &gcb.tcbs[gcb.active_id];
  if (curr_thread->id <= IDLE_THREAD_IDX ) return;
  
  //Active thread yields remaining computation time
  if (curr_thread->state == RUNNING) {
    curr_thread->state = WAITING;
    pend_pendsv();
  }

}

/**
 * @brief  initialize a mutex at a given priority ceiling
 * @param  max_prio max priority at which to init the  mutex
 * @return the mutex structure of the just init'd mutex
*/
kmutex_t *sys_mutex_init( uint32_t max_prio ) {
  if (gcb.num_mutexes + 1 > gcb.max_mutexes) return NULL;

  kmutex_t *mutex = &gcb.mutexes[gcb.num_mutexes];
  mutex->id =  gcb.num_mutexes++;
  mutex->prio_ceil = max_prio;
  mutex->locked_by =  -1;
  mutex->pending = 0;
  return mutex;
}

/**
 * @brief  locks a resource, i.e a mutex 
 * @param  mutex    the mutex to lock
*/
void sys_mutex_lock( kmutex_t *mutex ) {
  tcb_t *curr_thread = &gcb.tcbs[gcb.active_id];
  if (curr_thread->id == IDLE_THREAD_IDX) return;
  
  //Abort thread if it dishonors mutex priority
  if (curr_thread->static_prio < mutex->prio_ceil){
    curr_thread->state = INACTIVE;
    gcb.num_inactive++;
    printk("Error: Thread cannot lock mutex %d \n", mutex->id);
    pend_pendsv();
    return;
  }

  if (mutex->locked_by == curr_thread->id){
    printk("Warning: mutex resource %d already locked by thread\n", mutex->id);
    return;
  }
  
  if (mutex->locked_by == -1 && !locked_geq_prio_mutex(curr_thread)){
    mutex->locked_by = curr_thread->id;
    if (mutex->prio_ceil < curr_thread->dyn_prio) curr_thread->dyn_prio = mutex->prio_ceil;
  } else {
    //Acknowledge lock request in mutex's pending bit vector. Swap out thread
    set_pending_state(mutex, curr_thread->id, 1);
    curr_thread->state = BLOCKED;
    pend_pendsv();
  }
}

/**
 * @brief  unlocks a resource, i.e a mutex 
 * @param  mutex    the mutex to unlock
*/
void sys_mutex_unlock( kmutex_t *mutex ) {
  tcb_t *curr_thread = &gcb.tcbs[gcb.active_id];
  if (curr_thread->id == IDLE_THREAD_IDX) return;

  if (mutex->locked_by != curr_thread->id){
    printk("Warning: mutex resource %d not locked by thread\n", mutex->id);
    return;
  }
  
  mutex->locked_by = -1;
  int fallback_prio = get_fallback_prio(curr_thread);
  if (fallback_prio != -1) curr_thread->dyn_prio = fallback_prio;
  else curr_thread->dyn_prio = curr_thread->static_prio;

  //After unlocking, grant lock request to another thread
  for (int i=USER_THREAD_FIRST_IDX; i < gcb.next; i++){
    tcb_t *thread = &gcb.tcbs[i];
    //Grant to thread that doesn't violate IPCP rules
    if (get_pending_state(mutex, thread->id) && !locked_geq_prio_mutex(thread)){
      mutex->locked_by = thread->id;
      set_pending_state(mutex, thread->id, 0);
      //Nested mutex might mean state is no longer in blocked because of earlier unlock
      if (thread->state == BLOCKED) thread->state = RUNNABLE;
      pend_pendsv();
      break;
    }
  }
  
}

tcb_t *get_active_thread(){
   tcb_t *curr_thread = NULL;
    uint32_t i;
    for(i=0; i<gcb.next; i++){
        if (gcb.tcbs[i].state == RUNNING) {
          curr_thread = &gcb.tcbs[i];
          break;
        }
    }
    return curr_thread;
}

tcb_t *get_next_thread(){
  uint32_t max_prio = (1 << gcb.max_threads); //use high upper bound number
  tcb_t *next_thread = NULL;

  for (int i=USER_THREAD_FIRST_IDX; i < gcb.next; i++){
   tcb_t *thread = &gcb.tcbs[i];
    if ((thread->state == RUNNABLE || thread->state == RUNNING)){
      uint32_t thread_prio = get_curr_prio(thread->id);
      if (thread_prio < max_prio){
        max_prio = thread_prio;
        next_thread = thread;
      } else if (thread_prio == max_prio && next_thread != NULL){
        /* An escalated thread causing priority conflicts should be run first if it uses a mutex
        Ensures thread is only blocked once initially as is protocol with IPCP */
        if (is_using_mutex(thread->id)) next_thread = thread;
      }
    }
  }

  return next_thread;
}

int stack_overflows(uint32_t max_threads, uint32_t stack_size) {
  uint32_t needed = max_threads *  (1 << mm_log2ceil_size(stack_size)) * 4;
  uint32_t avail_u = (uint32_t)&__thread_u_stacks_top - (uint32_t)&__thread_u_stacks_low;
  uint32_t avail_k = (uint32_t)&__thread_k_stacks_top - (uint32_t)&__thread_k_stacks_low;

  if (needed > avail_u || needed > avail_k) return 1;
  return 0;  
}

/**
 * @brief initializes default and idle threads
 * @param  idle_fn     pointer to the idle function, or NULL if none provided
 *
 */
void set_default_threads(void *idle_fn) {
  /* Function is called in thread_init. Consequently,invariant is that 
    array idx 0 & 1 are assigned to main & idle threads respectively
   */
 tcb_t *main_thread = &gcb.tcbs[gcb.next];
  main_thread->id = gcb.next++;
  main_thread->state = RUNNING;

  //Idle function thread setup similar to regular thread
 tcb_t *idle_thread = &gcb.tcbs[gcb.next];
  idle_thread->id = gcb.next++;
  idle_thread->state = RUNNABLE;
  idle_thread->svc_status = 0;
    
  void *used_idle_fn = idle_fn;

  extern void idle_default();
  if (!used_idle_fn){
      used_idle_fn = &idle_default;
  }else{
      gcb.k_stack_next += gcb.stack_size * 4;
      gcb.u_stack_next += gcb.stack_size * 4;
  }

  idle_thread->msp = (void*)gcb.k_stack_next;
  idle_thread->k_stack_high = gcb.k_stack_next;
  idle_thread->psp = (void*)gcb.u_stack_next;
  idle_thread->u_stack_high = gcb.u_stack_next;

  setup_init_stack_frame(idle_thread, used_idle_fn, (void*)0);
}

void setup_init_stack_frame(tcb_t *thread, void *fn, void *vargp) {
  /*Edit ARM-saved context to run function initally 
  and push callee-saved context onto main stack for assembly handling on restoration */
  interrupt_stack_frame *init_psp = (interrupt_stack_frame*)thread->psp - 1;
  init_psp->pc = (uint32_t)fn;
  init_psp->xPSR = XPSR_INIT;
  init_psp->r0 = (uint32_t)vargp;
  init_psp->lr = (uint32_t)&thread_kill;
  thread->psp = init_psp;

  callee_stack_frame *init_msp = (callee_stack_frame*)thread->msp - 1;
  init_msp->psp = (uint32_t)thread->psp;
  init_msp->lr = PSP_THREAD;
  thread->msp = init_msp;
}

int is_schedulable(uint32_t newC, uint32_t newT){
  float sum = 0;
  for (int i = USER_THREAD_FIRST_IDX; i < gcb.next; i++){
    if (gcb.tcbs[i].state == INACTIVE) continue;
    sum += (float)gcb.tcbs[i].C/ (float)gcb.tcbs[i].T;
  }

  sum += (float)newC / (float)newT;
  int num_threads = gcb.next - USER_THREAD_FIRST_IDX  -gcb.num_inactive + 1;
  if (sum <= ub_table[num_threads]) return 1;
  return 0;
}

void update_thread_times() {
  //Update execution status of active thread
 tcb_t *curr_thread = &gcb.tcbs[gcb.active_id];
  if (!(curr_thread->id == MAIN_THREAD_IDX || curr_thread->id == IDLE_THREAD_IDX)){
      curr_thread->running_C++;
      curr_thread->total_C++;
      if (curr_thread->running_C == curr_thread->C){
        curr_thread->running_C = 0;
        curr_thread->state = WAITING;
      }
  }

  //Search through threads and prepare woken ones for running
  for (int i=USER_THREAD_FIRST_IDX; i < gcb.next; i++){
   tcb_t *thread = &gcb.tcbs[i];
    if (thread->state == INACTIVE) continue;
    if (thread->next_deadline <=  gcb.tick_count){
      thread->state= RUNNABLE;
      thread->running_C= 0;
      thread->last_deadline= thread->next_deadline;
      thread->next_deadline= thread->next_deadline + thread->T;
    }
  }
}

/**
 * @brief  gets an inactive thread which we can restart
 *
 * @return  an inactive thread which we can restart
 */
tcb_t *find_inactive_thread(){
  tcb_t *thread= NULL;

  for (int i=USER_THREAD_FIRST_IDX; i < gcb.next; i++){
    if (gcb.tcbs[i].state == INACTIVE){ 
      thread = &gcb.tcbs[i];
      break;
    }
  }
  return thread;
}

/**
 * @brief  set mutex pending state
 *
 * @param  mutex       passed kernel mutex
 * @param  thread_id   the passed thread tcb structure
 * @param  val         the value od the mutex
 *
 * @return  the higher priority locked mutex
 */
void set_pending_state(kmutex_t *mutex, uint32_t thread_id, uint32_t val){
  if (val) mutex->pending |= (1 << thread_id);
  else mutex->pending &= ~(1 << thread_id);
}

int get_pending_state(kmutex_t *mutex, uint32_t thread_id){
  return (mutex->pending >> thread_id) & 1;
}

/**
 * @brief  if the priority of the locked mutex is >= get the higher mutex
 *
 * @param  thread      the passed thread TCB structure
 * @return  the higher priority locked mutex
 */
int locked_geq_prio_mutex(tcb_t *thread){
  int higher_locked_mutex = 0;

  for (uint32_t i=0; i < gcb.num_mutexes; i++){
    kmutex_t *search_mutex = &gcb.mutexes[i];
    if  (search_mutex->locked_by != -1 &&
         search_mutex->locked_by != thread->id && search_mutex->prio_ceil <= get_curr_prio(thread->id)){
      higher_locked_mutex =  1;
      break;
    }
  }
  return higher_locked_mutex;
}

/**
 * @brief  get the curent priority of the passed thread
 *
 * @param  thread_id   the passed thread TCB structure
 * @return  the current priority of the thread
 */
uint32_t get_curr_prio(uint32_t thread_id){
  if (gcb.tcbs[thread_id].dyn_prio < gcb.tcbs[thread_id].static_prio) return gcb.tcbs[thread_id].dyn_prio;
  return gcb.tcbs[thread_id].static_prio;
}

/**
 * @brief  get the fallback priority of the passed thread in IPCP scheduling
 *
 * @param  thread      the passed thread TCB structure
 */
int get_fallback_prio(tcb_t *thread){
  int max_prio_mutex = (1 << gcb.max_threads); //use high upper bound number

  for (uint32_t i=0; i < gcb.num_mutexes; i++){
    kmutex_t *search_mutex = &gcb.mutexes[i];
    if (search_mutex->locked_by == thread->id &&
        (int)search_mutex->prio_ceil < max_prio_mutex){
          max_prio_mutex = search_mutex->prio_ceil;
    }
  }
  if (max_prio_mutex == (1 << gcb.max_threads)) return -1;
  return max_prio_mutex;
}

/**
 * @brief  checks if a thread is currently using a mutex
 *
 * @param  thread_id   the identifier of the thread
 */
uint32_t is_using_mutex(uint32_t thread_id){
  int is_using = 0;

  for (uint32_t i=0; i < gcb.num_mutexes; i++){
    if (gcb.mutexes[i].locked_by == (int)thread_id){
      is_using = 1;
      break;
    }
  }

  return is_using;
}

/**
 * @brief  switches which memory region is protected
 *
 * @param  next_thread        the next scheduled thread
 */
void switch_mem_protect(tcb_t *next_thread){

    uint32_t region_size= gcb.stack_size * 4;
    uint32_t next_u_stack_base= (next_thread->u_stack_high) -region_size;

    if(next_thread->id != MAIN_THREAD_IDX){
        mm_region_disable(6);
        mm_region_enable(6, (void*)next_u_stack_base, mm_log2ceil_size(region_size), 0, 1);
    }
}
