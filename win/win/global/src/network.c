#include "network.h"
#include "utils.h"

/* const char *
* inet_ntop4(src, dst, size)
*	format an IPv4 address
* return:
*	`dst' (as a const)
* notes:
*	(1) uses no statics
*	(2) takes a u_char* not an in_addr as input
* author:
*	Paul Vixie, 1996.
*/
static char * inet_ntop4(const u_char *src, char *dst, socklen_t size)
{
	static const char fmt[128] = "%u.%u.%u.%u";
	char tmp[sizeof "255.255.255.255"];
	int l;

	l = _snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]); // ****
	if (l <= 0 || (socklen_t)l >= size) {
		return (NULL);
	}
	strcpy_s(dst, size, tmp);
	return (dst);
}

/* const char *
* inet_ntop6(src, dst, size)
*	convert IPv6 binary address into presentation (printable) format
* author:
*	Paul Vixie, 1996.
*/
static char * inet_ntop6(const u_char *src, char *dst, socklen_t size)
{
	/*
	* Note that int32_t and int16_t need only be "at least" large enough
	* to contain a value of the specified size.  On some systems, like
	* Crays, there is no such thing as an integer variable with 16 bits.
	* Keep this in mind if you think this function should have been coded
	* to use pointer overlays.  All the world's not a VAX.
	*/
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
	struct { int base, len; } best, cur;
#define NS_IN6ADDRSZ	16
#define NS_INT16SZ	2
	u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i;

	/*
	* Preprocess:
	*	Copy the input (bytewise) array into a wordwise array.
	*	Find the longest run of 0x00's in src[] for :: shorthanding.
	*/
	memset(words, '\0', sizeof words);
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	best.len = 0;
	cur.base = -1;
	cur.len = 0;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		}
		else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	/*
	* Format the result.
	*/
	tp = tmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
			i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0)
			*tp++ = ':';
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 && (best.len == 6 ||
			(best.len == 7 && words[7] != 0x0001) ||
			(best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src + 12, tp, sizeof tmp - (tp - tmp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		tp += _snprintf(tp, 1, "%x", words[i]); // ****
	}
	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) ==
		(NS_IN6ADDRSZ / NS_INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	/*
	* Check for overflow, copy, and we're done.
	*/
	if ((socklen_t)(tp - tmp) > size) {
		return (NULL);
	}
	strcpy_s(dst, size, tmp);
	return (dst);
}

char * inet_ntop_win32(int af, const void *src, char *dst, socklen_t size)
{
	switch (af) {
	case AF_INET:
		return (inet_ntop4((unsigned char*)src, (char*)dst, size));			// ipv4
	case AF_INET6:
		return (char*)(inet_ntop6((unsigned char*)src, (char*)dst, size));	// ipv6
	default:
		return 0;
	}
}
