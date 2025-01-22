#TOOLCHAIN=~/toolchain/gcc-arm-none-eabi-4_9-2014q4/bin
#PREFIX=$(TOOLCHAIN)/arm-none-eabi-
PREFIX=arm-none-eabi-

BOARD=./BOARD
CMSIS=./include
DRIVERS=./drivers
UTILS=./utilities

ARCHFLAGS=-mthumb -mcpu=cortex-m0plus
COMMONFLAGS=-g3 -O0 -Wall $(ARCHFLAGS)

CFLAGS=-I$(BOARD) -I$(CMSIS) -I$(DRIVERS) -I$(UTILS) -DCPU_MKL46Z256VLL4 $(COMMONFLAGS)
LDFLAGS=$(ARCHFLAGS) --specs=nosys.specs -Wl,--gc-sections,-Map,$(TARGET).map,-TMKL46Z256xxx4_flash.ld
LDLIBS=

AS=$(PREFIX)as
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
OBJCOPY=$(PREFIX)objcopy
SIZE=$(PREFIX)size
RM=rm -f

TARGET=exam

SRC=$(wildcard *.c)
OBJ=$(patsubst %.c, %.o, $(SRC)) startup_MKL46Z4.o

BOARDSRC=$(wildcard $(BOARD)/*.c)
BOARDOBJ=$(patsubst %.c, %.o, $(BOARDSRC))

CMSISSRC=$(wildcard $(CMSIS)/*.c)
CMSISOBJ=$(patsubst %.c, %.o, $(CMSISSRC))

DRIVERSSRC=$(wildcard $(DRIVERS)/*.c)
DRIVERSOBJ=$(patsubst %.c, %.o, $(DRIVERSSRC))

UTILSSRC=$(wildcard $(UTILS)/*.c)
UTILSOBJ=$(patsubst %.c, %.o, $(UTILSSRC))

all: build size
build: elf srec bin
elf: $(TARGET).elf
srec: $(TARGET).srec
bin: $(TARGET).bin

clean:
	$(RM) $(TARGET).srec $(TARGET).elf $(TARGET).bin $(TARGET).map $(OBJ)\
	$(CMSISOBJ) $(DRIVERSOBJ) $(UTILSOBJ) $(BOARDOBJ) *~

$(TARGET).elf: $(OBJ) $(BOARDOBJ) $(CMSISOBJ) $(DRIVERSOBJ) $(UTILSOBJ)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.srec: %.elf
	$(OBJCOPY) -O srec $< $@

%.bin: %.elf
	    $(OBJCOPY) -O binary $< $@

size:
	$(SIZE) $(TARGET).elf

flash: all
	openocd -f openocd.cfg -c "program $(TARGET).elf verify reset exit"
