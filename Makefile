CC = clang
CFLAGS = -std=c99 -g3 -Wall -Wextra -Werror
LLIBS = -lglfw
IDIR = -Ithird_party/glad/include

bin/main: src/main.c bin/glad.o | bin
	${CC} ${CFLAGS} ${LLIBS} ${IDIR} src/main.c bin/glad.o -o bin/main

bin/glad.o: third_party/glad/src/glad.c | bin
	${CC} -c third_party/glad/src/glad.c -o bin/glad.o -Ithird_party/glad/include

bin:
	mkdir -p bin

run: bin/main
	bin/main
