/*
 * SPI message definition.
 * 
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#pragma once

#include <stdlib.h>
#include <lwiot.h>

#include <lwiot/log.h>
#include <lwiot/stream.h>
#include <lwiot/types.h>
#include <lwiot/bytebuffer.h>

#include <lwiot/io/gpiopin.h>

#ifdef __cplusplus
namespace lwiot
{
	class SpiMessage {
	public:
		explicit SpiMessage(const size_t& size, int cs = 0);
		explicit SpiMessage(const size_t& size, const GpioPin& pin);
		virtual ~SpiMessage() = default;

		ByteBuffer& rxdata();
		ByteBuffer& txdata();

		SpiMessage& operator <<(uint8_t byte);
		SpiMessage& operator<<(const uint8_t *msg);

		SpiMessage& operator >>(uint8_t& byte);

		const size_t& size() const;
		const GpioPin& cspin() const;

	private:
		ByteBuffer _tx;
		ByteBuffer _rx;
		size_t _idx;
		size_t _size;
		GpioPin _cspin;
	};
}
#endif
