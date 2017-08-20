TARGET = camera_patch_lite
OBJS = main.o minini/minIni.o

INCDIR = include
MININI_DEFINES = -DNDEBUG -DINI_FILETYPE=SceUID -DPORTABLE_STRNICMP -DINI_NOFLOAT
CFLAGS = -O2 -Os -G0 -Wall -fshort-wchar -fno-pic -mno-check-zero-division $(MININI_DEFINES)
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = 
LDFLAGS =

BUILD_PRX = 1

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak