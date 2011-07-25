ARCH=$(shell isainfo -k)
CC=gcc
LD=ld

CFLAGS=-ffreestanding -nodefaultlibs -I../include -I. -D_KERNEL -DSOLARIS10 -DNEVADA -DNEVADATEST -DDEBUG -DDEBUGINPUT
LDFLAGS=-r -dy -N misc/hidparser -N misc/usba

ifeq ($(ARCH), amd64)
CFLAGS+=-m64 -mcmodel=kernel -mno-red-zone
KERNEL_DIR=/usr/kernel/drv/amd64
else ifeq ($(ARCH), i386)
KERNEL_DIR=/usr/kernel/drv
endif


.PHONY: all

all: wacom

wacom: wacom.o
	$(LD) $(LDFLAGS) -o wacom wacom.o

wacom.o: wacom.c
	$(CC) $(CFLAGS) -c wacom.c


########
# Original targets from Max
#
#all: wacom
#
#wacom: wacom.o
#	ld -r -dy -N misc/hidparser -N misc/usba wacom.o -o wacom
#
#wacom.o: wacom.c wacom.h
#	cc -DSOLARIS10 -D_KERNEL -DNEVADATEST -DDEBUG -DDEBUGINPUT -c -I . wacom.c
#	cc -m64 -xmodel=kernel -DNEVADA -DSOLARIS10 -D_KERNEL -c -I . -I ../include -I /wd320/max/onnv/usr/src/uts/common/sys/usb/clients/hidparser -I /wd320/max/onnv/usr/src/uts/common  wacom.c


