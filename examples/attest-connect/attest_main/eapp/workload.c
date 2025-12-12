//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "app/eapp_utils.h"
#include "app/string.h"
#include "app/syscall.h"
#include "edge/edge_call.h"
#include "edge/edge_common.h"

#define NONCE_OFFSET 0x2000
#define ENC_INFO_OFFSET 0x3000


#define OCALL_PRINT_STRING 1
#define OCALL_PRINT_VALUE 2
#define OCALL_CUSTOM 3
#define OCALL_WAIT 4
#define OCALL_GET_NONCE 5
#define OCALL_COPY_REPORT 6

#define EYRIE_UNTRUSTED_START 0xffffffff80000000
#define EYRIE_SHARED_START 0xffffffffa0000000

struct enc_info {
  unsigned int eid;
  char path[256];
};

struct connection_history {
  int len;
  struct enc_info entries[5];
};

struct connection_history history;
char nonce[2048];
char report[4096];

unsigned long ocall_print_string(char* string) {
  unsigned long retval;
  ocall(OCALL_PRINT_STRING, string, strlen(string)+1, &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_print_value(unsigned int value) {
  unsigned long retval;
  ocall(OCALL_PRINT_VALUE, &value, sizeof(unsigned int), &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_custom(unsigned int command) {
  unsigned long retval;
  ocall(OCALL_CUSTOM, &command, sizeof(unsigned int), &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_wait(unsigned int seconds) {
  unsigned long retval;
  ocall(OCALL_WAIT, &seconds, sizeof(unsigned int), &retval, sizeof(unsigned long));
  return retval;
}

unsigned long ocall_get_nonce(char* buffer, size_t bufsize) {
  struct edge_data retdata;
  ocall(OCALL_GET_NONCE, NULL, 0, &retdata, sizeof(struct edge_data));
  if (retdata.size > bufsize) retdata.size = bufsize;
  copy_from_shared(buffer, retdata.offset, retdata.size);
  return retdata.size;
}

unsigned long ocall_copy_report(char* buffer, size_t bufsize) {
  unsigned long retval;
  ocall(OCALL_COPY_REPORT, buffer, bufsize, &retval, sizeof(unsigned long));
  return retval;
}

int add_connection_history(struct enc_info *enc_info) {
  if (history.len < 5) {
    history.entries[history.len] = *enc_info;
    history.len++;
    ocall_print_string("Enclave: Added to connection history.");
    return 0;
  } else {
    ocall_print_string("Enclave: Connection history is full, cannot add more entries.");
    return -1;
  }
}

int do_attestation() {
  unsigned long ret = 0;
  ret = ocall_get_nonce(nonce, sizeof(nonce));
  if (ret == 0) {
    ocall_print_string("Enclave: Failed to get nonce from host!");
    return -1;
  }

  attest_enclave((void*)report, nonce, ret, (void*)0, 0);

  ret = ocall_copy_report(report, sizeof(report));
  if (ret == 0) {
    ocall_print_string("Enclave: Failed to copy report to host!");
    return -1;
  }
  return 0;
}

int main()
{
  unsigned int ret;
  // write some data to shared memory
  ocall_print_string("Main workload program started!");

  ocall_custom(1);

  // ocall_wait(1);

  // Read from untrusted memory
  unsigned int eid_self = 0;
  if (copy_from_shared(&eid_self, 0x2000, sizeof(unsigned int))) { // offset by 8KB
    ocall_print_string("Enclave: Failed to read from untrusted memory!");
  }
  ocall_print_string("Enclave: Read eid_self from untrusted memory at offset 0x2000:");
  ocall_print_value(eid_self);

  /* This is used to test attestation */
  ret = ocall_get_nonce(nonce, sizeof(nonce));
  ocall_print_string(nonce);
  ocall_print_value(ret);

  attest_enclave((void*)report, nonce, ret, (void*)0, 0);
  ocall_print_string("Report generated.");
  ocall_copy_report(report, sizeof(report));
  // do_attestation();


  /*
  // Start the real work here
  ocall_custom(2);  // after this call, the enc_info struct is in the untrusted memory at offset 0x3000!

  struct enc_info info;
  if (copy_from_shared(&info, ENC_INFO_OFFSET, sizeof(struct enc_info))) { // offset by 12KB
    ocall_print_string("Enclave: Failed to read enc_info from untrusted memory!");
  }
  ocall_print_string("Enclave: Read enc_info from untrusted memory:");
  ocall_print_string("  Path: ");
  ocall_print_string(info.path);
  ocall_print_string("Connecting to peripheral enclave with eid: ");
  ocall_print_value(info.eid);

  int ret = connect_enclaves(info.eid);
  ocall_print_string("Enclave: connect_enclaves returned: ");
  ocall_print_value(ret);

  // Add to history
  add_history(info);
  
  // This is a signal that the connection is established
  ocall_custom(3);

  // Wait till the peripheral is ready
  ocall_wait(5);
  */

  // Read from the shared mem to get data from peripheral enclave
  // volatile char* shared_mem = (char*)EYRIE_SHARED_START;
  // ocall_print_string("Reading from shared memory after connection:");
  // ocall_print_string((char*)shared_mem);

  ocall_print_string("Main workload program finished. Exiting.");
  EAPP_RETURN(0);
}
