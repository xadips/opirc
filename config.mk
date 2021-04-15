# opirc version
VERSION = 0.2

# Paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# Includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc

# Flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# Set compiler
CC = cc
