QEMU ?= qemu-system-aarch64
QEMU_AUDIO_DRV = "none"
QEMU_FLAGS  = -machine virt,secure=on,acpi=off -m 1G -smp 2
QEMU_FLAGS += -nographic
QEMU_FLAGS += -cpu cortex-a57
# ATF bl1 boot loader
QEMU_FLAGS += -bios $(BR_BUILD)/images/bl1.bin
# Virtio rootfs
QEMU_FLAGS += -drive file=$(BR_BUILD)/images/rootfs.squashfs,if=none,format=raw,id=hd0
QEMU_FLAGS += -device virtio-blk-device,drive=hd0
# Kernel image
QEMU_FLAGS += -kernel $(BR_BUILD)/images/Image
# Networking
#QEMU_FLAGS += -netdev tap,id=n1 -device virtio-net,netdev=n1
# Kernel boot parameters
QEMU_FLAGS += -append "rootwait root=/dev/vda console=ttyAMA0 keep_bootcon"
# Enable logging of unimplemeted features
QEMU_FLAGS += -d unimp
# RTC
QEMU_FLAGS += -rtc base=utc
# Virtio serial port for punchboot
QEMU_FLAGS += -device virtio-serial-device
QEMU_FLAGS += -chardev socket,path=/tmp/pb.sock,server=on,wait=off,id=pb_serial
QEMU_FLAGS += -device virtserialport,chardev=pb_serial
# Enable semihosting, only bl1 and kernel is loaded through qemu
# bl2, bl31 and optee are loaded through the semihosting interface.
QEMU_FLAGS += -semihosting-config enable=on,target=native
# Monitor port
# QEMU_FLAGS += -monitor telnet::45454,server,nowait

# Note: ATF expects other images to be available in the directory where qemu
# is started.
.PHONY: qemu
qemu:
	@cd $(BR_BUILD)/images && $(QEMU) $(QEMU_FLAGS) $(QEMU_AUX_FLAGS)

