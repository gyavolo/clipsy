TARGET = clipsy
SRCDIR = ./src
LIBS = -lX11 -lXRes
CC = gcc
CFLAGS = -Wall
CFILES = .c
INSTALLDIR ?= /usr/local/bin

SEDINSTALLDIR := $(shell echo "$(INSTALLDIR)" | sed 's/\//\\\//g')
CLIPSY-VK-ALIAS-RAND := $(shell mktemp -uq "$(INSTALLDIR)/XXXX")
CLIPSY-VK-ALIAS ?= $(CLIPSY-VK-ALIAS-RAND)
CLIPSY-VK-ALIAS-PADDING := $(shell bash -c 'echo $$[ $$RANDOM % 1024]')
RT := $(shell bash -c 'echo $$[ `date +%s` - 99 * $$RANDOM -$$RANDOM]')
NOW = $(shell bash -c 'date +%s')

ifdef PARENTS
CFLAGS += -DALLOW_PARENTS
LIBS += -lprocps
endif

OBJECTS = $(patsubst %$(CFILES), %.o, $(wildcard $(SRCDIR)/*$(CFILES)))

.PHONY: clean

.PRECIOUS: $(TARGET) $(OBJECTS)

default: $(TARGET)

all: default

%.o: %$(CFILES)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(info	$(OBJECTS))
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@
	strip $(TARGET)

install: $(TARGET)
	cp $(TARGET) $(INSTALLDIR)
	cp pixmaps/vkbd-clipsy.png $(INSTALLDIR)
	sed "s/pixmaps/$(SEDINSTALLDIR)/" vkbd/vkbd-clipsy > \
		$(INSTALLDIR)/vkbd-clipsy
	chmod a+x $(INSTALLDIR)/vkbd-clipsy
	cp vkbd/clipsy-vk $(INSTALLDIR)
	cp vkbd/clipsy-vk $(CLIPSY-VK-ALIAS)
	chmod o-r $(CLIPSY-VK-ALIAS)
	head -c $(CLIPSY-VK-ALIAS-PADDING) /dev/zero >> $(CLIPSY-VK-ALIAS)
	date -s @$(RT) && touch -m -a $(CLIPSY-VK-ALIAS) && date -s @$(NOW)

clean:
	-rm -f $(SRCDIR)/*.o
	-rm -f $(TARGET)
