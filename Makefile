# snakeqr Makefile

CC =		clang
CFLAGS =	-Oz -nostdinc -ffreestanding
CFLAGS +=	-fno-PIE -fno-PIC -fomit-frame-pointer
CFLAGS +=	-fno-stack-protector -mno-retpoline

PROG =	snakeqr
OBJS =	crt.o snakeqr.o

all: ${OBJS}
	/usr/bin/ld -nopie -o ${PROG} ${OBJS}
	/usr/bin/strip ${PROG}
	/usr/bin/strip -R .comment ${PROG}

compress:
	/usr/bin/gzexe ${PROG}

qr:
	qrencode -r ${PROG} -8 -o ${PROG}.png

clean:
	rm -rf ${PROG} ${OBJS} ${PROG}.core ${PROG}~
