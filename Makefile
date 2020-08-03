# snakeqr Makefile

CC =		cc
CFLAGS =	-Oz -nostdinc -ffreestanding -fdata-sections -ffunction-sections
CFLAGS +=	-fno-PIE -fno-PIC -fno-ret-protector -fomit-frame-pointer
CFLAGS +=	-fno-stack-protector -mno-retpoline
CFLAGS +=	-Wno-int-to-void-pointer-cast

PROG =	snakeqr
OBJS =	crt.o snakeqr.o

all: ${OBJS}
	/usr/bin/ld -nopie --gc-sections -o ${PROG} ${OBJS}
	/usr/bin/strip ${PROG}
	/usr/bin/strip -R .comment ${PROG}
	/usr/bin/gzexe ${PROG}

qr:
	qrencode -r ${PROG} -8 -o ${PROG}.png

clean:
	rm -rf ${PROG} ${OBJS} ${PROG}.core ${PROG}~
