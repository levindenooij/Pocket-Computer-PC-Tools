INSTALL = install
RM = rm
PREFIX = /usr/local
VERSION = 0.1

all: build

build: 
	$(MAKE) -C ../Sources

installdirs:
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -d $(DESTDIR)/etc

install: build installdirs
	$(INSTALL) ../Sources/bas2img $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) ../Sources/bin2wav $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) ../Sources/wav2bin $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) cload $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) csave $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 644 sharppc.cfg $(DESTDIR)/etc/

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/bas2img
	$(RM) $(DESTDIR)$(PREFIX)/bin/bin2wav
	$(RM) $(DESTDIR)$(PREFIX)/bin/wav2bin
	$(RM) $(DESTDIR)$(PREFIX)/bin/csave
	$(RM) $(DESTDIR)$(PREFIX)/bin/cload
	$(RM) $(DESTDIR)/etc/sharppc.cfg

clean:
	$(RM) ../Sources/bas2img
	$(RM) ../Sources/bin2wav
	$(RM) ../Sources/wav2bin


