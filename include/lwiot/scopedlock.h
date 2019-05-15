/*
 * Scoped lock definition.
 * 
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#pragma once

#include <lwiot.h>
#include <stdlib.h>

#include <lwiot/kernel/lock.h>
#include <lwiot/log.h>
#include <lwiot/stl/referencewrapper.h>

namespace lwiot
{
	class ScopedLock {
	public:
		explicit ScopedLock(Lock& lock);
		ScopedLock(Lock* lock);
		virtual ~ScopedLock();

		ScopedLock& operator=(const ScopedLock& lock) = delete;

		void lock() const;
		void unlock() const;

	private:
		stl::ReferenceWrapper<Lock> _lock;
		mutable bool _locked;
	};
}
