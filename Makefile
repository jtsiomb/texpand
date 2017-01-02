PREFIX = /usr/local

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = texpand

CFLAGS = -pedantic -Wall -I/usr/local/include -g -O3 -fopenmp
LDFLAGS = -L/usr/local/lib $(libgl) -lassimp -limago -lgomp -lpng -lz -ljpeg

ifeq ($(shell uname -s | sed 's/MINGW32.*/MINGW32/'), MINGW32)
	libgl = -lopengl32 -lgdi32
	CFLAGS += -DUSE_WGL
else
	libgl = -lGL -lX11
	CFLAGS += -DUSE_GLX
endif


$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: install
install: $(bin)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(bin) $(DESTDIR)$(PREFIX)/bin/$(bin)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(bin)
