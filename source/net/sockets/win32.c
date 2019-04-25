/*
 * Standard socket implementation for UNIX.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include "unix_sockets.h"

#include <stdlib.h>
#include <stdio.h>
#include <lwiot.h>
#include <assert.h>

#include <lwiot/log.h>
#include <lwiot/error.h>

#include <lwiot/network/stdnet.h>

#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define IP6_SIZE 16

#ifndef CONFIG_CLIENT_QUEUE_LENGTH
#define CONFIG_CLIENT_QUEUE_LENGTH 10
#endif

static bool ip4_connect(socket_t* sock, remote_addr_t* addr)
{
	struct sockaddr_in sockaddr;
	const char enable = 1;

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = addr->port;
	sockaddr.sin_addr.s_addr = addr->addr.ip4_addr.ip;
	memset(&sockaddr.sin_zero, 0, sizeof(sockaddr.sin_zero));

	*sock = socket(AF_INET, SOCK_STREAM, 0);
	if(*sock < 0)
		return false;

	setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	if(connect(*sock, (struct sockaddr*) &sockaddr, sizeof(struct sockaddr)) < 0) {
		closesocket(*sock);
		return false;
	}

	return true;
}

static bool ip6_connect(socket_t* sock, remote_addr_t* addr)
{
	print_dbg("IPv6 not yet supported!");
	return false;
}

socket_t* tcp_socket_create(remote_addr_t* remote)
{
	socket_t *sock;
	bool result;

	sock = lwiot_mem_zalloc(sizeof(sock));
	assert(sock);

	if(remote->version == 6)
		result = ip6_connect(sock, remote);
	else
		result = ip4_connect(sock, remote);

	if(!result) {
		lwiot_mem_free(sock);
		return NULL;
	}

	return sock;
}

void socket_set_timeout(socket_t *sock, int tmo)
{
	assert(sock);
	setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tmo, sizeof(int));
}

ssize_t tcp_socket_send(socket_t* socket, const void* data, size_t length)
{
	int fd;

	assert(socket);
	assert(data);

	fd = *socket;

	if(length == 0)
		return 0;

	return send(fd, data, length, 0);
}

ssize_t tcp_socket_read(socket_t* socket, void* data, size_t length)
{
	int fd;

	assert(socket);
	assert(data);

	fd = *socket;
	if(length == 0)
		return 0;

	return recv(fd, data, length, 0);
}

socket_t* udp_socket_create(remote_addr_t* remote)
{
	socket_t *sock;
	int fd;

	sock = lwiot_mem_zalloc(sizeof(sock));
	assert(sock);

	if(remote->version == 6) {
		fd = socket(PF_INET6, SOCK_DGRAM, 0);
	} else {
		fd = socket(PF_INET, SOCK_DGRAM, 0);
	}

	if(fd < 0) {
		lwiot_mem_free(sock);
		return NULL;
	}

	*sock = fd;

	return sock;
}

ssize_t udp_send_to(socket_t* socket, const void *data, size_t length, remote_addr_t* remote)
{
	struct sockaddr_in ip;
	struct sockaddr_in6 ip6;
	ssize_t rv;

	if(remote->version == 6) {
		memcpy(ip6.sin6_addr.u.Byte, remote->addr.ip6_addr.ip, IP6_SIZE);
		ip6.sin6_family = AF_INET6;
		ip6.sin6_port = remote->port;
		rv = sendto(*socket, data, length, 0, (struct sockaddr*)&ip6, sizeof(ip6));
	} else {
		ip.sin_family = AF_INET;
		ip.sin_port = remote->port;
		ip.sin_addr.s_addr = remote->addr.ip4_addr.ip;
		rv = sendto(*socket, data, length, 0, (struct sockaddr*)&ip, sizeof(ip));
	}

	return rv;
}

ssize_t udp_recv_from(socket_t* socket, void *data, size_t length, remote_addr_t* remote)
{
	struct sockaddr_in ip;
	struct sockaddr_in6 ip6;
	ssize_t rv;
	socklen_t socklen;

	if(remote->version == 6) {
		socklen = sizeof(ip6);
		rv = recvfrom(*socket, data, length, 0, (struct sockaddr*)&ip6, &socklen);

		if(rv < 0)
			return rv;

		remote->port = ip6.sin6_port;
		memcpy(remote->addr.ip6_addr.ip, ip6.sin6_addr.u.Byte, IP6_SIZE);
	} else {
		socklen = sizeof(ip);
		rv = recvfrom(*socket, data, length, 0, (struct sockaddr*)&ip, &socklen);

		if(rv < 0)
			return rv;

		remote->port = ip.sin_port;
		remote->addr.ip4_addr.ip = ip.sin_addr.s_addr;
	}

	return rv;
}

void socket_close(socket_t* socket)
{
	assert(socket);
	closesocket(*socket);
	lwiot_mem_free(socket);
}

/* SERVER OPS */
socket_t* server_socket_create(socket_type_t type, bool ipv6)
{
	socket_t *sock;
	int fd;
	int domain;

	if(ipv6)
		domain = PF_INET6;
	else
		domain = AF_INET;

	if(type == SOCKET_DGRAM) {
		fd = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
	} else {
		fd = socket(domain, SOCK_STREAM, IPPROTO_TCP);
	}

	if(fd < 0) {
		wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
		return NULL;
	}

	sock = lwiot_mem_zalloc(sizeof(sock));
	*sock = fd;

	return sock;
}

static size_t socket_available(const socket_t* socket)
{
	unsigned long count;
	int fd;

	assert(socket);

	count = 0;
	fd = *socket;

	ioctlsocket(fd, FIONREAD, &count);
	return count;
}

size_t tcp_socket_available(socket_t* socket)
{
	return socket_available(socket);
}

size_t udp_socket_available(socket_t* socket)
{
	return socket_available(socket);
}

static bool bind_ipv4(const socket_t* sock, remote_addr_t* addr, uint16_t port)
{
	int fd;
	struct sockaddr_in server;

	fd = *sock;
	assert(fd >= 0);

	server.sin_port = port;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = addr->addr.ip4_addr.ip;

	return bind(fd, (struct sockaddr*)&server, sizeof(server)) < 0 ? false : true;
}

static bool bind_ipv6(const socket_t* sock, remote_addr_t* addr, uint16_t port)
{
	int fd;
	struct sockaddr_in6 server;

	fd = *sock;
	assert(fd >= 0);

	server.sin6_port = port;
	server.sin6_family = AF_INET6;
	memcpy(server.sin6_addr.u.Byte, addr->addr.ip6_addr.ip, sizeof(addr->addr.ip6_addr.ip));

	return bind(fd, (struct sockaddr*)&server, sizeof(server)) < 0 ? false : true;
}

bool server_socket_bind_to(socket_t* sock, remote_addr_t* remote, uint16_t port)
{
	if(remote->version == 6) {
		return bind_ipv6(sock, remote, port);
	}

	return bind_ipv4(sock, remote, port);
}

bool server_socket_bind(socket_t* sock, bind_addr_t addr, uint16_t port)
{
	remote_addr_t remote;
	assert(sock);

	switch(addr) {
	default:
	case BIND_ADDR_ANY:
		remote.addr.ip4_addr.ip = INADDR_ANY;
		return bind_ipv4(sock, &remote, port);

	case BIND_ADDR_LB:
		remote.addr.ip4_addr.ip = INADDR_LOOPBACK;
		return bind_ipv4(sock, &remote, port);

	case BIND6_ADDR_ANY:
		memcpy(remote.addr.ip6_addr.ip, in6addr_any.u.Byte, sizeof(remote.addr.ip6_addr.ip));
		return bind_ipv6(sock, &remote, port);
	}
}

bool server_socket_listen(socket_t *socket)
{
	int rv;

	assert(socket);
	rv = listen(*socket, CONFIG_CLIENT_QUEUE_LENGTH);

	return rv < 0 ? false : true;
}

socket_t* server_socket_accept(socket_t* socket)
{
	int sock;
	socket_t *client;

	assert(socket);
	sock = accept(*socket, NULL, NULL);

	if(sock < 0)
		return NULL;

	client = lwiot_mem_zalloc(sizeof(socket));
	assert(client);
	*client = sock;
	return client;
}

/* DNS */
int dns_resolve_host(const char *host, remote_addr_t* addr)
{
	struct addrinfo hints, *res, *p;
	int ai_family;

	ai_family = addr->version == 6 ? AF_INET6 : AF_INET;
	ai_family = addr->version == 0 ? AF_UNSPEC : ai_family;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = ai_family;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, NULL, &hints, &res) != 0) {
		return -1;
	}

	p = res;
	if (p->ai_family == AF_INET) { // IPv4
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		addr->addr.ip4_addr.ip = ipv4->sin_addr.s_addr;
		addr->version = 4;
	} else { // IPv6
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
		memcpy(addr->addr.ip6_addr.ip, ipv6->sin6_addr.u.Byte, IP6_SIZE);
		addr->version = 6;
	}

	freeaddrinfo(res); // free the linked list
	return -EOK;
}
