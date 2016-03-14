# 
# Makefile for candle-hack
#
# You must set the MCU below. Also, make sure the msp430-* tools are in your path.
# Do 'make install' to flash the device

MCU      = msp430g2231

PROG	 = main
CC       = msp430-gcc
LD       = msp430-ld
AR       = msp430-ar
AS       = msp430-gcc
GASP     = msp430-gasp
NM       = msp430-nm
OBJCOPY  = msp430-objcopy
OBJDUMP  = msp430-objdump
SIZE	 = msp430-size
MSPDEBUG = mspdebug
CFLAGS   = -std=gnu99 -Os -Wall -g -mmcu=$(MCU)
LDFLAGS  = -mmcu=$(MCU) -Wl,-Map=$(TARGET).map

SOURCES=main.c
DEPEND = $(SOURCES:.c=.d)
OBJS = $(SOURCES:.c=.o)

all: $(PROG).elf $(PROG).lst
	 $(SIZE) $(PROG).elf

.PHONY: all

$(PROG).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).elf $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.lst: %.elf
	$(OBJDUMP) -DS $< >$@

clean:
	rm -rf $(PROG).elf $(PROG).lst $(OBJS)

install:
	mspdebug rf2500 "prog ./$(PROG).elf"

