//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#include "edge/edge_call.h"
#include "host/keystone.h"

using namespace Keystone;

Enclave createEnclave(
    const char* eappPath, const char* runtimePath, const char* loaderPath) {
  Enclave enclave;
  Params params;

  params.setFreeMemSize(256 * 1024);
  params.setUntrustedSize(256 * 1024);
  params.setConnectSize(8 * 1024);  // 8 KB shared buffer

  enclave.init(eappPath, runtimePath, loaderPath, params);

  enclave.registerOcallDispatch(incoming_call_dispatch);
  // edge_call_init_internals(
  //     (uintptr_t)enclave.getSharedBuffer(), enclave.getSharedBufferSize());

  // enclave.run();

  return enclave;  // Will use move semantics (RVO/NRVO)
}

int
main(int argc, char** argv) {
  // create encalve 1 and enclave 2
  Enclave enc1 = createEnclave(argv[1], argv[2], argv[3]);
  // Enclave enc2 = createEnclave(argv[1], argv[2], argv[3]);

  // Connect enc1 and enc2 before they start
  // enc1.connect(enc2);
  edge_call_init_internals(
      (uintptr_t)enc1.getSharedBuffer(), enc1.getSharedBufferSize());

  enc1.run();
  
  // Wait for 2 seconds(just to simulate some delay)
  // sleep(1);

  // edge_call_init_internals(
  //     (uintptr_t)enc2.getSharedBuffer(), enc2.getSharedBufferSize());

  // enc2.run();

  return 0;
}
