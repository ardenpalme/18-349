/**
 * @file kmalloc.c
 *
 * @brief      Implementation of heap management library. This is a very
 *             simple library, it keeps a struct for tracking its state. Each
 *             kmalloc struct takes a heap start and end pointer. Heaps are
 *             defined in the linker script. Each kmalloc struct can be
 *             initilized to do either aligned or unaligned allocations.
 *
 *             Aligned allocations - In this, the size of all the allocations
 *             must be set in kmalloc init. Calling k_malloc_aligned will give
 *             you a region of whatever size you initilized kmalloc to. To
 *             perform frees on aligned regions, we just add the regions to a
 *             linked list. In the next allocation, we check the linked list
 *             before moving the break pointer.
 *
 *             In unaligned allocations, the caller may specify the size they
 *             want. You do not need to support frees on unaligned regions.
 *
 * @date       Febuary 12, 2019
 *
 * @author     Ronit Banerjee <ronitb@andrew.cmu.edu>
 */

#include "kmalloc.h"

#include <debug.h>
#include <unistd.h>

list_node *findLast(list_node *free_node);

/**
 * @brief      Initiliazes the kmalloc structure.
 *
 * @param[in]  internals        The internals
 * @param[in]  heap_low         The heap low
 * @param[in]  heap_top         The heap top
 * @param[in]  stack_size       The stack size
 * @param[in]  unaligned        This flags sets whether you can perfrom
 *                              unaligned allocations. If this flag is set,
 *                              then the stack size variable is ignored.
 *
 * @return     Returns 0 if allocation was successful, or -1 otherwise.
 */
void k_malloc_init( kmalloc_t* internals,
                    char* heap_low,
                    char* heap_top,
                    uint32_t stack_size,
                    uint32_t unaligned ){
    internals->heap_low= heap_low;
    internals->heap_top= heap_top;

    internals->prev_low= heap_low;
    internals->curr_low= heap_low;

    internals->stack_size= stack_size;

    internals->unaligned= unaligned;
    internals->free_node= NULL;
}

/**
 * @brief      This function lets you perform unaligned allocations. If the
 *             unaligned flag has not be set, you should fall into a
 *             breakpoint look at debug.h to see how to do this.
 *
 * @param[in]  internals  The kmalloc internal kernel structure
 * @param[in]  size       The allocation size.
 *
 * @return     Returns the pointer to the allocated buffer.
 */
void* k_malloc_unaligned( kmalloc_t* internals,
                          uint32_t size ){
    ASSERT(internals->unaligned);
    if(!size){
        internals->curr_low= (char*)(internals->curr_low + size);
        return (void*)internals->curr_low;
    }

    internals->prev_low= internals->curr_low;
    internals->curr_low= (char*)(internals->curr_low + size);
    return (void*)internals->prev_low;
}

/**
 * @brief      This function performs aligned allocations.
 *
 * @param[in]  internals  The internals structure.
 *
 * @return     Pointer to allocated buffer, can be NULL.
 */
void* k_malloc_aligned( kmalloc_t* internals ){
    ASSERT(!(internals->unaligned));

    /*check free-list first*/
    list_node *tmp;
    if(internals->free_node != NULL){
        tmp= internals->free_node;
        /* pop the node */
        internals->free_node = internals->free_node->next;
        return (void*)tmp;
    }

    internals->prev_low= internals->curr_low;
    internals->curr_low= (char*)((internals->curr_low) +
                            (internals->stack_size));
    if(internals->curr_low > internals->heap_top)
        return NULL;
    return (void*)internals->prev_low;
}

/**
 * @brief      This function allows you to free aligned chunks.
 *
 * @param[in]  internals  The internals structure.
 * @param[in]  buffer      Pointer to a buffer that was obtained.
 *
 * @warning    The pointer to the buffer must be the original pointer that was
 *             obtained from k_malloc_aligned. For example, if you are using
 *             the buffer as a stack it must be the orignial pointer you
 *             obtained and not the current stack position.
 */


void k_free( kmalloc_t* internals, void* buffer ){
    list_node *new_node;
    list_node *last_node;

    new_node= (list_node*)buffer;
    new_node->next= NULL;
    last_node= findLast(internals->free_node);

    if(last_node == NULL)
        internals->free_node= new_node;
    else
        last_node->next= new_node;
}
/** @brief - returns the last node in a linked-list of heap nodes
 */
list_node *findLast(list_node *free_node){
    list_node *last_node;

    /* free_node -> NULL*/
    if(free_node == NULL){
        last_node= NULL;

    /* free_node -> node -> NULL*/
    }else if((free_node != NULL) &&
            (free_node->next == NULL)){
        last_node= free_node;

    /* free_node -> node -> ... -> node -> NULL*/
    }else{
        last_node= free_node;
        while(last_node->next != NULL){
            last_node= last_node->next;
        }
    }
    return last_node;
}
