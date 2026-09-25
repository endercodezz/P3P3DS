TARGET = exception_module
OBJS = exmod.o exhndlr.o

# This is a kernel PRX
BUILD_PRX=1
PRX_EXPORTS=exports.exp

# Use only kernel libraries
USE_KERNEL_LIBS = 1
USE_KERNEL_LIBC = 1

CFLAGS = -G0 -Os -Wall -nostartfiles
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LDFLAGS=-nodefaultlibs

LIBDIR =

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

