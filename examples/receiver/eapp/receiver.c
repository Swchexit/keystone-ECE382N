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

uint32_t recv_msg(struct mailbox *mb, uint8_t *out, uint32_t max_len)
{
    // Wait until mailbox is full
    uint32_t s;
    do {
        s = atomic_load_explicit(&mb->state, memory_order_acquire);
    } while (s != MAILBOX_FULL);

    // Ensure we see payload after observing FULL
    atomic_thread_fence(memory_order_acquire);

    if (max_len > PAYLOAD_SIZE) max_len = PAYLOAD_SIZE;

    // Copy data out (simple loop)
    for (uint32_t i = 0; i < max_len; ++i)
        out[i] = mb->payload[i];

    // Mark mailbox empty again
    atomic_store_explicit(&mb->state, MAILBOX_EMPTY, memory_order_release);

    return max_len;  // in this simple version, we just always return max_len
}

int main()
{
  // write some data to shared memory
//   volatile char* shared_mem = (char*)EYRIE_SHARED_START;
  // printf("Writing to shared memory at address: %p\n", shared_mem);
  // shared_mem[0] = 'b';
  // shared_mem[1] = 'a';
  // shared_mem[2] = 'n';
  // shared_mem[3] = 'a';
  // shared_mem[4] = 'n';
  // shared_mem[5] = 'a';
  // shared_mem[6] = '\0';
  fprintf(stderr, "hello from receiver!\n");

  // read back from the shared memory
  // printf("Reading from shared memory: %s\n", shared_mem);
  struct mailbox *mb = (struct mailbox *)EYRIE_SHARED_START;
  uint8_t buffer[PAYLOAD_SIZE];
  recv_msg(mb, buffer, PAYLOAD_SIZE);
  fprintf(stderr, "Received message: %s\n", buffer);
  // Just try to read directly for now
//   fprintf(stderr, "Recieved message: %s\n", shared_mem);
  return 0;
}
