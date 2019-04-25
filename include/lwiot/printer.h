/*
 * Printer interface.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#pragma once

#include <lwiot.h>
#include <stdlib.h>

#include <lwiot/types.h>
#include <lwiot/error.h>
#include <lwiot/stl/string.h>

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

namespace lwiot
{
	class Printer;
	class Printable
	{
	public:
		virtual ~Printable() = default;
		virtual size_t printTo(Printer& p) const = 0;
	};

	class Printer {
	public:
		explicit Printer() : write_error(0)
		{
		}

		virtual ~Printer() = default;

		int getWriteError()
		{
			return write_error;
		}

		void clearWriteError()
		{
			setWriteError(0);
		}

		virtual size_t write(uint8_t byte) = 0;

		size_t write(const char *str)
		{
			if(str == NULL) {
				return 0;
			}
			return write((const uint8_t *) str, strlen(str));
		}

		virtual size_t write(const uint8_t *buffer, size_t size);

		size_t write(const char *buffer, size_t size)
		{
			return write((const uint8_t *) buffer, size);
		}

#ifdef WIN32
		size_t printf(const char * format, ...);
#else
		size_t printf(const char * format, ...)  __attribute__ ((format (printf, 2, 3)));
#endif
		size_t print(const String &);
		size_t print(const char[]);
		size_t print(char);
		size_t print(unsigned char, int = DEC);
		size_t print(int, int = DEC);
		size_t print(unsigned int, int = DEC);
		size_t print(long, int = DEC);
		size_t print(unsigned long, int = DEC);
		size_t print(double, int = 2);
		size_t print(const Printable&);
		size_t print(struct tm * timeinfo, const char * format = NULL);

		size_t println(const String &s);
		size_t println(const char[]);
		size_t println(char);
		size_t println(unsigned char, int = DEC);
		size_t println(int, int = DEC);
		size_t println(unsigned int, int = DEC);
		size_t println(long, int = DEC);
		size_t println(unsigned long, int = DEC);
		size_t println(double, int = 2);
		size_t println(const Printable&);
		size_t println(struct tm * timeinfo, const char * format = NULL);
		size_t println(void);

	private:
		int write_error;
		size_t printNumber(unsigned long, uint8_t);
		size_t printFloat(double, uint8_t);

	protected:
		void setWriteError(int err = 1)
		{
			write_error = err;
		}
	};
}
