/*
 * TCP client wrapper.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <lwiot.h>

#include <lwiot/types.h>
#include <lwiot/log.h>
#include <lwiot/stl/string.h>
#include <lwiot/network/tcpclient.h>
#include <lwiot/error.h>
#include <lwiot/network/stdnet.h>

namespace lwiot
{
	TcpClient::TcpClient() : _remote_addr((uint32_t)0), _remote_port(0)
	{
	}

	TcpClient::TcpClient(const lwiot::IPAddress &addr, uint16_t port) : _remote_addr(addr), _remote_port(to_netorders(port))
	{
	}

	TcpClient::TcpClient(const lwiot::String &host, uint16_t port) : _remote_addr((uint32_t)0), _remote_port(to_netorders(port))
	{
	}

	TcpClient& TcpClient::operator=(lwiot::TcpClient &&client)
	{
		*this = client;
		return *this;
	}

	TcpClient& TcpClient::operator=(const lwiot::TcpClient &client)
	{
		this->_remote_port = client._remote_port;
		this->_remote_addr = client._remote_addr;
		return *this;
	}

	TcpClient::operator bool() const
	{
		return this->connected();
	}

	bool TcpClient::operator==(const lwiot::TcpClient &other)
	{
		return false;
	}

	bool TcpClient::operator!=(const lwiot::TcpClient &other)
	{
		return false;
	}

	const IPAddress& TcpClient::remote() const
	{
		return this->_remote_addr;
	}

	uint16_t TcpClient::port() const
	{
		return to_netorders(this->_remote_port);
	}

	uint8_t TcpClient::read()
	{
		uint8_t tmp;

		auto rc = this->read(&tmp, sizeof(tmp));
		if(rc < 0)
			return 0;

		return tmp;
	}

	bool TcpClient::write(uint8_t byte)
	{
		return this->write(&byte, sizeof(byte)) == sizeof(byte);
	}

	Stream& TcpClient::operator<<(char x)
	{
		this->write((uint8_t)x);
		return *this;
	}

	Stream& TcpClient::operator<<(short x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(int x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(const long & x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(const long long& x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(unsigned char x)
	{
		this->write((uint8_t)x);
		return *this;
	}

	Stream& TcpClient::operator<<(unsigned short x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(unsigned int x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(unsigned const long & x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(const unsigned long long& x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(const float &x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(const double &x)
	{
		this->write((uint8_t*)&x, sizeof(x));
		return *this;
	}

	Stream& TcpClient::operator<<(const lwiot::String &str)
	{
		*this << str.c_str();
		return *this;
	}

	Stream& TcpClient::operator<<(const char *cstr)
	{
		this->write(reinterpret_cast<const uint8_t *>(cstr), strlen(cstr));
		return *this;
	}
}
