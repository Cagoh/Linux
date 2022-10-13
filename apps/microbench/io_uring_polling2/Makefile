TARGET = io_uring_polling2
SRC = $(wildcard *.c) \
	  $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)

CFLAGS := $(CFLAGS) -pthread
LDFLAGS := $(LDFLAGS)

PREFIX=${MICRODIR}/${TARGET}/inst/${PARSECPLAT}

MACHINE = $(shell uname -m)


#ifeq ($(MACHINE), x86_64)
#LIBS := $(LIBS) ${MICRODIR}/pkgs/io_uring_libs/x86/
#LIBS := $(LIBS) ${MICRODIR}/pkgs/io_uring_libs/x86/liburing.so.2
#LIBS := $(LIBS) /lib/liburing.so.2
#else
#LIBS := $(LIBS) ${MICRODIR}/pkgs/io_uring_libs/aarch64/liburing.so.2
#LIBS := $(LIBS) ${MICRODIR}/pkgs/io_uring_libs/aarch64/
#endif

#ifeq ($(MACHINE), x86_64)
#LIBS := /usr/lib/liburing.a
#else
#LIBS := /usr/lib/liburing.a
#endif

.PHONY: clean

$(TARGET):
	mkdir -p $(PREFIX)
	$(CC) $(SRC) $(CFLAGS) -o $(PREFIX)/$(TARGET) $(LDFLAGS)
	rm -rf $(DEP) $(OBJ)



clean:
	rm -rf $(PREFIX) $(DEP) $(OBJ)


