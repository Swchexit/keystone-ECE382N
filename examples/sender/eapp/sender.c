#include <stdio.h>
#include <stdatomic.h>
#include <stdint.h>

#define EYRIE_UNTRUSTED_START 0xffffffff80000000
#define EYRIE_SHARED_START 0xffffffffa0000000

#define MAILBOX_EMPTY 0
#define MAILBOX_FULL  1
#define PAYLOAD_SIZE  256

struct mailbox {
    _Atomic uint32_t state;    // 0 = empty, 1 = full
    uint8_t          payload[PAYLOAD_SIZE];
};

void send_msg(struct mailbox *mb, const uint8_t *data, uint32_t len)
{
    if (len > PAYLOAD_SIZE) len = PAYLOAD_SIZE;

    // Wait until mailbox is empty
    uint32_t s;
    do {
        s = atomic_load_explicit(&mb->state, memory_order_acquire);
    } while (s != MAILBOX_EMPTY);

    // Copy data (no libc memcpy; simple loop)
    for (uint32_t i = 0; i < len; ++i)
        mb->payload[i] = data[i];

    // Zero out the rest (optional)
    for (uint32_t i = len; i < PAYLOAD_SIZE; ++i)
        mb->payload[i] = 0;

    // Ensure payload writes are visible before marking as full
    atomic_thread_fence(memory_order_release);

    atomic_store_explicit(&mb->state, MAILBOX_FULL, memory_order_release);
}



int main()
{
  // write some data to shared memory
//   volatile char* shared_mem = (char*)EYRIE_SHARED_START;
  // printf("Writing to shared memory at address: %p\n", shared_mem);
  
  fprintf(stderr, "hello, from sender!\n");
  
//   shared_mem[0] = 'h';
//   shared_mem[1] = 'e';
//   shared_mem[2] = 'l';
//   shared_mem[3] = 'l';
//   shared_mem[4] = 'o';
//   shared_mem[5] = '!';
//   shared_mem[6] = '\0';
  

  // read back from the shared memory
  // printf("Reading from shared memory: %s\n", shared_mem);

  struct mailbox *mb = (struct mailbox *)EYRIE_SHARED_START;
  const char *msg = "Hello from sender enclave!";
  send_msg(mb, (const uint8_t *)msg, strlen(msg) + 1);

  fprintf(stderr, "Message sent!\n");

  // just hang here
  // while(1) {
  //   // does nothing
  // }
  return 0;
}
