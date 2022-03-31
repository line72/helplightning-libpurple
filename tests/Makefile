C_SRCS = check_libhelplightning.c \
	test_rcl.c \
	test_cmf.c \
	test_ballyhoo_message.c \
	test_ballyhoo_deflate.c \
	test_ballyhoo_xml.c


# Object file names using 'Substitution Reference'
C_OBJS = $(C_SRCS:.c=.o)

PURPLE_MOD=purple

APPNAME=check_libhelplightning

CC=gcc
PKGS=$(PURPLE_MOD) glib-2.0 gobject-2.0 xmlrpc xmlrpc_util check

CFLAGS = \
    -g \
    -O2 \
    -Wall \
    -Wno-error=strict-aliasing \
    -Wno-deprecated-declarations \
    -fPIC \
    -D_DEFAULT_SOURCE=1 \
    -D_XOPEN_SOURCE=1 \
    -std=c99 \
    $(shell pkg-config --cflags $(PKGS)) \
    $(LOCAL_CFLAGS)

LIBS = $(shell pkg-config --libs $(PKGS)) -L../ -lhelplightning


.PHONY: all
all: $(APPNAME)

LDFLAGS = 

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
%.E: %.c
	$(CC) -E $(CFLAGS) -o $@ $<

$(APPNAME): $(C_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f *.o $(APPNAME) Makefile.dep

Makefile.dep: $(C_SRCS)
	$(CC) -MM $(CFLAGS) $^ > Makefile.dep

include Makefile.dep
