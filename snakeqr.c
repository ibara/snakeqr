/*
 * Copyright (c) 2020 Brian Callahan <bcallah@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * snakeqr -- game of snake that fits in a QR code
 */

// syscall table
#define SYS_EXIT 60
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_GETRANDOM 318
#define SYS_WAIT4 61
#define SYS_RT_SIGACTION 13
#define SYS_RT_SIGRETURN 15
#define SYS_EXECVE 59
#define SYS_FORK 57
#define SYS_SETITIMER 38

extern long _syscall(long number, ...);

struct timeval {
	long long tv_sec;
	long tv_usec;
};

struct itimerval {
	struct timeval it_interval;
	struct timeval it_value;
};

struct sigaction {
	void (*sa_handler)(int);
	unsigned long sa_flags;
	void (*sa_restorer) (void);
	unsigned long sa_mask;
};

static char direction;
static short location[23][79];

static char fruitx, fruity;
static char snakex, snakey;

static short snakelen;

static void
_exit(int status)
{
	_syscall(SYS_EXIT, status);
}

static long
read(int d, void *buf, unsigned long nbytes)
{
	return _syscall(SYS_READ, d, buf, nbytes);
}

static void
write(int d, const void *buf, unsigned long nbytes)
{
	_syscall(SYS_WRITE, d, buf, nbytes);
}

static void
getentropy(void *buf, unsigned long buflen)
{
	_syscall(SYS_GETRANDOM, buf, buflen, 0);
}

static long
wait4(int wpid, int *status, int options, void *rusage)
{
	return _syscall(SYS_WAIT4, wpid, status, options, rusage);
}

static void
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	_syscall(SYS_RT_SIGACTION, sig, act, oact, 8);  // sigactsize = sizeof(sigset_t)
}

static void
sigreturn(void)
{
	_syscall(SYS_RT_SIGRETURN);
}

static void
execve(const char *path, char *const argv[], char *const envp[])
{
	_syscall(SYS_EXECVE, path, argv, envp);
}

static long
vfork(void)
{
	return _syscall(SYS_FORK);
}

static void
setitimer(int which, const struct itimerval *value, struct itimerval *ovalue)
{
	_syscall(SYS_SETITIMER, which, value, ovalue);
}

static void
system(const char *command)
{
	char *argp[4] = { "/bin/sh", "-c", (void *) 0, (void *) 0 };
	int cpid, pid, pstat;

	argp[2] = (char *) command;

	switch ((cpid = vfork())) {
	case -1:
		_exit(1);
	case 0:
		execve("/bin/sh", argp, (void *) 0);
		_exit(127);
	}

	do {
		pid = wait4(cpid, &pstat, 0, (void *) 0);
	} while (pid == -1);
}

static void
timer(void (*cb)(int))
{
	struct sigaction action;

	action.sa_mask = 0;
	action.sa_flags = 0x04000000;   // SA_RESTORER
	action.sa_handler = cb;
	action.sa_restorer = &sigreturn;

	sigaction(14, &action, (void *) 0);
}

static void
fruitplace(void)
{
	char buf[2], xs[2], ys[2];
	int i, n;

again:
	getentropy(buf, 2);
	fruitx = buf[0];
	fruity = buf[1];

	if ((fruitx < 2 || fruitx > 78) || (fruity < 2 || fruity > 22) || location[fruity][fruitx] != 0)
		goto again;

	++snakelen;

	i = 0;
	n = fruitx;
	do {
		xs[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);

	i = 0;
	n = fruity;
	do {
		ys[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);

	write(1, "\033[", 2);
	if (fruity > 9)
		write(1, &ys[1], 1);
	write(1, &ys[0], 1);
	write(1, ";", 1);

	if (fruitx > 9)
		write(1, &xs[1], 1);
	write(1, &xs[0], 1);
	write(1, "H%", 2);
}

static void
checkcrash(void)
{
	char score[4];
	int i, n;

	if ((snakex < 2 || snakex > 78) || (snakey < 2 || snakey > 22) || (location[snakey][snakex] > 0 && location[snakey][snakex] < snakelen)) {
		system("stty sane");
		write(1, "\033[?25h", 6);
		write(1, "\033[2J\033[H", 7);

		i = 0;
		--snakelen;
		n = --snakelen;
		do {
			score[i++] = n % 10 + '0';
		} while ((n /= 10) > 0);

		write(1, "Score: ", 7);
		if (snakelen > 999)
			write(1, &score[3], 1);
		if (snakelen > 99)
			write(1, &score[2], 1);
		if (snakelen > 9)
			write(1, &score[1], 1);
		write(1, &score[0], 1);
		write(1, "\n", 1);

		_exit(0);
	}
}

static void
drawscr(void)
{
	char xs[2], ys[2];
	int i, n, x, y;

	for (y = 1; y < 23; y++) {
		for (x = 1; x < 79; x++) {
			if (location[y][x] > 0) {
				i = 0;
				n = x;
				do {
					xs[i++] = n % 10 + '0';
				} while ((n /= 10) > 0);

				i = 0;
				n = y;
				do {
					ys[i++] = n % 10 + '0';
				} while ((n /= 10) > 0);

				write(1, "\033[", 2);
				if (y > 9)
					write(1, &ys[1], 1);
				write(1, &ys[0], 1);
				write(1, ";", 1);

				if (x > 9)
					write(1, &xs[1], 1);
				write(1, &xs[0], 1);

				if (--(location[y][x]) == 0)
					write(1, "H ", 2);
				else
					write(1, "H@", 2);
			}
		}
	}
}

static void
move(void)
{

	switch (direction) {
	case 0:
		--snakey;
		break;
	case 1:
		++snakey;
		break;
	case 2:
		--snakex;
		break;
	case 3:
		++snakex;
	}

	checkcrash();

	if (snakex == fruitx && snakey == fruity)
		fruitplace();

	location[snakey][snakex] = snakelen;

	drawscr();
}

static void
do_move(int signo)
{
	static struct itimerval value;

	if (signo == 0)
		timer(do_move);

	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 100000;

	move();

	setitimer(0, &value, (void *) 0);
}

static void
snakeinit(void)
{
	char buf[2];

again:
	getentropy(buf, 2);
	snakex = buf[0];
	snakey = buf[1];

	if ((snakex < 8 || snakex > 72) || (snakey < 8 || snakey > 16))
		goto again;

	location[snakey][snakex] = snakelen;

	direction = 3;
}

static void
scrinit(void)
{
	int x, y;

	write(1, "\033[?25l", 6);
	write(1, "\033[2J\033[H", 7);

	write(1, " ", 1);
	for (x = 1; x < 79; x++)
		write(1, "-", 1);
	write(1, "\n", 1);

	for (y = 1; y < 22; y++) {
		write(1, "|", 1);
		for (x = 0; x < 78; x++)
			write(1, " ", 1);
		write(1, "|\n", 2);
	}

	write(1, " ", 1);
	for (x = 1; x < 79; x++)
		write(1, "-", 1);
}

void
main(void)
{
	int c;

	++snakelen;
	scrinit();

	system("stty cbreak -echo");

	snakeinit();
	fruitplace();

	drawscr();

	do_move(0);
	while (1) {
		c = 0;
		read(0, &c, 1);
		switch (c) {
			case 'h':
				direction = 2;
				break;
			case 'j':
				direction = 1;
				break;
			case 'k':
				direction = 0;
				break;
			case 'l':
				direction = 3;
		}
	}
}
