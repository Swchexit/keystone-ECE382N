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
  }
}

static void write_eid_to_file(const char* path, int eid) {
  FILE* f = fopen(path, "w");
  if (f) {
    fprintf(f, "%d\n", eid);
    fclose(f);
    printf("Wrote sender EID %d to %s\n", eid, path);
  }
}

static int read_eid_from_file(const char* path) {
  int eid = -1;
  FILE* f = fopen(path, "r");
  if (f) {
    fscanf(f, "%d", &eid);
    fclose(f);
    printf("Read receiver EID %d from %s\n", eid, path);
  }
  return eid;
}

int
main(int argc, char** argv) {
  if (argc < 4) {
    printf("Usage: %s <eapp> <runtime> <loader>\n", argv[0]);
    return 1;
  }

  // Clean up any old sync files
  unlink(RECEIVER_READY_FILE);
  unlink(SENDER_EID_FILE);
  unlink(RECEIVER_CONNECTED_FILE);

  Enclave enclave;
  Params params;

  params.setFreeMemSize(256 * 1024);
  params.setUntrustedSize(256 * 1024);
  // params.setConnectSize(256 * 1024);  // Allocate shared memory for connection

  enclave.init(argv[1], argv[2], argv[3], params);

  enclave.registerOcallDispatch(incoming_call_dispatch);
  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  int sender_eid = enclave.getEid();
  printf("Sender enclave initialized with EID: %d\n", sender_eid);

  // Wait for receiver to be ready
  wait_for_file(RECEIVER_READY_FILE);
  int receiver_eid = read_eid_from_file(RECEIVER_READY_FILE);

  if (receiver_eid < 0) {
    printf("Failed to read receiver EID\n");
    return 1;
  }

  // Connect to receiver enclave
  // printf("Connecting to receiver enclave (EID %d)...\n", receiver_eid);
  // Error err = enclave.connect(receiver_eid);
  // if (err != Error::Success) {
  //   printf("Failed to connect to receiver enclave\n");
  //   return 1;
  // }
  // printf("Successfully connected to receiver enclave\n");

  // Write sender EID for receiver (for info purposes)
  write_eid_to_file(SENDER_EID_FILE, sender_eid);

  // Wait for receiver to be ready to run
  wait_for_file(RECEIVER_CONNECTED_FILE);
  printf("Receiver is ready, starting sender enclave...\n");

  enclave.run();

  // Cleanup
  unlink(RECEIVER_READY_FILE);
  unlink(SENDER_EID_FILE);
  unlink(RECEIVER_CONNECTED_FILE);

  return 0;
}
