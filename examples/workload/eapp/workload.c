//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "app/eapp_utils.h"
#include "app/string.h"
#include "app/syscall.h"
#include "edge/edge_call.h"

#define OCALL_PRINT_STRING 1
#define OCALL_PRINT_VALUE 2
#define OCALL_CUSTOM 3
#define OCALL_WAIT 4

#define EYRIE_UNTRUSTED_START 0xffffffff80000000
#define EYRIE_SHARED_START 0xffffffffa0000000

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

int main()
{
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

  // Start the real work here
  ocall_custom(2);  // after this call, the eid we're connecting to is in the untrusted memory!

  unsigned int eid_other = 0;
  if (copy_from_shared(&eid_other, 0x2000, sizeof(unsigned int))) { // offset by 8KB
    ocall_print_string("Enclave: Failed to read eid from untrusted memory!");
  }
  ocall_print_string("Connecting to peripheral enclave with eid: ");
  ocall_print_value(eid_other);

  int ret = connect_enclaves(eid_other);
  ocall_print_string("Enclave: connect_enclaves returned: ");
  ocall_print_value(ret);
  
  // This is a signal that the connection is established
  ocall_custom(3);

  // Wait till the peripheral is ready
  ocall_wait(5);

  // Read from the shared mem to get data from peripheral enclave
  volatile char* shared_mem = (char*)EYRIE_SHARED_START;
  ocall_print_string("Reading from shared memory after connection:");
  ocall_print_string((char*)shared_mem);

  ocall_print_string("Main workload program finished. Exiting.");
  EAPP_RETURN(0);
}
