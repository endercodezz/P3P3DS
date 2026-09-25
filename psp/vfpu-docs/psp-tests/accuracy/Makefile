TARGET = vfpu-accuracy-test
OBJS = main.o ../test-utils.o

INCDIR =
CFLAGS = -Wall -O2 -I..
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
ifeq ($(BUILD_PRX),)
  CFLAGS += -D_BUILD_EBOOT_=1
endif

LIBDIR =
LDFLAGS =
LIBS= -lpsppower  -lm -lpspkubridge

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = VFPU accuracy tests

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

