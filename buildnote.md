# issue:  https://github.com/keystone-enclave/keystone/issues/455

Just a workaround for those stuck on these build issues. We get build failures on both Ubuntu 20.04 and Fedora 39 if defconfig qemu_riscv64_virt_defconfig is used. Build completes fine on a Ubuntu 20.04 if riscv64_generic_defconfig is used.


$ export BUILDROOT_CONFIGFILE=riscv64_generic_defconfig
$ export KEYSTONE_PLATFORM=generic
$ export KEYSTONE_BITS=64
$ export BUILDROOT_TARGET=all
$ make -j$(nproc)

$ qemu-system-riscv64 -machine virt -nographic  -smp 1   -m 2G   -bios build-generic64/buildroot.build/images/fw_jump.bin  -kernel build-generic64/buildroot.build/images/Image -append "root=/dev/vda ro console=ttyS0"  -drive file=build-generic64/buildroot.build/images/rootfs.ext2,format=raw,id=hd0   -device virtio-blk-device,drive=hd0