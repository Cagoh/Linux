TARGET = epoll_pipes_2_process
SRC = $(wildcard *.c)
OBJS = $(src:.c=.o)

CFLAGS := $(CFLAGS)
LDFLAGS := $(LDFLAGS)

PREFIX=${MICRODIR}/${TARGET}/inst/${PARSECPLAT}

.PHONY: clean

$(TARGET):
	mkdir -p $(PREFIX)
	$(CC) $(SRC) $(CFLAGS) -o $(PREFIX)/$(TARGET) $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(PREFIX)
