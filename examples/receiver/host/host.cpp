//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "edge/edge_call.h"
#include "host/keystone.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace Keystone;

#define RECEIVER_READY_FILE "/tmp/keystone_receiver_ready"
#define SENDER_EID_FILE "/tmp/keystone_sender_eid"
#define RECEIVER_CONNECTED_FILE "/tmp/keystone_receiver_connected"

static void wait_for_file(const char* path) {
  printf("Waiting for %s...\n", path);
  while (access(path, F_OK) != 0) {
    usleep(100000);  // 100ms
  }
  printf("Found %s\n", path);
}

static void create_flag_file(const char* path) {
  int fd = open(path, O_CREAT | O_WRONLY, 0644);
  if (fd >= 0) {
    close(fd);
    printf("Created %s\n", path);
  }
}

static int read_eid_from_file(const char* path) {
  int eid = -1;
  FILE* f = fopen(path, "r");
  if (f) {
    fscanf(f, "%d", &eid);
    fclose(f);
    printf("Read sender EID %d from %s\n", eid, path);
  }
  return eid;
}

static void write_eid_to_file(const char* path, int eid) {
  FILE* f = fopen(path, "w");
  if (f) {
    fprintf(f, "%d\n", eid);
    fclose(f);
    printf("Wrote receiver EID %d to %s\n", eid, path);
  }
}

int
main(int argc, char** argv) {
  if (argc < 4) {
    printf("Usage: %s <eapp> <runtime> <loader>\n", argv[0]);
    return 1;
  }

  Enclave enclave;
  Params params;

  params.setFreeMemSize(256 * 1024);
  params.setUntrustedSize(256 * 1024);
  params.setConnectSize(2 * 4096);  // Allocate shared memory for connection. 8KB is enough

  enclave.init(argv[1], argv[2], argv[3], params);

  enclave.registerOcallDispatch(incoming_call_dispatch);
  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  int receiver_eid = enclave.getEid();
  printf("Receiver enclave initialized with EID: %d\n", receiver_eid);

  // Signal that receiver is ready and write EID
  write_eid_to_file(RECEIVER_READY_FILE, receiver_eid);

  // Wait for sender EID (for info purposes)
  wait_for_file(SENDER_EID_FILE);
  int sender_eid = read_eid_from_file(SENDER_EID_FILE);
  printf("Sender EID is: %d\n", sender_eid);

  printf("Connecting to sender enclave (EID %d)...\n", sender_eid);
  Error err = enclave.connect(sender_eid);
  if (err != Error::Success) {
    printf("Failed to connect to sender enclave\n");
    return 1;
  }
  printf("Successfully connected to sender enclave\n");

  // Signal that receiver is ready to run
  create_flag_file(RECEIVER_CONNECTED_FILE);

  sleep(1);  // Small delay to ensure sender is ready
  printf("Starting receiver enclave...\n");

  enclave.run();

  return 0;
}
