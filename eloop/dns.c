#ifndef WIN32

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <resolv.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/nameser.h>
#include <sys/utsname.h>
#include <sys/un.h>
#endif

#ifndef __ACN_AP__
#define __ACN_AP__
#endif

/*
 * jphwang--(2010/04/09) TODO: OK
 */
#ifdef __ACN_AP__
#include"build_config.h"
#if 1//YWHWANGPATCH_MULTIBSS_ACN
#include"eloop.h"  
#endif
#if 1//for hywmon
	#ifdef __UCLIBC_HAS_THREADS__
	# include <pthread.h>
	#endif
void    check_hostapd_alive2(char* if_name){}
	# define BIGLOCK
	# define BIGUNLOCK

	# define LOCK
	# define UNLOCK
#else
void    check_hostapd_alive2(char* if_name);
	#ifdef __UCLIBC_HAS_THREADS__
	# include <pthread.h>
	static pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t __resolv_lock = PTHREAD_MUTEX_INITIALIZER;
	#endif
	#define LOCK	pthread_mutex_lock(&mylock)
	#define UNLOCK	pthread_mutex_unlock(&mylock)

	#define BIGLOCK	pthread_mutex_lock(&__resolv_lock)
	#define BIGUNLOCK	pthread_mutex_unlock(&__resolv_lock)
#endif
#else /* original */
	# define BIGLOCK
	# define BIGUNLOCK

	# define LOCK
	# define UNLOCK
#endif /* __ACN_AP__ */

#define MAX_RECURSE 5
#define REPLY_TIMEOUT 5	// 20101021 hongki99.kim : Set Timeout 5sec
#define MAX_RETRIES 1
#define MAX_SERVERS 3
#define MAX_SEARCH 4

#define MAX_ALIASES	5

/* 1:ip + 1:full + MAX_ALIASES:aliases + 1:NULL */
#define 	ALIAS_DIM		(2 + MAX_ALIASES + 1)

int g_bPrintDNStimeout=0;
//#undef DEBUG
#define DEBUG	0
#ifdef WIN32
#define DPRINTF(X,args)
#define DPRINTF1(X,args) 
#else
#if DEBUG
#define DPRINTF(X,args...) fprintf(stderr, X, ##args);fflush(stderr)
#define DPRINTF1(X,args...) fprintf(stderr, X, ##args);fflush(stderr)
#else
#define DPRINTF(X,args...)
#define DPRINTF1(X,args...) fprintf(stderr, X, ##args);fflush(stderr)
#endif /* DEBUG */
#endif

/* Structs */
struct resolv_header {
	int id;
	int qr,opcode,aa,tc,rd,ra,rcode;
	int qdcount;
	int ancount;
	int nscount;
	int arcount;
};

struct resolv_question {
	char * dotted;
	int qtype;
	int qclass;
};

struct resolv_answer {
	char * dotted;
	int atype;
	int aclass;
	int ttl;
	int rdlength;
	unsigned char * rdata;
	int rdoffset;
	char* buf;
	size_t buflen;
	size_t add_count;
};

enum etc_hosts_action {
    GET_HOSTS_BYNAME = 0,
    GETHOSTENT,
    GET_HOSTS_BYADDR,
};

/*
 * jphwang++(2010/04/09) TODO: OK
 *  ; uClibc의 특수목적 내부 함수들(예: gethostbyname()에서 사용하는 내부루틴 ,....) 은 GCC 3.3에서는
 *    컴파일 지시어인 "__attribute__ ((visibility ("hidden")))" 를 이용하여 해당 루틴의 Symbol을 공유 라이브러리
 *    에서 삭제하여 다른 object에서 참조를 할 수 없도록 기능을 수행한다.
 *    # define attribute_hidden __attribute__ ((visibility ("hidden")))
 *
 *    즉, 아래의 코드중 uClibc내의 internal symbol인 다음 list들은 undefined symbol로 Error를 유발한다.
 *     __searchdomains,
 *     __searchdomain,
 *     __encode_question,
 *     __length_question,
 *     __decode_answer,
 *     __searchdomains,
 *     __open_nameservers,
 *     __nameservers,
 *     __nameserver
 *    현재는 해당 symbol을 직접 Link할 방법이 없기 때문에 해당 코드를 떼어서 삽입한다.
 *    (추후 아래의 gethostbyname_ext()를 따로 만들어 사용한 사용한 이유를 알아야 한다.
 */
#ifdef __ACN_AP__
# define attribute_hidden
int attribute_hidden __encode_dotted(const char *dotted, unsigned char *dest, int maxlen)
{
	int used = 0;

	while (dotted && *dotted) {
		char *c = strchr(dotted, '.');
		int l = c ? c - dotted : strlen(dotted);

		if (l >= (maxlen - used - 1))
			return -1;

		dest[used++] = l;
		memcpy(dest + used, dotted, l);
		used += l;

		if (c)
			dotted = c + 1;
		else
			break;
	}

	if (maxlen < 1)
		return -1;

	dest[used++] = 0;

	return used;
}

int attribute_hidden __decode_dotted(const unsigned char *data, int offset,
				  char *dest, int maxlen)
{
	int l;
	int measure = 1;
	int total = 0;
	int used = 0;

	if (!data)
		return -1;

	while ((l=data[offset++]))
	{
		if (measure)
		    total++;
		if ((l & 0xc0) == (0xc0))
		{
			if (measure)
				total++;
			/* compressed item, redirect */
			offset = ((l & 0x3f) << 8) | data[offset];
			measure = 0;
			continue;
		}

		if ((used + l + 1) >= maxlen)
			return -1;

		memcpy(dest + used, data + offset, l);
		offset += l;
		used += l;
		if (measure)
		{
			total += l;
		}

		if (data[offset] != 0)
		{
			dest[used++] = '.';
		}
		else
		{
			dest[used++] = '\0';
		}
	}

	/* The null byte must be counted too */
	if (measure)
	{
	    total++;
	}

	DPRINTF("Total decode len = %d\n", total);

	return total;
}

int attribute_hidden __encode_question(struct resolv_question *q,
					unsigned char *dest, int maxlen)
{
	int i;

	i = __encode_dotted(q->dotted, dest, maxlen);
	if (i < 0)
		return i;

	dest += i;
	maxlen -= i;

	if (maxlen < 4)
		return -1;

	dest[0] = (q->qtype & 0xff00) >> 8;
	dest[1] = (q->qtype & 0x00ff) >> 0;
	dest[2] = (q->qclass & 0xff00) >> 8;
	dest[3] = (q->qclass & 0x00ff) >> 0;

	return i + 4;
}

int attribute_hidden __length_dotted(const unsigned char *data, int offset)
{
	int orig_offset = offset;
	int l;

	if (!data)
		return -1;

	while ((l = data[offset++])) {

		if ((l & 0xc0) == (0xc0)) {
			offset++;
			break;
		}

		offset += l;
	}

	return offset - orig_offset;
}

int attribute_hidden __length_question(unsigned char *message, int offset)
{
	int i;

	i = __length_dotted(message, offset);
	if (i < 0)
		return i;

	return i + 4;
}

int attribute_hidden __decode_answer(unsigned char *message, int offset,
				  struct resolv_answer *a)
{
	char temp[256];
	int i;

	i = __decode_dotted(message, offset, temp, sizeof(temp));
	if (i < 0)
		return i;

	message += offset + i;

	a->dotted = strdup(temp);
	a->atype = (message[0] << 8) | message[1];
	message += 2;
	a->aclass = (message[0] << 8) | message[1];
	message += 2;
	a->ttl = (message[0] << 24) |
		(message[1] << 16) | (message[2] << 8) | (message[3] << 0);
	message += 4;
	a->rdlength = (message[0] << 8) | message[1];
	message += 2;
	a->rdata = message;
	a->rdoffset = offset + i + RRFIXEDSZ;

	DPRINTF("i=%d,rdlength=%d\n", i, a->rdlength);

	return i + RRFIXEDSZ + a->rdlength;
}


int __nameservers;
char * __nameserver[MAX_SERVERS];
int __searchdomains;
char * __searchdomain[MAX_SEARCH];

/*
 *	we currently read formats not quite the same as that on normal
 *	unix systems, we can have a list of nameservers after the keyword.
 */
int __nameserver_exist(int serverChoice)
{
	return __nameserver[serverChoice-1]&&strlen(__nameserver[serverChoice-1])>=7? 1:0;
}
int attribute_hidden __open_nameservers()
{
	FILE *fp;
	int i;
#define RESOLV_ARGS 5
	char szBuffer[128], *p, *argv[RESOLV_ARGS];
	int argc;

	BIGLOCK;
#if 0
	if (__nameservers > 0) {
	    BIGUNLOCK;
	    return 0;
	}
#else
	__nameservers = 0;
#endif

	if ((fp = fopen("/etc/resolv.conf", "r")) ||
			(fp = fopen("/etc/config/resolv.conf", "r")))
	{

		while (fgets(szBuffer, sizeof(szBuffer), fp) != NULL) {

			for (p = szBuffer; *p && isspace(*p); p++)
				/* skip white space */;
			if (*p == '\0' || *p == '\n' || *p == '#') /* skip comments etc */
				continue;
			argc = 0;
			while (*p && argc < RESOLV_ARGS) {
				argv[argc++] = p;
				while (*p && !isspace(*p) && *p != '\n')
					p++;
				while (*p && (isspace(*p) || *p == '\n')) /* remove spaces */
					*p++ = '\0';
			}

			if (strcmp(argv[0], "nameserver") == 0) {
				for (i = 1; i < argc && __nameservers < MAX_SERVERS; i++) {
					__nameserver[__nameservers++] = strdup(argv[i]);
					DPRINTF("adding nameserver %s\n", argv[i]);
				}
			}

			/* domain and search are mutually exclusive, the last one wins */
			if (strcmp(argv[0],"domain")==0 || strcmp(argv[0],"search")==0) {
				while (__searchdomains > 0) {
					free(__searchdomain[--__searchdomains]);
					__searchdomain[__searchdomains] = NULL;
				}
				for (i=1; i < argc && __searchdomains < MAX_SEARCH; i++) {
					__searchdomain[__searchdomains++] = strdup(argv[i]);
					DPRINTF("adding search %s\n", argv[i]);
				}
			}
		}
		fclose(fp);
		DPRINTF("nameservers = %d\n", __nameservers);
		BIGUNLOCK;
		return 0;
	}
	DPRINTF("failed to open %s\n", "resolv.conf");
	h_errno = NO_RECOVERY;
	BIGUNLOCK;
	return -1;
}

void attribute_hidden __close_nameservers(void)
{
	BIGLOCK;
	while (__nameservers > 0) {
		free(__nameserver[--__nameservers]);
		__nameserver[__nameservers] = NULL;
	}
	while (__searchdomains > 0) {
		free(__searchdomain[--__searchdomains]);
		__searchdomain[__searchdomains] = NULL;
	}
	BIGUNLOCK;
}

int __encode_header(struct resolv_header *h, unsigned char *dest, int maxlen)
{
	if (maxlen < HFIXEDSZ)
		return -1;

	dest[0] = (h->id & 0xff00) >> 8;
	dest[1] = (h->id & 0x00ff) >> 0;
	dest[2] = (h->qr ? 0x80 : 0) |
		((h->opcode & 0x0f) << 3) |
		(h->aa ? 0x04 : 0) |
		(h->tc ? 0x02 : 0) |
		(h->rd ? 0x01 : 0);
	dest[3] = (h->ra ? 0x80 : 0) | (h->rcode & 0x0f);
	dest[4] = (h->qdcount & 0xff00) >> 8;
	dest[5] = (h->qdcount & 0x00ff) >> 0;
	dest[6] = (h->ancount & 0xff00) >> 8;
	dest[7] = (h->ancount & 0x00ff) >> 0;
	dest[8] = (h->nscount & 0xff00) >> 8;
	dest[9] = (h->nscount & 0x00ff) >> 0;
	dest[10] = (h->arcount & 0xff00) >> 8;
	dest[11] = (h->arcount & 0x00ff) >> 0;

	return HFIXEDSZ;
}

int __decode_header(unsigned char *data, struct resolv_header *h)
{
	h->id = (data[0] << 8) | data[1];
	h->qr = (data[2] & 0x80) ? 1 : 0;
	h->opcode = (data[2] >> 3) & 0x0f;
	h->aa = (data[2] & 0x04) ? 1 : 0;
	h->tc = (data[2] & 0x02) ? 1 : 0;
	h->rd = (data[2] & 0x01) ? 1 : 0;
	h->ra = (data[3] & 0x80) ? 1 : 0;
	h->rcode = data[3] & 0x0f;
	h->qdcount = (data[4] << 8) | data[5];
	h->ancount = (data[6] << 8) | data[7];
	h->nscount = (data[8] << 8) | data[9];
	h->arcount = (data[10] << 8) | data[11];

	return HFIXEDSZ;
}


#else /* original */

	/* Global stuff (stuff needing to be locked to be thread safe)... */
	extern int __nameservers ;
	extern char * __nameserver[MAX_SERVERS];
	extern int __searchdomains;
	extern char * __searchdomain[MAX_SEARCH];

#endif

#if 1//YWHWANGPATCH_MULTIBSS_ACN
/* Just for the record, having to lock __dns_lookup() just for these two globals
 * is pretty lame.  I think these two variables can probably be de-global-ized,
 * which should eliminate the need for doing locking here...  Needs a closer
 * look anyways. */
static int ns=0, id=1;

int __dns_lookup(const char *name, int type, int nscount, char **nsip,
			   unsigned char **outpacket, struct resolv_answer *a)
{
	int i, j, len, fd, pos, rc;
	struct timeval tv;
	fd_set fds;
	struct resolv_header h;
	struct resolv_question q;
	struct resolv_answer ma;
	int first_answer = 1;
	int retries = 0;
	unsigned char * packet = malloc(PACKETSZ);
	char *dns, *lookup = malloc(MAXDNAME);
	int variant = -1;
	struct sockaddr_in sa;
	int local_ns = -1, local_id = -1;
#ifdef __UCLIBC_HAS_IPV6__
	int v6;
	struct sockaddr_in6 sa6;
#endif

	fd = -1;

	if (!packet || !lookup || !nscount)
	    goto fail;


	DPRINTF("Looking up type %d answer for '%s'\n", type, name);

	/* Mess with globals while under lock */
	LOCK;
	local_ns = ns % nscount;
	local_id = id;
	UNLOCK;

	while (retries < MAX_RETRIES) {
		if (fd != -1)
			close(fd);

		memset(packet, 0, PACKETSZ);

		memset(&h, 0, sizeof(h));

		++local_id;
		local_id &= 0xffff;
		h.id = local_id;
		dns = nsip[local_ns];

		h.qdcount = 1;
		h.rd = 1;

		DPRINTF("encoding header\n", h.rd);

		i = __encode_header(&h, packet, PACKETSZ);
		if (i < 0)
			goto fail;

		strncpy(lookup,name,MAXDNAME);
		if (variant >= 0) {
                        BIGLOCK;
                        if (variant < __searchdomains) {
#if 1
				    int nameLen=strlen(name);

                                strncpy(lookup+nameLen,".", MAXDNAME);
                                strncpy(lookup+nameLen,__searchdomain[variant], MAXDNAME);
#else
                                strncat(lookup,".", MAXDNAME);
                                strncat(lookup,__searchdomain[variant], MAXDNAME);
#endif								
                        }
                        BIGUNLOCK;
                }
		DPRINTF("lookup name: %s\n", lookup);
		q.dotted = (char *)lookup;
		q.qtype = type;
		q.qclass = C_IN; /* CLASS_IN */

		j = __encode_question(&q, packet+i, PACKETSZ-i);
		if (j < 0)
			goto fail;

		len = i + j;

		DPRINTF("On try %d, sending query to port %d of machine %s\n",
				retries+1, NAMESERVER_PORT, dns);

#ifdef __UCLIBC_HAS_IPV6__
		v6 = inet_pton(AF_INET6, dns, &sa6.sin6_addr) > 0;
		fd = socket(v6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
		fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif
		if (fd < 0) {
                    retries++;
		    continue;
		}

		/* Connect to the UDP socket so that asyncronous errors are returned */
#ifdef __UCLIBC_HAS_IPV6__
		if (v6) {
		    sa6.sin6_family = AF_INET6;
		    sa6.sin6_port = htons(NAMESERVER_PORT);
		    /* sa6.sin6_addr is already here */
		    rc = connect(fd, (struct sockaddr *) &sa6, sizeof(sa6));
		} else {
#endif
		    sa.sin_family = AF_INET;
		    sa.sin_port = htons(NAMESERVER_PORT);
		    sa.sin_addr.s_addr = inet_addr(dns);
		    rc = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
#ifdef __UCLIBC_HAS_IPV6__
		}
#endif

		if (rc < 0) {
		    if (errno == ENETUNREACH) {
			/* routing error, presume not transient */
			goto tryall;
		    } else
			/* retry */
                        retries++;
			continue;
		}

		DPRINTF("Transmitting packet of length %d, id=%d, qr=%d\n",
				len, h.id, h.qr);
		send(fd, packet, len, 0);

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = REPLY_TIMEOUT;
		tv.tv_usec = 0;

		if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0) {
		    DPRINTF("Timeout\n");
			if(g_bPrintDNStimeout)fprintf(stderr, "ccccccc------------\n");
			/* timed out, so retry send and receive,
			 * to next nameserver on queue */
			goto tryall;
		}

		len = recv(fd, packet, 512, 0);
		if (len < HFIXEDSZ) {
			/* too short ! */
			goto again;
		}


		__decode_header(packet, &h);

		DPRINTF("id = %d, qr = %d\n", h.id, h.qr);

		if ((h.id != local_id) || (!h.qr)) {
			/* unsolicited */
			goto again;
		}


		DPRINTF("Got response %s\n", "(i think)!");
		DPRINTF("qrcount=%d,ancount=%d,nscount=%d,arcount=%d\n",
				h.qdcount, h.ancount, h.nscount, h.arcount);
		DPRINTF("opcode=%d,aa=%d,tc=%d,rd=%d,ra=%d,rcode=%d\n",
				h.opcode, h.aa, h.tc, h.rd, h.ra, h.rcode);

		if ((h.rcode) || (h.ancount < 1)) {
			/* negative result, not present */
			goto again;
		}

		pos = HFIXEDSZ;

		for (j = 0; j < h.qdcount; j++) {
			DPRINTF("Skipping question %d at %d\n", j, pos);
			i = __length_question(packet, pos);
			DPRINTF("Length of question %d is %d\n", j, i);
			if (i < 0)
				goto again;
			pos += i;
		}
		DPRINTF("Decoding answer at pos %d\n", pos);


		first_answer = 1;
		for (j=0;j<h.ancount;j++,pos += i)
		{
		    i = __decode_answer(packet, pos, &ma);

		    if (i<0) {
			DPRINTF("failed decode %d\n", i);
			goto again;
		    }

		    if ( first_answer )
		    {
			ma.buf = a->buf;
			ma.buflen = a->buflen;
			ma.add_count = a->add_count;
			memcpy(a, &ma, sizeof(ma));
			if (a->atype != T_SIG && (0 == a->buf || (type != T_A && type != T_AAAA)))
			{
			    break;
			}
			if (a->atype != type)
			{
			    free(a->dotted);
			    continue;
			}
			a->add_count = h.ancount - j - 1;
			if ((a->rdlength + sizeof(struct in_addr*)) * a->add_count > a->buflen)
			{
			    break;
			}
			a->add_count = 0;
			first_answer = 0;
		    }
		    else
		    {
			free(ma.dotted);
			if (ma.atype != type)
			{
			    continue;
			}
			if (a->rdlength != ma.rdlength)
			{
			    free(a->dotted);
			    DPRINTF("Answer address len(%u) differs from original(%u)\n",
				    ma.rdlength, a->rdlength);
			    goto again;
			}
			memcpy(a->buf + (a->add_count * ma.rdlength), ma.rdata, ma.rdlength);
			++a->add_count;
		    }
		}

		DPRINTF("Answer name = |%s|\n", a->dotted);
		DPRINTF("Answer type = |%d|\n", a->atype);

		close(fd);

		if (outpacket)
			*outpacket = packet;
		else
			free(packet);
		free(lookup);

		/* Mess with globals while under lock */
		LOCK;
		ns = local_ns;
		id = local_id;
		UNLOCK;

		return (len);				/* success! */

	  tryall:

		/* if there are other nameservers, give them a go,
		   otherwise return with error */
		{
		    variant = -1;
                    local_ns = (local_ns + 1) % nscount;
                    if (local_ns == 0)
                      retries++;

                    continue;
		}

	  again:

		/* if there are searchdomains, try them or fallback as passed */
		{
		    int sdomains;
		    BIGLOCK;
		    sdomains=__searchdomains;
		    BIGUNLOCK;

		    if (variant < sdomains - 1) {
			/* next search */
			variant++;
		    } else {
			/* next server, first search */
			local_ns = (local_ns + 1) % nscount;
                        if (local_ns == 0)
                          retries++;

			variant = -1;
		    }
		}
	}

fail:
	if (fd != -1)
	    close(fd);
	if (lookup)
	    free(lookup);
	if (packet)
	    free(packet);
	h_errno = NETDB_INTERNAL;
	/* Mess with globals while under lock */
	if (local_ns != -1) {
	    LOCK;
	    ns = local_ns;
	    id = local_id;
	    UNLOCK;
	}
	return -1;
}
int gethostbyname_r_2(char* if_name, const char * name,
			    struct hostent * result_buf,
			    char * buf, size_t buflen,
			    struct hostent ** result,
			    int * h_errnop, int serverchoice)
{
	struct in_addr *in;
	struct in_addr **addr_list;
	char **alias;
	unsigned char *packet;
	struct resolv_answer a;
	int i;
	int __nameserversXX;
	char ** __nameserverXX;
#if 1
	char * __nameserverXX_backup;
#endif


	__open_nameservers();
	*result=NULL;
	if (!name)
		return EINVAL;
#if 0
	/* do /etc/hosts first */
	{
		int old_errno = errno;	/* Save the old errno and reset errno */
		__set_errno(0);			/* to check for missing /etc/hosts. */

		if ((i=__get_hosts_byname_r(name, AF_INET, result_buf,
				buf, buflen, result, h_errnop))==0)
			return i;
		switch (*h_errnop) {
			case HOST_NOT_FOUND:
			case NO_ADDRESS:
				break;
			case NETDB_INTERNAL:
				if (errno == ENOENT) {
					break;
				}
				/* else fall through */
			default:
				return i;
		}
		__set_errno(old_errno);
	}

	DPRINTF("Nothing found in /etc/hosts\n");
#endif

	*h_errnop = NETDB_INTERNAL;
	if (buflen < sizeof(*in))
		return ERANGE;
	in=(struct in_addr*)buf;
	buf+=sizeof(*in);
	buflen-=sizeof(*in);

	if (buflen < sizeof(*addr_list)*2)
		return ERANGE;
	addr_list=(struct in_addr**)buf;
	buf+=sizeof(*addr_list)*2;
	buflen-=sizeof(*addr_list)*2;

	addr_list[0] = in;
	addr_list[1] = 0;

	if (buflen < sizeof(char *)*(ALIAS_DIM))
		return ERANGE;
	alias=(char **)buf;
	buf+=sizeof(char **)*(ALIAS_DIM);
	buflen-=sizeof(char **)*(ALIAS_DIM);

	if (buflen<256)
		return ERANGE;
	strncpy(buf, name, buflen);

	alias[0] = buf;
	alias[1] = NULL;

	/* First check if this is already an address */
	if (inet_aton(name, in)) {
	    result_buf->h_name = buf;
	    result_buf->h_addrtype = AF_INET;
	    result_buf->h_length = sizeof(*in);
	    result_buf->h_addr_list = (char **) addr_list;
	    result_buf->h_aliases = alias;
	    *result=result_buf;
	    *h_errnop = NETDB_SUCCESS;
	    return NETDB_SUCCESS;
	}

	for (;;) {

	    BIGLOCK;
	    __nameserversXX=__nameservers;
	    __nameserverXX=__nameserver;
	    BIGUNLOCK;
	    a.buf = buf;
	    a.buflen = buflen;
	    a.add_count = 0;
#if 1
	    __nameserverXX_backup = __nameserverXX[0];
	    if(serverchoice == 1){
	    	__nameserversXX = 1;

	    }else if(serverchoice == 2){
	    	__nameserversXX = 1;
	    	__nameserverXX[0] =__nameserverXX[1];
	    }
#if 1//YWHWANG_PATCH_NODNSHANDLING
	    if(!__nameserverXX[__nameserversXX-1]){
		*h_errnop = HOST_NOT_FOUND;
		DPRINTF("__dns_lookup\n");
		return TRY_AGAIN;
	    }
#endif
#endif
	    i = __dns_lookup(name, T_A, __nameserversXX, __nameserverXX, &packet, &a);
	    check_hostapd_alive2(if_name);

#if 1
	    __nameserverXX[0] = __nameserverXX_backup;
#endif

	    if (i < 0) {
		*h_errnop = HOST_NOT_FOUND;
		DPRINTF("__dns_lookup\n");
		return TRY_AGAIN;
	    }

	    if ((a.rdlength + sizeof(struct in_addr*)) * a.add_count + 256 > buflen)
	    {
		free(a.dotted);
		free(packet);
		*h_errnop = NETDB_INTERNAL;
		DPRINTF("buffer too small for all addresses\n");
		return ERANGE;
	    }
	    else if(a.add_count > 0)
	    {
		memmove(buf - sizeof(struct in_addr*)*2, buf, a.add_count * a.rdlength);
		addr_list = (struct in_addr**)(buf + a.add_count * a.rdlength);
		addr_list[0] = in;
		for (i = a.add_count-1; i>=0; --i)
		    addr_list[i+1] = (struct in_addr*)(buf - sizeof(struct in_addr*)*2 + a.rdlength * i);
		addr_list[a.add_count + 1] = 0;
		buflen -= (((char*)&(addr_list[a.add_count + 2])) - buf);
		buf = (char*)&addr_list[a.add_count + 2];
	    }

	    strncpy(buf, a.dotted, buflen);
	    free(a.dotted);

	    if (a.atype == T_A) { /* ADDRESS */
		memcpy(in, a.rdata, sizeof(*in));
		result_buf->h_name = buf;
		result_buf->h_addrtype = AF_INET;
		result_buf->h_length = sizeof(*in);
		result_buf->h_addr_list = (char **) addr_list;
#ifdef __UCLIBC_MJN3_ONLY__
#warning TODO -- generate the full list
#endif
		result_buf->h_aliases = alias; /* TODO: generate the full list */
		free(packet);
		break;
	    } else {
		free(packet);
		*h_errnop=HOST_NOT_FOUND;
		return TRY_AGAIN;
	    }
	}

	*result=result_buf;
	*h_errnop = NETDB_SUCCESS;

	return NETDB_SUCCESS;
}
struct hostent *gethostbyname_ext(char* if_name,const char *name, int serverchoice)
{
	static struct hostent h;
	static char buf[sizeof(struct in_addr) +
			sizeof(struct in_addr *)*2 +
			sizeof(char *)*(ALIAS_DIM) + 384/*namebuffer*/ + 32/* margin */];
	struct hostent *hp=NULL;
	gethostbyname_r_2(if_name,name, &h, buf, sizeof(buf), &hp, &h_errno, serverchoice);
	return hp;
}
#endif

/*
 * End
 */

#if 1//YWHWANGPATCH_MULTIBSS_ACN
//#include <ucontext.h>
//#include "libtask/taskimpl.h"
#include "taskimpl.h"
#define ASYNC_METHOD 2

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)



/* Just for the record, having to lock __dns_lookup() just for these two globals
 * is pretty lame.  I think these two variables can probably be de-global-ized,
 * which should eliminate the need for doing locking here...  Needs a closer
 * look anyways. */
//static int ns=0, id=1;

int first_init_nameserver=1;
void gethostbyname_ext_eloop_finalize()
{
        if(!first_init_nameserver){
                first_init_nameserver=1;
                __close_nameservers();
        }
}
void gethostbyname_ext_eloop_reconf()
{
        if(!first_init_nameserver){
                first_init_nameserver=1;
                __close_nameservers();
        }
        DPRINTF("gethostbyname_ext_eloop_reconf init nameservers\n");
        first_init_nameserver=0;__open_nameservers();
}


#define DBG_TASK_SCHED 0

#define HYW_MAX_RETRIES 4
#define HYW_MAX_TIMEOUT 5
#if ASYNC_METHOD==1//ASYNC
#define puctx_eloop(pdnsCtx) (&pdnsCtx->main_uc) /*for caller*/
#define puctx_resolver(pdnsCtx) (&pdnsCtx->func_uc) /*for coroutine*/
#define uctx_eloop(pdnsCtx) (pdnsCtx->main_uc) /*for caller*/
#define uctx_resolver(pdnsCtx) (pdnsCtx->func_uc) /*for coroutine*/
typedef struct _DNSCtx{
        /*for ucontext*/
        int bCompleted;
        ucontext_t main_uc, func_uc;//for caller & callee
        
        /*for callback*/
        OnDNScompleteFunc onComplete;void* userCtx;
        
        /*for return value*/
        //int retCode;
	struct hostent h;
	char buf[sizeof(struct in_addr) +
			sizeof(struct in_addr *)*2 +
			sizeof(char *)*(ALIAS_DIM) + 384/*namebuffer*/ + 32/* margin */];
	struct hostent *hp;
	int h_errno_P;
	
	/*for initial args*/
	char* if_name; const char * name;
	struct hostent * result_buf;
	char * buf_in; size_t buflen;
	struct hostent ** result;
	int * h_errnop; int serverchoice;int dwTimeout;int nRetries;
}DNSCtx,*PDNSCtx;
static void IsCompleted_DNS(PDNSCtx pdnsctx)
{
   if(pdnsctx->bCompleted){
           pdnsctx->onComplete(pdnsctx,pdnsctx->hp,pdnsctx->userCtx);
           gethostbyname_ext_free_result(pdnsctx);
   }
}
static void eloop_handler_DNS(void *eloop_data, void *user_ctx, int bTimeout)
{
   PDNSCtx pdnsctx=(PDNSCtx)eloop_data;
   int *pisTimeout=(int*)user_ctx;
   
   if(DBG_TASK_SCHED)printf("eloop_handler_DNS: swapcontext(&uctx_eloop, &uctx_resolver)\n");
   *pisTimeout=bTimeout;
    if (swapcontext(puctx_eloop(pdnsctx), puctx_resolver(pdnsctx)) == -1)
        die("swapcontext");

   if(DBG_TASK_SCHED)printf("eloop_handler_DNS: checking result\n");
   IsCompleted_DNS(pdnsctx);//for async mode!!
   if(DBG_TASK_SCHED)printf("eloop_handler_DNS: exiting\n");
}
static void eloop_timeout_handler_mySockDNS(void *eloop_data, void *user_ctx)
{
        return eloop_handler_DNS(eloop_data,user_ctx,1);
}
static void eloop_sock_handler_mySockDNS(int sock, void *eloop_ctx, void *sock_ctx)
{
        return eloop_handler_DNS(eloop_ctx,sock_ctx,0);
}
static void eloop_mySleep(int secs,int usecs,PDNSCtx pdnsCtx)
{
        int isTimeout=-1;
        
        eloop_register_timeout(secs,usecs,eloop_timeout_handler_mySockDNS,pdnsCtx,(void*)&isTimeout);
        if(DBG_TASK_SCHED)printf("__dns_lookup: running\n");
        if(DBG_TASK_SCHED)printf("__dns_lookup: swapcontext(&uctx_resolver, &uctx_eloop)\n");
        if (swapcontext(puctx_resolver(pdnsCtx), puctx_eloop(pdnsCtx)) == -1)
                die("swapcontext");
        if(DBG_TASK_SCHED)printf("__dns_lookup: resumed\n");
        if(!isTimeout)eloop_cancel_timeout(eloop_timeout_handler_mySockDNS,pdnsCtx,(void*)&isTimeout);//auto unregister timeout!!
}
#elif ASYNC_METHOD==2//ASYNC
typedef struct _DNSCtx{
        /*for ucontext*/
        baseTaskCtx ctx;
        
        /*for callback*/
        OnDNScompleteFunc onComplete;void* userCtx;
        
        /*for return value*/
        //int retCode;
	struct hostent h;
	char buf[sizeof(struct in_addr) +
			sizeof(struct in_addr *)*2 +
			sizeof(char *)*(ALIAS_DIM) + 384/*namebuffer*/ + 32/* margin */];
	struct hostent *hp;
	int h_errno_P;
	
	/*for initial args*/
	char* if_name; const char * name;
	struct hostent * result_buf;
	char * buf_in; size_t buflen;
	struct hostent ** result;
	int * h_errnop; int serverchoice;int dwTimeout;int nRetries;
}DNSCtx,*PDNSCtx;
#endif

static int __dns_lookup_async(const char *name, int type, int nscount, char **nsip,unsigned char **outpacket, struct resolv_answer *a,int dwTimeout,int nRetries,PDNSCtx pdnsCtx)
{
#if 1//ASYNC
        int isTimeout=-1;
#endif
	int i, j, len, fd, pos, rc;
	struct timeval tv;
	fd_set fds;
	struct resolv_header h;
	struct resolv_question q;
	struct resolv_answer ma;
	int first_answer = 1;
	int retries = 0;
	unsigned char * packet = malloc(PACKETSZ);
	char *dns, *lookup = malloc(MAXDNAME);
	int variant = -1;
	struct sockaddr_in sa;
	int local_ns = -1, local_id = -1;
#ifdef __UCLIBC_HAS_IPV6__
	int v6;
	struct sockaddr_in6 sa6;
#endif

	fd = -1;

	if (!packet || !lookup || !nscount)
	    goto fail;


	DPRINTF("Looking up type %d answer for '%s'\n", type, name);

	/* Mess with globals while under lock */
	LOCK;
	local_ns = ns % nscount;
	local_id = id;
	UNLOCK;

	while (retries < nRetries/*MAX_RETRIES*/) {
		if (fd != -1)
			close(fd);

		memset(packet, 0, PACKETSZ);

		memset(&h, 0, sizeof(h));

		++local_id;
		local_id &= 0xffff;
		h.id = local_id;
		dns = nsip[local_ns];

		h.qdcount = 1;
		h.rd = 1;

		DPRINTF("encoding header\n", h.rd);

		i = __encode_header(&h, packet, PACKETSZ);
		if (i < 0)
			goto fail;

		strncpy(lookup,name,MAXDNAME);
		if (variant >= 0) {
                        BIGLOCK;
                        if (variant < __searchdomains) {
#if 1
				    int nameLen=strlen(name);

                                strncpy(lookup+nameLen,".", MAXDNAME);
                                strncpy(lookup+nameLen,__searchdomain[variant], MAXDNAME);
#else
                                strncat(lookup,".", MAXDNAME);
                                strncat(lookup,__searchdomain[variant], MAXDNAME);
#endif								
                        }
                        BIGUNLOCK;
                }
		DPRINTF("lookup name: %s\n", lookup);
		q.dotted = (char *)lookup;
		q.qtype = type;
		q.qclass = C_IN; /* CLASS_IN */

		j = __encode_question(&q, packet+i, PACKETSZ-i);
		if (j < 0)
			goto fail;

		len = i + j;

		DPRINTF("On try %d, sending query to port %d of machine %s\n",
				retries+1, NAMESERVER_PORT, dns);

#ifdef __UCLIBC_HAS_IPV6__
		v6 = inet_pton(AF_INET6, dns, &sa6.sin6_addr) > 0;
		fd = socket(v6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
		fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif
		if (fd < 0) {
                    retries++;
                    if(DBG_TASK_SCHED)printf("__dns_lookup: socket error\n");
		    continue;
		}

		/* Connect to the UDP socket so that asyncronous errors are returned */
#ifdef __UCLIBC_HAS_IPV6__
		if (v6) {
		    sa6.sin6_family = AF_INET6;
		    sa6.sin6_port = htons(NAMESERVER_PORT);
		    /* sa6.sin6_addr is already here */
		    rc = connect(fd, (struct sockaddr *) &sa6, sizeof(sa6));
		} else {
#endif
		    sa.sin_family = AF_INET;
		    sa.sin_port = htons(NAMESERVER_PORT);
		    sa.sin_addr.s_addr = inet_addr(dns);
		    rc = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
#ifdef __UCLIBC_HAS_IPV6__
		}
#endif

		if (rc < 0) {
		    if (errno == ENETUNREACH) {
#if ASYNC_METHOD==1//ASYNC
			if(DBG_TASK_SCHED)printf("__dns_lookup: connect(UDP) error ENETUNREACH\n");
			eloop_mySleep(dwTimeout,0,pdnsCtx);
#elif ASYNC_METHOD==2
			errno=0;//auto clear error_no
			sleep_task_on_defaultloop(dwTimeout,0,(PbaseTaskCtx)pdnsCtx,NULL,NULL/*or timerId*/);
#endif
			/* routing error, presume not transient */
			goto tryall;
		    } else
			/* retry */
                        retries++;
			continue;
		}

		DPRINTF("Transmitting packet of length %d, id=%d, qr=%d\n",
				len, h.id, h.qr);
		send(fd, packet, len, 0);
#if ASYNC_METHOD==1//ASYNC
                if(pdnsCtx->onComplete){
                        eloop_register_read_sock(fd,eloop_sock_handler_mySockDNS,pdnsCtx,(void*)&isTimeout);
                        eloop_register_timeout(dwTimeout,0,eloop_timeout_handler_mySockDNS,pdnsCtx,(void*)&isTimeout);
                        if(DBG_TASK_SCHED)printf("__dns_lookup: running\n");
                        if(DBG_TASK_SCHED)printf("__dns_lookup: swapcontext(&uctx_resolver, &uctx_eloop)\n");
                        if (swapcontext(puctx_resolver(pdnsCtx), puctx_eloop(pdnsCtx)) == -1)
                                die("swapcontext");
                        if(DBG_TASK_SCHED)printf("__dns_lookup: resumed\n");
                        eloop_unregister_read_sock(fd);//always auto unregister socket!!
                        if(!isTimeout)eloop_cancel_timeout(eloop_timeout_handler_mySockDNS,pdnsCtx,(void*)&isTimeout);//auto unregister timeout!!
                }
                else{
        		FD_ZERO(&fds);
        		FD_SET(fd, &fds);
        		tv.tv_sec = dwTimeout;//REPLY_TIMEOUT;
        		tv.tv_usec = 0;
        
        		if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
        		        isTimeout=1;
        		else    isTimeout=0;
                }
                if(isTimeout)
#elif ASYNC_METHOD==2
                if(pdnsCtx->onComplete){
                        defaultloop_register_read_sock(fd,defaultloop_sock_handler_for_task,pdnsCtx,(void*)&isTimeout);//Eloop depdendet code
                        if(DBG_TASK_SCHED)printf("__dns_lookup: running\n");
 				isTimeout=sleep_task_on_defaultloop(dwTimeout,0,(PbaseTaskCtx)pdnsCtx,NULL,NULL/*or timerId*/);						
                        if(DBG_TASK_SCHED)printf("__dns_lookup: resumed(isTimeout=%d)\n",isTimeout);
                        defaultloop_unregister_read_sock(fd);//always auto unregister socket!!//Eloop depdendet code
                }
                else{
        		FD_ZERO(&fds);
        		FD_SET(fd, &fds);
        		tv.tv_sec = dwTimeout;//REPLY_TIMEOUT;
        		tv.tv_usec = 0;
        
        		if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
        		        isTimeout=1;
        		else    isTimeout=0;
                }
                if(isTimeout==1)
#else
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = REPLY_TIMEOUT;
		tv.tv_usec = 0;

		if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
#endif
		{
		    DPRINTF("Timeout\n");
			if(g_bPrintDNStimeout)fprintf(stderr, "ccccccc------------\n");
			/* timed out, so retry send and receive,
			 * to next nameserver on queue */
			goto tryall;
		}

		len = recv(fd, packet, 512, 0);
		if (len < HFIXEDSZ) {
			/* too short ! */
			goto again;
		}


		__decode_header(packet, &h);

		DPRINTF("id = %d, qr = %d\n", h.id, h.qr);

		if ((h.id != local_id) || (!h.qr)) {
			/* unsolicited */
			goto again;
		}


		DPRINTF("Got response %s\n", "(i think)!");
		DPRINTF("qrcount=%d,ancount=%d,nscount=%d,arcount=%d\n",
				h.qdcount, h.ancount, h.nscount, h.arcount);
		DPRINTF("opcode=%d,aa=%d,tc=%d,rd=%d,ra=%d,rcode=%d\n",
				h.opcode, h.aa, h.tc, h.rd, h.ra, h.rcode);

		if ((h.rcode) || (h.ancount < 1)) {
			/* negative result, not present */
			goto again;
		}

		pos = HFIXEDSZ;

		for (j = 0; j < h.qdcount; j++) {
			DPRINTF("Skipping question %d at %d\n", j, pos);
			i = __length_question(packet, pos);
			DPRINTF("Length of question %d is %d\n", j, i);
			if (i < 0)
				goto again;
			pos += i;
		}
		DPRINTF("Decoding answer at pos %d\n", pos);


		first_answer = 1;
		for (j=0;j<h.ancount;j++,pos += i)
		{
		    i = __decode_answer(packet, pos, &ma);

		    if (i<0) {
			DPRINTF("failed decode %d\n", i);
			goto again;
		    }

		    if ( first_answer )
		    {
			ma.buf = a->buf;
			ma.buflen = a->buflen;
			ma.add_count = a->add_count;
			memcpy(a, &ma, sizeof(ma));
			if (a->atype != T_SIG && (0 == a->buf || (type != T_A && type != T_AAAA)))
			{
			    break;
			}
			if (a->atype != type)
			{
			    free(a->dotted);
			    continue;
			}
			a->add_count = h.ancount - j - 1;
			if ((a->rdlength + sizeof(struct in_addr*)) * a->add_count > a->buflen)
			{
			    break;
			}
			a->add_count = 0;
			first_answer = 0;
		    }
		    else
		    {
			free(ma.dotted);
			if (ma.atype != type)
			{
			    continue;
			}
			if (a->rdlength != ma.rdlength)
			{
			    free(a->dotted);
			    DPRINTF("Answer address len(%u) differs from original(%u)\n",
				    ma.rdlength, a->rdlength);
			    goto again;
			}
			memcpy(a->buf + (a->add_count * ma.rdlength), ma.rdata, ma.rdlength);
			++a->add_count;
		    }
		}

		DPRINTF("Answer name = |%s|\n", a->dotted);
		DPRINTF("Answer type = |%d|\n", a->atype);

		close(fd);

		if (outpacket)
			*outpacket = packet;
		else
			free(packet);
		free(lookup);

		/* Mess with globals while under lock */
		LOCK;
		ns = local_ns;
		id = local_id;
		UNLOCK;

		return (len);				/* success! */

	  tryall:

		/* if there are other nameservers, give them a go,
		   otherwise return with error */
		{
		    variant = -1;
                    local_ns = (local_ns + 1) % nscount;
                    if (local_ns == 0)
                      retries++;

                    continue;
		}

	  again:

		/* if there are searchdomains, try them or fallback as passed */
		{
		    int sdomains;
		    BIGLOCK;
		    sdomains=__searchdomains;
		    BIGUNLOCK;

		    if (variant < sdomains - 1) {
			/* next search */
			variant++;
		    } else {
			/* next server, first search */
			local_ns = (local_ns + 1) % nscount;
                        if (local_ns == 0)
                          retries++;

			variant = -1;
		    }
		}
	}

fail:
	if (fd != -1)
	    close(fd);
	if (lookup)
	    free(lookup);
	if (packet)
	    free(packet);
	h_errno = NETDB_INTERNAL;
	/* Mess with globals while under lock */
	if (local_ns != -1) {
	    LOCK;
	    ns = local_ns;
	    id = local_id;
	    UNLOCK;
	}
	return -1;
}
static int gethostbyname_r_2_async(char* if_name, const char * name, struct hostent * result_buf, char * buf, size_t buflen, struct hostent ** result, int * h_errnop, int serverchoice,int dwTimeout,int nRetries,PDNSCtx pdnsCtx)
{
	struct in_addr *in;
	struct in_addr **addr_list;
	char **alias;
	unsigned char *packet;
	struct resolv_answer a;
	int i;
	int __nameserversXX;
	char ** __nameserverXX;
#if 1
	char * __nameserverXX_backup;
#endif

#if 1//ASYNC
        if(first_init_nameserver){
                DPRINTF("gethostbyname_ext_eloop_req init nameservers\n");
                first_init_nameserver=0;__open_nameservers();
        }
#else
	__open_nameservers();
#endif
	*result=NULL;
	if (!name)
		return EINVAL;
#if 0
	/* do /etc/hosts first */
	{
		int old_errno = errno;	/* Save the old errno and reset errno */
		__set_errno(0);			/* to check for missing /etc/hosts. */

		if ((i=__get_hosts_byname_r(name, AF_INET, result_buf,
				buf, buflen, result, h_errnop))==0)
			return i;
		switch (*h_errnop) {
			case HOST_NOT_FOUND:
			case NO_ADDRESS:
				break;
			case NETDB_INTERNAL:
				if (errno == ENOENT) {
					break;
				}
				/* else fall through */
			default:
				return i;
		}
		__set_errno(old_errno);
	}

	DPRINTF("Nothing found in /etc/hosts\n");
#endif

	*h_errnop = NETDB_INTERNAL;
	if (buflen < sizeof(*in))
		return ERANGE;
	in=(struct in_addr*)buf;
	buf+=sizeof(*in);
	buflen-=sizeof(*in);

	if (buflen < sizeof(*addr_list)*2)
		return ERANGE;
	addr_list=(struct in_addr**)buf;
	buf+=sizeof(*addr_list)*2;
	buflen-=sizeof(*addr_list)*2;

	addr_list[0] = in;
	addr_list[1] = 0;

	if (buflen < sizeof(char *)*(ALIAS_DIM))
		return ERANGE;
	alias=(char **)buf;
	buf+=sizeof(char **)*(ALIAS_DIM);
	buflen-=sizeof(char **)*(ALIAS_DIM);

	if (buflen<256)
		return ERANGE;
	strncpy(buf, name, buflen);

	alias[0] = buf;
	alias[1] = NULL;

	/* First check if this is already an address */
	if (inet_aton(name, in)) {
	    result_buf->h_name = buf;
	    result_buf->h_addrtype = AF_INET;
	    result_buf->h_length = sizeof(*in);
	    result_buf->h_addr_list = (char **) addr_list;
	    result_buf->h_aliases = alias;
	    *result=result_buf;
	    *h_errnop = NETDB_SUCCESS;
	    return NETDB_SUCCESS;
	}

	for (;;) {

	    BIGLOCK;
	    __nameserversXX=__nameservers;
	    __nameserverXX=__nameserver;
	    BIGUNLOCK;
	    a.buf = buf;
	    a.buflen = buflen;
	    a.add_count = 0;
#if 1
	    __nameserverXX_backup = __nameserverXX[0];
	    if(serverchoice == 1){
	    	__nameserversXX = 1;

	    }else if(serverchoice == 2){
	    	__nameserversXX = 1;
	    	__nameserverXX[0] =__nameserverXX[1];
	    }
#endif
	    i = __dns_lookup_async(name, T_A, __nameserversXX, __nameserverXX, &packet, &a,dwTimeout,nRetries,pdnsCtx);
	    check_hostapd_alive2(if_name);

#if 1
	    __nameserverXX[0] = __nameserverXX_backup;
#endif

	    if (i < 0) {
		*h_errnop = HOST_NOT_FOUND;
		DPRINTF("__dns_lookup\n");
		return TRY_AGAIN;
	    }

	    if ((a.rdlength + sizeof(struct in_addr*)) * a.add_count + 256 > buflen)
	    {
		free(a.dotted);
		free(packet);
		*h_errnop = NETDB_INTERNAL;
		DPRINTF("buffer too small for all addresses\n");
		return ERANGE;
	    }
	    else if(a.add_count > 0)
	    {
		memmove(buf - sizeof(struct in_addr*)*2, buf, a.add_count * a.rdlength);
		addr_list = (struct in_addr**)(buf + a.add_count * a.rdlength);
		addr_list[0] = in;
		for (i = a.add_count-1; i>=0; --i)
		    addr_list[i+1] = (struct in_addr*)(buf - sizeof(struct in_addr*)*2 + a.rdlength * i);
		addr_list[a.add_count + 1] = 0;
		buflen -= (((char*)&(addr_list[a.add_count + 2])) - buf);
		buf = (char*)&addr_list[a.add_count + 2];
	    }

	    strncpy(buf, a.dotted, buflen);
	    free(a.dotted);

	    if (a.atype == T_A) { /* ADDRESS */
		memcpy(in, a.rdata, sizeof(*in));
		result_buf->h_name = buf;
		result_buf->h_addrtype = AF_INET;
		result_buf->h_length = sizeof(*in);
		result_buf->h_addr_list = (char **) addr_list;
#ifdef __UCLIBC_MJN3_ONLY__
#warning TODO -- generate the full list
#endif
		result_buf->h_aliases = alias; /* TODO: generate the full list */
		free(packet);
		break;
	    } else {
		free(packet);
		*h_errnop=HOST_NOT_FOUND;
		return TRY_AGAIN;
	    }
	}

	*result=result_buf;
	*h_errnop = NETDB_SUCCESS;

	return NETDB_SUCCESS;
}


#if ASYNC_METHOD==1//ASYNC
/*static*/ void gethostbyname_r_2_mips_start(PDNSCtx pdnsCtx)
{
/*run on issuer*/
    int ret;
    if(DBG_TASK_SCHED)printf("gethostbyname_r_2_mips_start: started\n");//startfn,startargs
    ret=gethostbyname_r_2_async(pdnsCtx->if_name,pdnsCtx->name,pdnsCtx->result_buf,pdnsCtx->buf_in,pdnsCtx->buflen,pdnsCtx->result,pdnsCtx->h_errnop,pdnsCtx->serverchoice,pdnsCtx->dwTimeout,pdnsCtx->nRetries,pdnsCtx);
/*run on callback*/
    if(DBG_TASK_SCHED)printf("gethostbyname_r_2: returned\n");

    pdnsCtx->bCompleted=1;//pdnsCtx->retCode=ret;
    if(DBG_TASK_SCHED)printf("gethostbyname_r_2_mips_start: swapcontext(&uctx_resolver, &uctx_eloop)\n");
    if (swapcontext(puctx_resolver(pdnsCtx), puctx_eloop(pdnsCtx)) == -1)
        die("swapcontext");
    /*can not reach here*/
    if(DBG_TASK_SCHED)printf("gethostbyname_r_2_mips_start: returning(can not reach here)\n");
    //taskexit(0);
    //return ret;
}

void gethostbyname_ext_free_result(PDNSCtx pdnsCtx)
{
        free(pdnsCtx);
}
PDNSCtx gethostbyname_ext_task_alloc(char* if_name,const char *name, int serverchoice,int dwTimeout,int nRetries,OnDNScompleteFunc onComplete,void* userCtx)
{
#define FUNC1_STACK_SIZE 32768
        unsigned int *func_stack;//tmp
        //int dwTimeout=HYW_MAX_TIMEOUT,nRetries=HYW_MAX_RETRIES;
        PDNSCtx pdnsCtx;
        
        pdnsCtx=(PDNSCtx)malloc(sizeof(DNSCtx)+FUNC1_STACK_SIZE);
        if(pdnsCtx==NULL){
                fprintf(stderr,"gethostbyname_ext_async malloc : DNSCtx\n");
                return NULL;
        }
        
        memset(pdnsCtx,0,sizeof(DNSCtx));
        
        func_stack=(unsigned int*)(pdnsCtx+1);
        pdnsCtx->bCompleted=0;pdnsCtx->userCtx=userCtx;pdnsCtx->onComplete=onComplete;
        
        memset(puctx_resolver(pdnsCtx),0,sizeof(uctx_resolver(pdnsCtx)));
#if 0   
        {
        sigset_t zero;//tmp
	sigemptyset(&zero);
	sigprocmask(SIG_BLOCK, &zero, &puctx_resolver(pdnsCtx)->uc_sigmask);
	}
#endif

        if (getcontext(puctx_resolver(pdnsCtx)) == -1)
                die("getcontext");
        puctx_resolver(pdnsCtx)->uc_stack.ss_sp = func_stack+8;
        puctx_resolver(pdnsCtx)->uc_stack.ss_size = FUNC1_STACK_SIZE-64;
        //puctx_resolver(pdnsCtx)->uc_link = (struct __ucontext*)puctx_eloop(pdnsCtx);//not supported!!
#if 0//Many Params(cause exception)
        makecontext(puctx_resolver(pdnsCtx),(void(*)())gethostbyname_r_2_mips_start,18/*argc*/,if_name/*$4*/,name/*$5*/, &pdnsCtx->h/*$6*/, pdnsCtx->buf/*$7*/,0,0,0,0,0,0,0,0,sizeof(pdnsCtx->buf)/*$16*/, &pdnsCtx->hp/*$17*/, &pdnsCtx->h_errno_P/*$18*/, serverchoice/*$19*/,dwTimeout/*$20*/,pdnsCtx/*$21*/);
#else
	pdnsCtx->if_name=if_name;pdnsCtx->name=name;
	pdnsCtx->result_buf=&pdnsCtx->h;pdnsCtx->buf_in=pdnsCtx->buf;pdnsCtx->buflen=sizeof(pdnsCtx->buf);
	pdnsCtx->result=&pdnsCtx->hp;pdnsCtx->h_errnop=&pdnsCtx->h_errno_P; 
	pdnsCtx->serverchoice=serverchoice;pdnsCtx->dwTimeout=dwTimeout;pdnsCtx->nRetries=nRetries;
        makecontext(puctx_resolver(pdnsCtx),(void(*)())gethostbyname_r_2_mips_start,1,pdnsCtx/*$21*/);
#endif
        return pdnsCtx;
}
void gethostbyname_ext_task_sched(PDNSCtx pdnsCtx)
{
        if(DBG_TASK_SCHED)printf("gethostbyname_ext_async: swapcontext(&uctx_eloop, &uctx_resolver)\n");
        if (swapcontext(puctx_eloop(pdnsCtx), puctx_resolver(pdnsCtx)) == -1)
                die("swapcontext");
        if(DBG_TASK_SCHED)printf("gethostbyname_ext_async: exiting\n");
}
#elif ASYNC_METHOD==2//ASYNC
#define GETHOSTBYNAME_STACK_SIZE 32768
void* gethostbyname_r_2_taskExitCallback(void* ctx,void* user_data)
{
	PDNSCtx pdnsCtx=(PDNSCtx)ctx;
	pdnsCtx->onComplete(pdnsCtx,pdnsCtx->hp,pdnsCtx->userCtx);
	return NULL;
}
void* gethostbyname_r_2_taskEntry(void* ctx,int arg1)
{
	PDNSCtx pdnsCtx=(PDNSCtx)ctx;
    int ret;
    if(DBG_TASK_SCHED)printf("gethostbyname_r_2_taskEntry: started(%d)\n",arg1);//startfn,startargs
    ret=gethostbyname_r_2_async(pdnsCtx->if_name,pdnsCtx->name,pdnsCtx->result_buf,pdnsCtx->buf_in,pdnsCtx->buflen,pdnsCtx->result,pdnsCtx->h_errnop,pdnsCtx->serverchoice,pdnsCtx->dwTimeout,pdnsCtx->nRetries,pdnsCtx);

    if(DBG_TASK_SCHED)printf("gethostbyname_r_2_taskEntry: exit_task\n");
	if(ret!=0||*pdnsCtx->h_errnop){
		//printf("gethostbyname_r_2_arm_start: ret=%d,h_errno=%d\n",ret,*pdnsCtx->h_errnop);
		//memset(&pdnsCtx->h,0,sizeof(pdnsCtx->h));//fill bad!!
		//pdnsCtx->h.h_addrtype = ~AF_INET;  
		;
	}
	exit_task((PbaseTaskCtx)pdnsCtx,gethostbyname_r_2_taskExitCallback,NULL);
    /*can not reach here*/
    if(DBG_TASK_SCHED)printf("gethostbyname_r_2_taskEntry: returning(can not reach here)\n");
	return NULL;
}
#endif


struct hostent *gethostbyname_ext_async(char* if_name,const char *name, int serverchoice,int dwTimeout,int nRetries,OnDNScompleteFunc onComplete,void* userCtx)
{
#if ASYNC_METHOD==1//ASYNC
        PDNSCtx pdnsCtx;

		if(!onComplete)goto SyncStart;
        pdnsCtx=gethostbyname_ext_task_alloc(if_name,name,serverchoice,dwTimeout,nRetries,onComplete,userCtx);
        gethostbyname_ext_task_sched(pdnsCtx);
        IsCompleted_DNS(pdnsCtx);//for sync mode!!
        return NULL;
#elif ASYNC_METHOD==2//ASYNC
	PbaseTaskCtx task_ctx=NULL;
	PDNSCtx pdnsCtx;
	
		if(!onComplete)goto SyncStart;
	task_ctx=(PbaseTaskCtx)make_task(gethostbyname_r_2_taskEntry,sizeof(DNSCtx),GETHOSTBYNAME_STACK_SIZE,4321);
	pdnsCtx=(PDNSCtx)task_ctx;
	//register arguments!!
	pdnsCtx->if_name=if_name;pdnsCtx->name=name;
	pdnsCtx->result_buf=&pdnsCtx->h;pdnsCtx->buf_in=pdnsCtx->buf;pdnsCtx->buflen=sizeof(pdnsCtx->buf);
	pdnsCtx->result=&pdnsCtx->hp;pdnsCtx->h_errnop=&pdnsCtx->h_errno_P; 
	pdnsCtx->serverchoice=serverchoice;pdnsCtx->dwTimeout=dwTimeout;pdnsCtx->nRetries=nRetries;
	//register callback
        pdnsCtx->userCtx=userCtx;pdnsCtx->onComplete=onComplete;
	start_task(task_ctx);
       return(struct hostent*)task_ctx;
#else	//SYNC
#endif
SyncStart:
	{
	static struct hostent h;
	static char buf[sizeof(struct in_addr) +
			sizeof(struct in_addr *)*2 +
			sizeof(char *)*(ALIAS_DIM) + 384/*namebuffer*/ + 32/* margin */];
	struct hostent *hp=NULL;
	
	gethostbyname_r_2(if_name,name, &h, buf, sizeof(buf), &hp, &h_errno, serverchoice);
	return hp;
	}
}
#endif


#if 1
#if 0
/* Description of data base entry for a single host.  */
struct hostent
{
  char *h_name;			/* Official name of host.  */
  char **h_aliases;		/* Alias list.  */
  int h_addrtype;		/* Host address type.  */
  int h_length;			/* Length of address.  */
  char **h_addr_list;		/* List of addresses from name server.  */
#define	h_addr	h_addr_list[0]	/* Address, for backward compatibility.  */
};
#endif
int dup_hostentry(struct hostent* pheIn,struct hostent* pheOut,char buf[sizeof(struct in_addr) +
			sizeof(struct in_addr *)*2 +
			sizeof(char *)*(ALIAS_DIM) + 384/*namebuffer*/ + 32/* margin */],int buflen,int * h_errnop)
{
        int nAlias=0,i;
	struct in_addr *in;
	//struct in_addr **addr_list;
	char **alias;
	struct resolv_answer a;

DPRINTF("dup_hostentry:check buf\n");fflush(stdout);
        if(!pheIn||!buf||buflen<=0)return ERANGE;
        memset(pheOut,0,sizeof(*pheIn));
        memset(buf,0,buflen);

DPRINTF("dup_hostentry:check type(pheIn=0x%x,pheOut=0x%x,h_name=%s,h_aliases=0x%x,h_addr_list=0x%x,h_addrtype=%d,h_length=%d)\n",pheIn,pheOut,pheIn->h_name,pheIn->h_aliases,pheIn->h_addr_list,pheIn->h_addrtype,pheIn->h_length);fflush(stdout);
	a.atype=(pheIn->h_addrtype==AF_INET? T_A:-9999);
	pheOut->h_addrtype=pheIn->h_addrtype;//used
	pheOut->h_length=pheIn->h_length;//used
	//pheOut->h_addr_list[0]//used
	if(a.atype!=T_A){
		if(h_errnop)*h_errnop = HOST_NOT_FOUND;
                return TRY_AGAIN;//HOST_NOT_FOUND
	}
        /*count add_count,alias*/
DPRINTF("dup_hostentry:check add_count,alias\n");fflush(stdout);
	a.add_count=0;
	if(pheIn->h_addr_list){
        	while(pheIn->h_addr_list[a.add_count++])DPRINTF("%x ",pheIn->h_addr_list[a.add_count-1]);//+ 2
        	if(pheIn->h_addr_list[1]&&pheIn->h_addr_list[0])
                	a.rdlength=(unsigned long)pheIn->h_addr_list[1]-(unsigned long)pheIn->h_addr_list[0];//+ sizeof(struct in_addr*)
                else    a.rdlength=sizeof(struct in_addr);//pheIn->h_length?
        }
        else DPRINTF("dup_hostentry:h_addr_list NULL\n");
        nAlias=0;if(pheIn->h_aliases)while(pheIn->h_aliases[nAlias++]);else DPRINTF("h_aliases NULL\n");

DPRINTF("dup_hostentry: a.add_count=%d,a.rdlength=%d,nAlias=%d\n",a.add_count,a.rdlength,nAlias);
        /*copy*/
DPRINTF("dup_hostentry:copy h_addr_list\n");fflush(stdout);
	in=(struct in_addr*)buf;
	buf+=sizeof(*in);
	buflen-=sizeof(*in);
        pheOut->h_addr_list=(char**)buf;//pointer base
        buf+=sizeof(pheOut->h_addr_list[0])*a.add_count;buflen-=sizeof(pheOut->h_addr_list[0])*a.add_count;
        if(buflen<=0)return ERANGE;
DPRINTF("dup_hostentry:copy h_addr_list contents\n");fflush(stdout);
	pheOut->h_addr_list[0]=(char*)in;
	pheOut->h_addr_list[1]=NULL;
	if(a.rdlength>0){
                for(i=0;i<a.add_count-1;i++){
                        pheOut->h_addr_list[i]=buf;//pointer val
                        buf+=a.rdlength;buflen-=a.rdlength;
                        if(buflen<=0)return ERANGE;
                        DPRINTF("h_addr_list[%d]=%s\n",i,(char*)inet_ntoa(*(struct in_addr*)pheIn->h_addr_list[i]));//(char*)inet_ntoa(*(struct in_addr*)hp->h_addr)
                        memcpy(pheOut->h_addr_list[i],pheIn->h_addr_list[i],a.rdlength);//pointed data
                }
                pheOut->h_addr_list[i]=NULL;
        }
        else DPRINTF("dup_hostentry:a.rdlength 0\n");
DPRINTF("dup_hostentry:copy h_aliases\n");fflush(stdout);
        pheOut->h_aliases=(char**)buf;//pointer base
        buf+=sizeof(char *)*nAlias;buflen-=sizeof(char *)*nAlias;
        if(buflen<=0)return ERANGE;
DPRINTF("dup_hostentry:copy h_aliases contents\n");fflush(stdout);
        for(i=0;i<nAlias-1;i++){
                pheOut->h_aliases[i]=buf;//pointer val
                buf+=strlen(pheIn->h_aliases[i])+1;buflen-=strlen(pheIn->h_aliases[i])+1;
                if(buflen<=0)return ERANGE;
                DPRINTF("h_aliases[%d]=%s(%d)\n",i,pheOut->h_aliases[i],strlen(pheOut->h_aliases[i]));
                strcpy(pheOut->h_aliases[i],pheIn->h_aliases[i]);//pointed data
        }
        pheOut->h_aliases[i]=NULL;//pointer val
DPRINTF("dup_hostentry:copy h_name\n");fflush(stdout);
        pheOut->h_name=buf;//pointer val
        buf+=strlen(pheIn->h_name)+1;buflen-=strlen(pheIn->h_name)+1;
        if(buflen<=0)return ERANGE;
        if(pheIn->h_name)strcpy((char*)pheOut->h_name/*redirected to buf*/,pheIn->h_name);//pointed data
        else DPRINTF("dup_hostentry: h_name NULL\n");
	    if(h_errnop)*h_errnop = NETDB_SUCCESS;
	return NETDB_SUCCESS;
}
#endif

#endif
