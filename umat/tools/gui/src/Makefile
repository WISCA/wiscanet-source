PROG = gui_srv 
MNGDIR= ./mongoose-6.0
MNGSRC = $(MNGDIR)/mongoose.c

SOURCES = ui_srv.c ui_srv.h $(MNGSRC) msgdef.h

INSDIR = ../bin

CC = g++
CFLAGS = -W -Wall -DMG_ENABLE_SSL -lssl -lcrypto $(CFLAGS_EXTRA)

all: $(PROG)

$(PROG):
	$(CC) $(SOURCES) -o $@ $(CFLAGS) -I$(MNGDIR)
	cp $(PROG) $(INSDIR) 
	rm $(PROG)

clean:
	rm -rf $(PROG)
