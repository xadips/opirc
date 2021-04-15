#include <sys/select.h>

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "arg.h"
#include "config.h"

#define MAX_NICK_LENGTH 32
#define MAX_BUFFER_LENGTH 4096
/* Actually it's 200 characters by RFC */
#define CHANNEL_NAME_LENGTH 256

char *argv0;
static char *host = DEFAULT_HOST;
static char *port = DEFAULT_PORT;
static char *password;
static char nick[MAX_NICK_LENGTH];
static char bufin[MAX_BUFFER_LENGTH];
static char bufout[MAX_BUFFER_LENGTH];
static char channel[CHANNEL_NAME_LENGTH];
static time_t trespond;
static FILE *srv;

#undef strlcpy
#include "strlcpy.c"
#include "util.c"

static void localout(char *channel, char *fmt, ...)
{
	static char timestr[80];
	time_t t;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufout, sizeof bufout, fmt, ap);
	va_end(ap);
	t = time(NULL);
	strftime(timestr, sizeof timestr, TIMESTAMP_FORMAT, localtime(&t));
	fprintf(stdout, "%-12s: %s %s\n", channel, timestr, bufout);
}

static void serverout(char *fmt, ...)
{
	/* use variable list and epsilon for multiple args 
	// argument params
	*/
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufout, sizeof bufout, fmt, ap);
	va_end(ap);
	fprintf(srv, "%s\r\n", bufout);
}

static void privatemsg(char *channel, char *msg)
{
	if (channel[0] == '\0')
	{
		localout("", "No channel to send to");
		return;
	}
	localout(channel, "<%s> %s", nick, msg);
	serverout("PRIVMSG %s :%s", channel, msg);
}

static void parselocal(char *s)
{
	char c, *p;

	if (s[0] == '\0')
		return;
	skip(s, '\n');
	if (s[0] != COMMAND_PREFIX_CHARACTER)
	{
		privatemsg(channel, s);
		return;
	}
	c = *++s;
	if (c != '\0' && isspace(s[1]))
	{
		p = s + 2;
		switch (c)
		{
			/* join server alias */
		case 'j':
			serverout("JOIN %s", p);
			if (channel[0] == '\0')
				strlcpy(channel, p, sizeof channel);

			return;
			/* leave channel alias */
		case 'l':
			s = eat(p, isspace, 1);
			p = eat(s, isspace, 0);
			if (!*s)
				s = channel;
			if (*p)
				*p++ = '\0';
			if (!*p)
				p = DEFAULT_PARTING_MESSAGE;
			serverout("PART %s :%s", s, p);

			return;
			/* message alias */
		case 'm':
			s = eat(p, isspace, 1);
			p = eat(s, isspace, 0);
			if (*p)
				*p++ = '\0';

			privatemsg(s, p);
			return;
			/* set default channel */
		case 's':
			strlcpy(channel, p, sizeof channel);
			return;
		}
	}
	serverout("%s", s);
}

static void parseserver(char *cmd)
{
	char *usr, *par, *txt;

	usr = host;
	if (!cmd || !*cmd)
		return;
	if (cmd[0] == ':')
	{
		usr = cmd + 1;
		cmd = skip(usr, ' ');
		if (cmd[0] == '\0')
			return;

		skip(usr, '!');
	}

	skip(cmd, '\r');
	par = skip(cmd, ' ');
	txt = skip(par, ':');

	trim(par);

	if (!strcmp("PONG", cmd))
		return;

	if (!strcmp("PRIVMSG", cmd))
		localout(par, "<%s> %s", usr, txt);

	else if (!strcmp("PING", cmd))
		serverout("PONG %s", txt);

	else
	{
		localout(usr, ">< %s (%s): %s", cmd, par, txt);

		if (!strcmp("NICK", cmd) && !strcmp(usr, nick))
			strlcpy(nick, txt, sizeof nick);
	}
}

static void usage(void)
{
	eprint("usage: opirc [-h host] [-p port] [-n nick] [-k keyword] [-v]\n", argv0);
}

int main(int argc, char *argv[])
{
	/* I guess there's a struct for time */
	struct timeval tv;
	const char *user = getenv("USER");
	/* my poor read descriptor */
	fd_set rd;

	strlcpy(nick, user ? user : "unknown", sizeof nick);

	/* messsy macro to manage arguments */
	ARGBEGIN
	{
	case 'h':
		host = EARGF(usage());
		break;
	case 'p':
		port = EARGF(usage());
		break;
	case 'n':
		strlcpy(nick, EARGF(usage()), sizeof nick);
		break;
	case 'k':
		password = EARGF(usage());
		break;
	case 'v':
		eprint("opirc - © 2021 Ričardas Čubukinas\n");
		break;
	default:
		usage();
	}
	ARGEND;

	/* init */
	srv = fdopen(connectserv(host, port), "r+");
	if (!srv)
		eprint("fdopen:");

	/* login */
	if (password)
		serverout("PASS %s", password);

	serverout("NICK %s", nick);
	serverout("USER %s localhost %s :%s", nick, host, nick);

	fflush(srv);

	/* clean buffers just in case */
	setbuf(stdout, NULL);
	setbuf(srv, NULL);
	setbuf(stdin, NULL);

	/* main client loop */
	for (;;)
	{
		/* We'll be reading stdin */
		FD_ZERO(&rd);
		FD_SET(0, &rd);
		FD_SET(fileno(srv), &rd);
		tv.tv_sec = 120;
		tv.tv_usec = 0;

		int n = select(fileno(srv) + 1, &rd, 0, 0, &tv);
		/* If selected server returns less than 0, well it's an error */
		if (n < 0)
		{
			if (errno == EINTR)
				continue;

			eprint("opirc: error on select():");
		}
		else if (n == 0)
		{
			if (time(NULL) - trespond >= 300)
				eprint("opirc shutting down: parse timeout\n");

			serverout("PING %s", host);
			continue;
		}
		if (FD_ISSET(fileno(srv), &rd))
		{
			if (fgets(bufin, sizeof bufin, srv) == NULL)
				eprint("opirc: remote host closed connection\n");

			parseserver(bufin);
			trespond = time(NULL);
		}
		if (FD_ISSET(0, &rd))
		{
			if (fgets(bufin, sizeof bufin, stdin) == NULL)
				eprint("opirc: broken pipe\n");

			parselocal(bufin);
		}
	}

	return 0;
}
