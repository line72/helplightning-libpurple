C_SRCS_S = libhelplightning.c \
	libcmf.c \
	librcl.c \
	libballyhoo.c \
	libballyhoo_xml.c \
	libballyhoo_deflate.c \
	libballyhoo_message.c \
	libballyhoo_deferred.c \
	libgaldr.c \
	libgaldr_auth.c \
	libgaldr_contact.c \
	libgaldr_handler.c \
	libgaldr_internal.c \
	libgaldr_messaging.c \
	libgaldr_responses.c \
	libgaldr_session.c \
	libgaldr_signals.c \
	libgaldr_utils.c \
	libgaldr_workspace.c
C_SRCS=$(patsubst %.c,src/%.c,${C_SRCS_S})
# Object file names using 'Substitution Reference'
C_OBJS = $(C_SRCS:.c=.o)

PURPLE_MOD=purple

ifeq ($(OS),Windows_NT)

LIBNAME=libhelplightning.dll
PIDGIN_TREE_TOP ?= ../pidgin-2.10.11
WIN32_DEV_TOP ?= $(PIDGIN_TREE_TOP)/../win32-dev
WIN32_CC ?= $(WIN32_DEV_TOP)/mingw-4.7.2/bin/gcc

PROGFILES32=${ProgramFiles(x86)}
ifndef PROGFILES32
PROGFILES32=$(PROGRAMFILES)
endif

CC = $(WIN32_DEV_TOP)/mingw-4.7.2/bin/gcc

DATA_ROOT_DIR_PURPLE:="$(PROGFILES32)/Pidgin"
PLUGIN_DIR_PURPLE:="$(DATA_ROOT_DIR_PURPLE)/plugins"
CFLAGS = \
    -g \
    -O2 \
    -Wall \
    -D_DEFAULT_SOURCE=1 \
    -D_XOPEN_SOURCE=1 \
    -std=c99 \
	-I$(PIDGIN_TREE_TOP)/libpurple \
	-I$(WIN32_DEV_TOP)/glib-2.28.8/include -I$(WIN32_DEV_TOP)/glib-2.28.8/include/glib-2.0 -I$(WIN32_DEV_TOP)/glib-2.28.8/lib/glib-2.0/include
LIBS = -L$(WIN32_DEV_TOP)/glib-2.28.8/lib -L$(PIDGIN_TREE_TOP)/libpurple -lpurple -lintl -lglib-2.0 -lgobject-2.0 -g -ggdb -static-libgcc -lz -lws2_32 

else

LIBNAME=libhelplightning.so

CC=gcc
PLUGIN_DIR_PURPLE:=$(DESTDIR)$(shell pkg-config --variable=plugindir $(PURPLE_MOD))
DATA_ROOT_DIR_PURPLE:=$(DESTDIR)$(shell pkg-config --variable=datarootdir $(PURPLE_MOD))
PKGS=$(PURPLE_MOD) glib-2.0 gobject-2.0 xmlrpc xmlrpc_util

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

LIBS = $(shell pkg-config --libs $(PKGS)) -lz

endif

.PHONY: all
all: $(LIBNAME)

LDFLAGS = -shared

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
%.E: %.c
	$(CC) -E $(CFLAGS) -o $@ $<

$(LIBNAME): $(C_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: install install-user
install: $(LIBNAME)
	install -d $(PLUGIN_DIR_PURPLE)
	install $(LIBNAME) $(PLUGIN_DIR_PURPLE)
	for z in 16 22 48 ; do \
		install -d $(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/$$z ; \
		install -m 0644 img/helplightning$$z.png $(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/$$z/helplightning.png ; \
	done

install-user: $(LIBNAME)
	install -d $(HOME)/.purple/plugins
	install $(LIBNAME) $(HOME)/.purple/plugins

.PHONY: uninstall
uninstall: $(LIBNAME)
	rm $(PLUGIN_DIR_PURPLE)/$(LIBNAME)
	rm $(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/16/helplightning.png
	rm $(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/22/helplightning.png
	rm $(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/48/helplightning.png

.PHONY: clean
clean:
	rm -f *.o $(LIBNAME) Makefile.dep

.PHONY: modversion
modversion:
	pkg-config --modversion $(PKGS)

Makefile.dep: $(C_SRCS)
	$(CC) -MM $(CFLAGS) $^ > Makefile.dep

include Makefile.dep
