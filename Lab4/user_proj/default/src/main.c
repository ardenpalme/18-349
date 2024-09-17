#include <stdio.h>
#include <349_threads.h>
#define UNUSED __attribute__((unused))

int test(int x){
    printf("result= %d", 3*x);
    return (3*x);
}

int main(UNUSED int argc, UNUSED char const *argv[]){
    printf("thread syscall test\n");
    thread_init(16, 0, NULL, 0, 0);

    int (*fn)(int)= &test;
    int x= 3; 
    thread_create((void*)fn, 1, 20, 30, &x);
    
    scheduler_start(16000000);
    while(1);
    return 0;
}
