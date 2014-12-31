TARGET  := sdmenu
SCRIPTS := sdmenu_run sdmenu_nav
CFLAGS  ?= -Os -pipe -march=native
LDFLAGS ?= -s
PREFIX  ?= /usr/local
DESTDIR ?= bin

all: $(TARGET)

install: $(TARGET) $(SCRIPTS)
	$(foreach I, $^, install -D $(I) $(PREFIX)/$(DESTDIR)/$(I);)
