#!/bin/bash
BUILDROOT_TARGET=keystone-sm-dirclean make -j$(nproc)
BUILDROOT_TARGET=keystone-driver-dirclean make -j$(nproc)
BUILDROOT_TARGET=keystone-runtime-dirclean make -j$(nproc)
BUILDROOT_TARGET=host-keystone-sdk-dirclean make -j$(nproc)
BUILDROOT_TARGET=keystone-examples-dirclean make -j$(nproc)