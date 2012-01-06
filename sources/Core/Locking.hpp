/**************************************************************************
 *   Created: 2008/01/12 3:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Locking_h__0801120308
#define INCLUDED_FILE__TUNNELEX__Locking_h__0801120308

#include "Api.h"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	struct TUNNELEX_CORE_API Interlocked {

		static long Increment(long volatile *) throw();
		static long Increment(long volatile &) throw();

		static long Decrement(long volatile *) throw();
		static long Decrement(long volatile &) throw();

		static long Exchange(long volatile &destination, long value) throw();

		static long CompareExchange(
				long volatile &destination,
				long exchangeValue,
				long compareValue)
			throw();

	};

	//////////////////////////////////////////////////////////////////////////

	//! Recursive mutex.
	class TUNNELEX_CORE_API RecursiveMutex {
	public:
		RecursiveMutex();
		~RecursiveMutex();
	private:
		RecursiveMutex(const RecursiveMutex &);
		const RecursiveMutex & operator =(const RecursiveMutex &);
	public:
		void Acquire() throw();
		void Release() throw();
	private:
		class Implementation;
		Implementation *const m_pimpl;
	};

	//////////////////////////////////////////////////////////////////////////

	namespace Helpers {
		
		class TolerantSpinWait {
		public:
			TolerantSpinWait()
					: m_iterationsCount(0) {
				//...//
			}
		private:
			TolerantSpinWait(const TolerantSpinWait &);
			const TolerantSpinWait & operator =(const TolerantSpinWait &);
		public:
			void SpinOnce() {
				if (!(++m_iterationsCount % 10)) {
					Sleep();
				}
			}
		private:
			void Sleep();
		private:
			size_t m_iterationsCount;
		};

		class AggressiveSpinWait {
		public:
#			ifdef DEV_VER
				AggressiveSpinWait()
						: m_iterationsCount(0) {
					//...//
				}
#			endif
		private:
			AggressiveSpinWait(const AggressiveSpinWait &);
			const AggressiveSpinWait & operator =(const AggressiveSpinWait &);
		public:
			void SpinOnce() {
#				ifdef DEV_VER
					if (!(++m_iterationsCount % 100)) {
						Report();
					}
#				endif
			}
		private:
#			ifdef DEV_VER
				void Report() const;
#			endif
		private:
#			ifdef DEV_VER
				size_t m_iterationsCount;
#			endif
		};

		template<bool isAggressive>
		struct FlagToSpinWait {
			static_assert(isAggressive, "Failed to specialize spin type.");
			typedef ::TunnelEx::Helpers::AggressiveSpinWait SpinWait;
		};

		template<>
		struct FlagToSpinWait<false> {
			typedef ::TunnelEx::Helpers::TolerantSpinWait SpinWait;
		};

	}

	//////////////////////////////////////////////////////////////////////////

	template<bool isAggressiveWait>
	class SpinMutex {

	private:

		typedef typename ::TunnelEx::Helpers::FlagToSpinWait<isAggressiveWait>::SpinWait
			SpinWait;

	public:

		SpinMutex()
				: m_isLocked(0) {
			//...//
		}

		~SpinMutex() {
			assert(!m_isLocked);
		}

	private:

		SpinMutex(const SpinMutex &);
		const SpinMutex & operator =(const SpinMutex &);

	public:

		void Acquire() throw() {
			for (SpinWait wait; ; wait.SpinOnce()) {
				if (::TunnelEx::Interlocked::CompareExchange(m_isLocked, 1, 0) == 0) {
					return;
				}
			}
		}

		void Release() throw() {
			verify(::TunnelEx::Interlocked::Exchange(m_isLocked, 0) == 1);
		}

	private:

		volatile long m_isLocked;

	};

	//////////////////////////////////////////////////////////////////////////

	template<bool isAggressiveWait>
	class ReadWriteSpinMutex {

	private:

		typedef typename ::TunnelEx::Helpers::FlagToSpinWait<isAggressiveWait>::SpinWait
			SpinWait;

		enum Flags {
			FLAG_WRITER			= 0x80000000,
			FLAG_WRITER_WAIT	= 0x40000000,
			FLAG_WRITERS		= FLAG_WRITER | FLAG_WRITER_WAIT,
			FLAG_READER			= ~FLAG_WRITER
		};

	public:

		ReadWriteSpinMutex()
				: m_state(0) {
			//...//
		}

		~ReadWriteSpinMutex() {
			assert(m_state == 0);
		}

	private:

		ReadWriteSpinMutex(const ReadWriteSpinMutex &);
		const ReadWriteSpinMutex & operator =(const ReadWriteSpinMutex &);

	public:

		void AcquireWrite() throw() {
			for (SpinWait wait; ; wait.SpinOnce()) {
                const auto state = m_state;
                if (	(state == 0 || state == FLAG_WRITER_WAIT)
						&& :TunnelEx::Interlocked::CompareExchange(
								m_state, FLAG_WRITER, state)
							== state) {
                    return;
				}
                if ((state & FLAG_WRITER_WAIT) == 0) {
                    :TunnelEx::Interlocked::CompareExchange(
						m_state,
						state | FLAG_WRITER_WAIT,
						state);
				}
            }
		}

		void ReleaseWrite() throw() {
			Interlocked::Exchange(m_state, 0 | (m_state & FLAG_WRITER_WAIT));
		}

		void AcquireRead() throw() {
			for (SpinWait wait; ; wait.SpinOnce()) {
				const auto state = m_state;
				if (!(state & FLAG_WRITER)) {
					if (:TunnelEx::Interlocked::CompareExchange(m_state, state + 1, state) == state) {
						return;
					}
				}
			}
		}

		void ReleaseRead() throw() {
			for (SpinWait spin; ; spin.SpinOnce()) {
				const auto state = m_state;
				assert(state & FLAG_READER);
				if (!(state & FLAG_READER)) {
					return;
				}
				if (:TunnelEx::Interlocked::CompareExchange(
							m_state,
							((state & FLAG_READER) - 1) | (state & FLAG_WRITER_WAIT),
							state)
						== state) {
					return;
				}
			}
		}

	private:

		volatile long m_state;

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename MutexT = RecursiveMutex>
	class Lock {
	public:
		typedef MutexT Mutex;
	public:
		explicit Lock(Mutex &mutexRef)
				: m_mutex(mutexRef) {
			m_mutex.Acquire();
		}
		~Lock() {
			m_mutex.Release();
		}
	public:
		Lock(const Lock &);
		const Lock & operator =(const Lock &);
	private:
		Mutex &m_mutex;
	};

	template<typename MutexT>
	class ReadLock {
	public:
		typedef MutexT Mutex;
	public:
		explicit ReadLock(Mutex &mutexRef)
				: m_mutex(mutexRef) {
			m_mutex.AcquireRead();
		}
		~ReadLock() {
			m_mutex.ReleaseRead();
		}
	public:
		ReadLock(const ReadLock &);
		const ReadLock & operator =(const ReadLock &);
	private:
		Mutex &m_mutex;
	};

	typedef ::TunnelEx::Lock<RecursiveMutex> RecursiveLock;
	
	template<typename MutexT>
	class WriteLock {
	public:
		typedef MutexT Mutex;
	public:
		explicit WriteLock(Mutex &mutexRef)
				: m_mutex(mutexRef) {
			m_mutex.AcquireWrite();
		}
		~WriteLock() {
			m_mutex.ReleaseWrite();
		}
	public:
		WriteLock(const WriteLock &);
		const WriteLock & operator =(const WriteLock &);
	private:
		Mutex &m_mutex;
	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__Locking_h__0801120308
