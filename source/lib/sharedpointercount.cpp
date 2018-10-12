/*
 * Smart pointer using reference counting.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <lwiot.h>

#include <lwiot/types.h>
#include <lwiot/log.h>
#include <lwiot/sharedpointer.h>

namespace lwiot
{
	SharedPointerCount::SharedPointerCount() : count(0)
	{
	}

	SharedPointerCount::SharedPointerCount(const lwiot::SharedPointerCount &count) = default;

	void SharedPointerCount::swap(lwiot::SharedPointerCount &count) noexcept
	{
		lwiot::lib::swap(this->count, count.count);
	}

	long SharedPointerCount::useCount() const
	{
		return this->count.value();
	}
}
