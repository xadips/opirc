# opirc - simple irc client

include config.mk

SRC = opirc.c
OBJ = ${SRC:.c=.o}

all: options opirc

options:
	@echo opirc build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk strlcpy.c util.c

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

opirc: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f opirc ${OBJ} opirc-${VERSION}.tar.gz
	@rm -f config.h

dist: clean
	@echo creating dist tarball
	@mkdir -p opirc-${VERSION}
	@cp -R LICENSE Makefile README arg.h config.def.h config.mk opirc.1 opirc.c util.c strlcpy.c opirc-${VERSION}
	@tar -cf opirc-${VERSION}.tar opirc-${VERSION}
	@gzip opirc-${VERSION}.tar
	@rm -rf opirc-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f opirc ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/opirc
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < opirc.1 > ${DESTDIR}${MANPREFIX}/man1/opirc.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/opirc.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/opirc
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/opirc.1

.PHONY: all options clean dist install uninstall
