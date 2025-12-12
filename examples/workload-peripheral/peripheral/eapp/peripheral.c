#include <stdio.h>
#include <time.h>

#define EYRIE_UNTRUSTED_START 0xffffffff80000000
#define EYRIE_SHARED_START 0xffffffffa0000000


void wait_for_seconds(int seconds) {
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < seconds) {
        // Busy-wait loop
    }
}

int main()
{
  // write some data to shared memory
  volatile char* shared_mem = (char*)EYRIE_SHARED_START;
  fprintf(stderr, "Writing to shared memory at address: %p\n", shared_mem);
  shared_mem[0] = 'h';
  shared_mem[1] = 'e';
  shared_mem[2] = 'l';
  shared_mem[3] = 'l';
  shared_mem[4] = 'o';
  shared_mem[5] = '!';
  shared_mem[6] = '\0';
  // printf("hello, world!\n");

  // read back from the shared memory
//   time_t start_time = time(NULL);
//   printf("Start time is %ld\n", start_time);
//   printf("Reading from shared memory: %s\n", shared_mem);

  fprintf(stderr, "Peripheral finished writing to shared memory, exiting.\n");
  return 0;
}
