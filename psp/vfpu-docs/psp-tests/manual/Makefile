TARGET = vfpu-manual-tests
OBJS = main.o ../test-utils.o test-conditional.o ieee754-fun.o vfpu-cc.o vfpu-bugs.o memops.o memops-helper.o mips-allegrex.o fp-unit.o undefined-behaviour.o

INCDIR = ../
CFLAGS = -Wall -ggdb -O2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
ifeq ($(BUILD_PRX),)
  CFLAGS += -D_BUILD_EBOOT_=1
endif

LIBDIR =
LDFLAGS =
LIBS= -lpspgum -lpspgu -lm -lpspfpu -lpsppower -lpspkubridge

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP VFPU manual tests

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

