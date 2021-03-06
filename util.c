#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
/* Sometimes vscode warns for not including va_list
// #include <stdarg.h>
*/

/* func to print errors */
static void eprint(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufout, sizeof bufout, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s", bufout);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s\n", strerror(errno));
	exit(1);
}

static int connectserv(char *host, char *port)
{
	static struct addrinfo hints;
	int serversocket;
	struct addrinfo *res, *r;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, port, &hints, &res) != 0)
		eprint("error: cannot resolve hostname '%s':", host);

	for (r = res; r; r = r->ai_next)
	{
		if ((serversocket = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1)
			continue;

		if (connect(serversocket, r->ai_addr, r->ai_addrlen) == 0)
			break;

		close(serversocket);
	}
	freeaddrinfo(res);

	if (!r)
		eprint("error: cannot connect to host '%s'\n", host);

	return serversocket;
}

static char *eat(char *s, int (*p)(int), int r)
{
	while (*s != '\0' && p(*s) == r)
		s++;
	return s;
}

static char *skip(char *s, char c)
{
	while (*s != c && *s != '\0')
		s++;
	if (*s != '\0')
		*s++ = '\0';
	return s;
}

static void trim(char *s)
{
	char *e;

	e = s + strlen(s) - 1;
	while (isspace(*e) && e > s)
		e--;
	*(e + 1) = '\0';
}
