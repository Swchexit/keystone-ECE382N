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

int main()
{
  // write some data to shared memory
  volatile char* shared_mem = (char*)EYRIE_SHARED_START;
  ocall_print_string("Writing to shared memory");
  shared_mem[0] = 'h';
  shared_mem[1] = 'e';
  shared_mem[2] = 'l';
  shared_mem[3] = 'l';
  shared_mem[4] = 'o';
  shared_mem[5] = '!';
  shared_mem[6] = '\0';
  ocall_print_string("hello, world!");

  // read back from the shared memory
  ocall_print_string("Reading from shared memory:");
  ocall_print_string((char*)shared_mem);

  // Read from untrusted memory
  unsigned int value = 0;
  if (copy_from_shared(&value, 0x2000, sizeof(unsigned int))) { // offset by 8KB
    ocall_print_string("Enclave: Failed to read from untrusted memory!");
  }
  ocall_print_string("Enclave: Read value from untrusted memory at offset 0x2000:");
  ocall_print_value(value);

  EAPP_RETURN(0);
}
