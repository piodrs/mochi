CC ?= gcc
CLANG_FORMAT ?= clang-format
CFLAGS ?=
LDLIBS ?= -lX11

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
C99FLAGS = -std=c99 -pedantic-errors -Wall -Wextra -Werror \
	-Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wshadow

.PHONY: all format

all: mochi

mochi: $(SOURCES) $(HEADERS) Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) $(C99FLAGS) $(SOURCES) -o $@ $(LDFLAGS) $(LDLIBS)

format:
	$(CLANG_FORMAT) -i --style=file $(SOURCES) $(HEADERS)
