#!/bin/sh
set -eu

cd "$(dirname "$0")" || exit 1

${CC:-cc} ${CFLAGS:-} -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	*.c -o fish ${LDFLAGS:-} -lX11
