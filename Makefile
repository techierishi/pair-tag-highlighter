CC=gcc

TARGET=pair_tag_highlighter
LIBDIR=/usr/lib/x86_64-linux-gnu/geany
SRCDIR=src
SOURCES=$(SRCDIR)/$(TARGET).c

#.PHONY: $(TARGET)
$(TARGET):
	$(CC) -c $(SOURCES) -fPIC $(shell pkg-config --cflags geany)
	$(CC) $@.o -o $@.so -shared $(shell pkg-config --libs geany)

install:
	cp $(TARGET).so $(LIBDIR)

deinstall:
	rm $(LIBDIR)/$(TARGET).so

clean:
	rm $(TARGET).o $(TARGET).so
