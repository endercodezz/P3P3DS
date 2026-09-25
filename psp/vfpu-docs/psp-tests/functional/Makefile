include Makefile.auto

TARGET = vfpu-test
OBJS = main.o $(AUTO_TEST_OBJS) ../test-utils.o

INCDIR = ../ ../..
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
PSP_EBOOT_TITLE = PSP VFPU function validation suite

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

