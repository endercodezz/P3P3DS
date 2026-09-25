TARGET = vfpu-regcompat
OBJS = main.o autogen-regtests.o ../test-utils.o

INCDIR =
CFLAGS = -Wall -O2 -I..
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
ifeq ($(BUILD_PRX),)
  CFLAGS += -D_BUILD_EBOOT_=1
endif

LIBDIR =
LDFLAGS =
LIBS= -lm -lpsppower -lpspkubridge

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = VFPU regcompat test

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

