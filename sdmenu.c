/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Ricardo Liang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define TEXT_NORMAL "\x1b(B\x1b[m"
#define TEXT_BOLD "\x1b[1m"
#define GO_UP "\x1b[A"
#define GO_LEFT "\x08"
#define CLEAR_SCREEN "\x1b[J"

struct {
	size_t lines;
	size_t width;
	const char *selected;
} PREFS = { 2, 82, TEXT_BOLD };

struct {
	struct entry {
		const char *str;
		size_t len;
		int pos;
	} *data;
	size_t len;
	size_t matches;
	size_t current;
} ENTRIES = { NULL, 0, 0, 0 };

struct {
	char str[BUFSIZ];
	size_t len;
} INPUT = { '\0', 0 };

struct {
	size_t cols;
	size_t used_rows;
} SCREEN = { 0, 0 };

int entry_compare(const struct entry *e1, const struct entry *e2)
{
	return e1->pos < e2->pos ? -1 : e1->pos > e2->pos ? 1 :
		e1->len < e2->len ? -1 : e1->len > e2->len ? 1 : 0;
}

void entry_swap(struct entry *e1, struct entry *e2)
{
	struct entry tmp = *e1;
	*e1 = *e2;
	*e2 = tmp;
}

void entry_update(struct entry *e)
{
	const char *p = INPUT.str[0] == '\0' ?
		e->str : strstr(e->str, INPUT.str);
	if ((e->pos = p != NULL ? p - e->str : -1) != -1)
		entry_swap(e, &ENTRIES.data[ENTRIES.matches++]);
}

void entries_filter(void)
{
	ENTRIES.matches = 0;
	size_t i;
	for (i = 0; i < ENTRIES.len; i++)
		entry_update(&ENTRIES.data[i]);
	qsort(ENTRIES.data, ENTRIES.matches, sizeof(struct entry),
			(int (*)(const void *, const void *)) entry_compare);
}

void entry_snprint(char **str, size_t size, size_t i)
{
	if (size > PREFS.width)
		size = PREFS.width;
	size_t all = i != ENTRIES.current ?
		snprintf(*str, size, "%s ",
				ENTRIES.data[i].str) :
		snprintf(*str, size, "%s%s" TEXT_NORMAL " ",
				PREFS.selected, ENTRIES.data[i].str);
	*str += all < size ? all : size;
}

void entries_init(struct entry *buf, int argc, char *argv[])
{
	ENTRIES.data = buf;
	ENTRIES.matches = ENTRIES.len = argc;
	size_t i = 0;
	for (i = 0; i < ENTRIES.len; ++i)
		ENTRIES.data[i] = (struct entry)
		{ argv[i], strlen(argv[i]), -1 };
}

void screen_clear(void)
{
	int i;
	for (i = 0; i < SCREEN.used_rows + 1; ++i)
		fputs(GO_UP, stderr);
	for (i = 0; i < SCREEN.cols; ++i)
		fputs(GO_LEFT, stderr);
	fputs(CLEAR_SCREEN, stderr);
}

void screen_refresh(void)
{
	char str[SCREEN.cols * PREFS.lines], *end = str + sizeof(str), *k = str;
	str[0] = '\0';
	int i;
	for (i = 0; i < ENTRIES.matches && k < end; i++)
		entry_snprint(&k, end - k, i);
	fprintf(stderr, "%s\n%s" TEXT_NORMAL, INPUT.str, str);
	SCREEN.used_rows = (k - str) / (SCREEN.cols + 1);
}

void screen_update(void)
{
	struct winsize s;
	ioctl(STDERR_FILENO, TIOCGWINSZ, &s);
	SCREEN.cols = s.ws_col + sizeof(TEXT_NORMAL) - 2;
}

void screen_init(void)
{
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ICANON & ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
	signal(SIGWINCH, (void (*)(int)) screen_update);
	screen_update();
}

void prefs_init(void)
{
	char *v;
	if ((v = getenv("SDMENU_LINES")) != NULL)
		PREFS.lines = atoi(v);
	if ((v = getenv("SDMENU_WIDTH")) != NULL)
		PREFS.width = atoi(v);
	if ((v = getenv("SDMENU_SELECTED")) != NULL)
		PREFS.selected = v;
}

void output(void)
{
	fputs("\n", stderr);
	puts(ENTRIES.current < ENTRIES.matches ?
			ENTRIES.data[ENTRIES.current].str : INPUT.str);
}

void interpret(void)
{
	char c = getchar();
	switch (c) {
	case '\t':
		ENTRIES.current++;
		break;
	case '\b':
	case '\x7f':
		INPUT.str[INPUT.len = 0] = '\0';
		ENTRIES.current = 0;
		break;
	case '\n':
		output();
	case '\x1b':
		exit(EXIT_SUCCESS);
	default:
		INPUT.str[INPUT.len++] = c;
		INPUT.str[INPUT.len] = '\0';
	}
}

void init(struct entry *buf, int argc, char *argv[])
{
	entries_init(buf, argc, argv);
	prefs_init();
	screen_init();
}

void main(int argc, char *argv[])
{
	struct entry buf[argc - 1];
	init(buf, argc - 1, argv + 1);
	for (;;) {
		screen_refresh();
		interpret();
		entries_filter();
		screen_clear();
	}
}

/* vim: set ts=8 sw=8 noet: */
