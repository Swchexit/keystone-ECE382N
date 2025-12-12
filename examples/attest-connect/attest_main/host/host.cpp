//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "edge/edge_call.h"
#include "edge/edge_common.h"
#include "host/keystone.h"
#include "verifier/report.h"

#define OCALL_PRINT_STRING 1
#define OCALL_PRINT_VALUE 2
#define OCALL_CUSTOM 3
#define OCALL_WAIT 4
#define OCALL_GET_NONCE 5
#define OCALL_COPY_REPORT 6

#define NONCE_OFFSET 0x2000
#define ENC_INFO_OFFSET 0x3000

using namespace Keystone;

Enclave* enc;
char report_buffer[4096];

#define PERI_EID_FILE "/tmp/keystone_peri_eid"    
#define CAN_START_FILE "/tmp/keystone_peri_can_start"

char nonce[] = "Pages full of unlikely words, Handfuls of hot, bitter tears. They call the author a silly fool, For they know not what he means.";


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

static enc_info read_eid_path_from_file(const char* path) {
  struct enc_info info;

  FILE* f = fopen(path, "r");
  if (f) {
    fscanf(f, "%d", &info.eid);
    // read the rest of the line as path
    char buf[256];
    fgets(buf, sizeof(buf), f);
    strncpy(info.path, buf, sizeof(info.path) - 1);
    info.path[sizeof(info.path) - 1] = '\0'; // ensure null-termination
    fclose(f);
    printf("Read receiver EID %d with path %s\n", info.eid, info.path);
  }
  return info;
}

void writeToUntrusted(Enclave &enc, unsigned int value);
void writeEncInfoToUntrusted(Enclave &enc, struct enc_info &info);

unsigned long print_string(char* str) {
  return printf("Enclave said: \"%s\"\n", str);
}

unsigned long print_value(unsigned int value) {
  return printf("Enclave said: %u\n", value);
}

void print_string_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  ret_val = print_string((char*)call_args);

  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }
}

void print_value_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  unsigned int value = *(unsigned int*)call_args;
  ret_val = print_value(value);

  uintptr_t data_section = edge_call_data_ptr();
  memcpy((void*)data_section, &ret_val, sizeof(unsigned long));
  if (edge_call_setup_ret(edge_call, (void*)data_section, sizeof(unsigned long))) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_PTR;
  } else {
    edge_call->return_data.call_status = CALL_STATUS_OK;
  }
}

void custom_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  unsigned int command = *(unsigned int*)call_args;
  // Custom logic can be added here
  if (command == 1) {
    printf("Host: custom command 1, writing eid of main to untrusted memory\n");
    writeToUntrusted(*enc, (unsigned int)enc->getEid());
  } else if (command == 2) {
    printf("Host: custom command 2, waiting for peripheral eid file\n");
    wait_for_file(PERI_EID_FILE);
    auto info_other = read_eid_path_from_file(PERI_EID_FILE);
    writeEncInfoToUntrusted(*enc, info_other);
    printf("Host: custom command 2, wrote peripheral eid %u path %s to untrusted memory\n", (unsigned int)info_other.eid, info_other.path);
  } else if (command == 3) {
    printf("Host: custom command 3, creating can start file\n");
    create_flag_file(CAN_START_FILE);
  } else if (command == 4) {
    // Print report
  }
  else {
    // unknown command
    printf("Host: custom_wrapper received unknown command %u\n", command);
  }
  
  // End of custom logic, return OK
  edge_call->return_data.call_status = CALL_STATUS_OK;
}

void wait_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  unsigned long ret_val;
  size_t arg_len;
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  unsigned int value = *(unsigned int*)call_args;
  printf("Host: wait_wrapper called, simulating wait for %u seconds\n", value);
  sleep(value); // Simulate some wait
  printf("Host: wait_wrapper finished waiting.\n");
  edge_call->return_data.call_status = CALL_STATUS_OK;
}

void get_nonce_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;
  const size_t nonce_size = sizeof(nonce);
  
  // Setup edge_data wrapper with offset and size
  struct edge_data data_wrapper;
  data_wrapper.size = nonce_size;
  data_wrapper.offset = sizeof(struct edge_call) + sizeof(struct edge_data);
  
  // Copy nonce data after the edge_data struct
  memcpy((void*)(_shared_start + sizeof(struct edge_call) + sizeof(struct edge_data)), nonce, nonce_size);
  
  // Copy edge_data wrapper
  memcpy((void*)(_shared_start + sizeof(struct edge_call)), &data_wrapper, sizeof(struct edge_data));
  
  // Setup return
  edge_call->return_data.call_ret_offset = sizeof(struct edge_call);
  edge_call->return_data.call_ret_size = sizeof(struct edge_data);
  edge_call->return_data.call_status = CALL_STATUS_OK;
}

void copy_report_wrapper(void* buffer) {
  struct edge_call* edge_call = (struct edge_call*)buffer;
  uintptr_t call_args;
  size_t arg_len;
  
  // Get the report data sent by the enclave
  if (edge_call_args_ptr(edge_call, &call_args, &arg_len) != 0) {
    edge_call->return_data.call_status = CALL_STATUS_BAD_OFFSET;
    return;
  }

  // Copy report from enclave to host buffer
  size_t copy_size = (arg_len < sizeof(report_buffer)) ? arg_len : sizeof(report_buffer);
  memcpy(report_buffer, (void*)call_args, copy_size);
  
  printf("Host: Received attestation report (%zu bytes)\n", copy_size);
  
  // Parse and print the report
  Report report;
  report.fromBytes((byte*)report_buffer);
  report.printPretty();
  
  edge_call->return_data.call_status = CALL_STATUS_OK;
}

Enclave createEnclave(
    const char* eappPath, const char* runtimePath, const char* loaderPath) {
  Enclave enclave;
  Params params;

  params.setFreeMemSize(256 * 1024);
  params.setUntrustedSize(256 * 1024);
  params.setConnectSize(8 * 1024);  // 8 KB shared buffer

  enclave.init(eappPath, runtimePath, loaderPath, params);

  enclave.registerOcallDispatch(incoming_call_dispatch);

  register_call(OCALL_PRINT_STRING, print_string_wrapper);
  register_call(OCALL_PRINT_VALUE, print_value_wrapper);
  register_call(OCALL_CUSTOM, custom_wrapper);
  register_call(OCALL_WAIT, wait_wrapper);
  register_call(OCALL_GET_NONCE, get_nonce_wrapper);
  register_call(OCALL_COPY_REPORT, copy_report_wrapper);

  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  return enclave;  // Will use move semantics (RVO/NRVO)
}

void writeToUntrusted(Enclave &enc, unsigned int value) {
    unsigned int* shared_mem = (unsigned int*)((char*)enc.getSharedBuffer() + 0x2000); // offset by 8KB
    *shared_mem = value;
    printf("Host: Wrote %u to untrusted memory at %p\n", value, shared_mem);
}

void writeEncInfoToUntrusted(Enclave &enc, struct enc_info &info) {
    char* shared_mem = (char*)enc.getSharedBuffer() + 0x3000; // offset by 12KB
    memcpy(shared_mem, &info, sizeof(struct enc_info));
    printf("Host: Wrote eid %u and path %s to untrusted memory at %p\n", info.eid, info.path, shared_mem);
}

int
main(int argc, char** argv) {
  // create encalve 1 and enclave 2
  Enclave enc1 = createEnclave(argv[1], argv[2], argv[3]);
  enc = &enc1;

  enc1.run();
  
  return 0;
}
