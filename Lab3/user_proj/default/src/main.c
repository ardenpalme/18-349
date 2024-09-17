#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define UNUSED __attribute__((unused))

/** @brief  Executes a fairly long delay. */
int delay(int status) {
  int x = 18;
  for (int i = 0; i < 100000; i++) {
    x = x + 349;
  }
  printf("\nBytes last printed: %d\n", status);
  return x;
}

int main(UNUSED int argc, UNUSED char const *argv[]){
    /*
    status= write(1, ">> ", 18);
    status= write(1, "Entered: \"", 11);
    status= write(1, buf, 90);
    status= write(1, ".\"\n", 3);

    exit(123);
    */
    int status= 0;
    char buf[100];
    printf("hello\n");
    status= write(1, "Enter: ", 11);
    status= read(0, &buf, 90);
    buf[status]=0;
    printf("%s\n",buf);
    (void)status;
    return 0;
}
