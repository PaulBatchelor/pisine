OBJS = mainc.o
OBJS += sysinitc.o
OBJS += exceptionstub.o
OBJS += del.o
OBJS += sync.o
OBJS += pgtbl.o
OBJS += mem.o
OBJS += interruptc.o
OBJS += gpio_c.o
OBJS += pwm.o

OBJS += ftbl.o
OBJS += osc.o

CFLAGS += -DNO_LIBSNDFILE

default: kernel.img

RASPPI	?= 1
PREFIX	?= arm-none-eabi-

FLOAT_ABI ?= hard

CC	= $(PREFIX)gcc
AS	= $(CC)
LD	= $(PREFIX)ld

ifeq ($(strip $(RASPPI)),1)
ARCH	?= -march=armv6k -mtune=arm1176jzf-s -marm -mfpu=vfp -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel
else ifeq ($(strip $(RASPPI)),2)
ARCH	?= -march=armv7-a -marm -mfpu=neon-vfpv4 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel7
else
ARCH	?= -march=armv8-a -mtune=cortex-a53 -marm -mfpu=neon-fp-armv8 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel8-32
endif

OPTIMIZE ?= -O2

AFLAGS	+= $(ARCH) -DRASPPI=$(RASPPI) $(INCLUDE)
CFLAGS	+= $(ARCH) -fsigned-char -ffreestanding -DRASPPI=$(RASPPI) $(INCLUDE) $(OPTIMIZE) -g

CFLAGS += -Iinclude --specs=rdimon.specs
AFLAGS += -Iinclude
CFLAGS += -I.
CFLAGS += -Wall -pedantic


%.o: %.S
	$(AS) $(AFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.a *.elf *.lst *.img *.hex *.cir *.map *~ $(EXTRACLEAN)
	$(RM) startup.o
	$(RM) sp/*.o

$(TARGET).hex: $(TARGET).img
	$(PREFIX)objcopy $(TARGET).elf -O ihex $(TARGET).hex

TOOLCHAIN_PATH=$(dir $(shell which $(CC)))/../
LIBS += $(TOOLCHAIN_PATH)/arm-none-eabi/lib/hard/libm.a
LIBS += $(TOOLCHAIN_PATH)/lib/gcc/arm-none-eabi/7.3.1/hard/libgcc.a
LIBS += $(TOOLCHAIN_PATH)/arm-none-eabi/lib/hard/libc.a

$(TARGET).img: $(OBJS) startup.o loader.ld
	$(LD) -o $(TARGET).elf -Map $(TARGET).map -T loader.ld startup.o $(OBJS) $(LIBS)
	#$(PREFIX)objdump -d $(TARGET).elf | $(PREFIX)c++filt > $(TARGET).lst
	$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).img
	wc -c $(TARGET).img
