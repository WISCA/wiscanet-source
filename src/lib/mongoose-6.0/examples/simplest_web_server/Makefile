PROG = simplest_web_server
SOURCES = $(PROG).c ../../mongoose.c
CFLAGS = -W -Wall -I../.. $(CFLAGS_EXTRA)

CROSS_COMPILE = arm-none-linux-gnueabi-
CC = gcc


all: $(PROG)

$(PROG): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(CFLAGS)

target:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(SOURCES) -o ARM-WSERVER

clean:
	rm -rf *.gc* *.dSYM *.exe *.obj *.o a.out $(PROG)

install:
	install -m 775 ARM-WSERVER /opt/openwrt_rx/ceva_libs/mongoose/
	install -m 775 ARM-WSERVER /opt/openwrt_jg/ceva_libs/mongoose/
