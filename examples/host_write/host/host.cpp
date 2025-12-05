//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "edge/edge_call.h"
#include "host/keystone.h"

#define OCALL_PRINT_STRING 1
#define OCALL_PRINT_VALUE 2

using namespace Keystone;

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

  edge_call_init_internals(
      (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  return enclave;  // Will use move semantics (RVO/NRVO)
}

void writeToUntrusted(Enclave &enc, unsigned int value) {
    unsigned int* shared_mem = (unsigned int*)((char*)enc.getSharedBuffer() + 0x2000); // offset by 8KB
    *shared_mem = value;
    printf("Host: Wrote %u to untrusted memory at %p\n", value, shared_mem);
}

int
main(int argc, char** argv) {
  // create encalve 1 and enclave 2
  Enclave enc1 = createEnclave(argv[1], argv[2], argv[3]);

  // Connect enc1 and enc2 before they start
  writeToUntrusted(enc1, 8769420U);
  enc1.run();
  
  // Wait for 2 seconds(just to simulate some delay)
  // sleep(1);

  return 0;
}
