CC=gcc
CFLAGS=-Wall -std=c99 -O2 -g -fPIC
PKGCONFIG=pkg-config
LIBS=$(shell $(PKGCONFIG) libnautilus-extension --libs)
INCS=$(shell $(PKGCONFIG) libnautilus-extension --cflags libxml-2.0 libzip)

ndt: nautilus-extension-mtime.c
	$(CC) ${CFLAGS} -c nautilus-extension-epub.c -o nautilus-extension-epub.o ${INCS}
	$(CC) -shared nautilus-extension-epub.o -o nautilus-extension-epub.so -lzip ${LIBS}

clean:
	rm -f *.o *.so

install:
	cp fb2-extension.so /usr/lib/nautilus/extensions-3.0
	
uninstall:
	rm -f /usr/lib/nautilus/extensions-3.0/nautilus-extension-epub.so

replace:
	rm -f /usr/lib/nautilus/extensions-3.0/nautilus-extension-epub.so
	cp nautilus-extension-epub.so /usr/lib/nautilus/extensions-3.0

debug:
	nautilus -q && nautilus --browser
