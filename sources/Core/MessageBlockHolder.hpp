
#ifndef MESSAGE_BLOCK_ADAPTER_H_INCLUDED
#define MESSAGE_BLOCK_ADAPTER_H_INCLUDED

#include "MessageBlock.hpp"
#include "Locking.hpp"
#include "Exceptions.hpp"

namespace TunnelEx {

	class MessagesAllocator;

	//////////////////////////////////////////////////////////////////////////

	class UniqueMessageBlockHolder : public MessageBlock {

	public:

		class Satellite : private boost::noncopyable {

		public:

			class Timings : private boost::noncopyable {

			public:

				struct LatencyPolicy {

					typedef boost::posix_time::time_duration::tick_type StatValueType;

					static StatValueType GetStatValue(
								const boost::posix_time::time_duration &);
					static boost::posix_time::ptime GetCurrentTime();

				};

			public:

				void SetReceivingStartTimePoint();

				void SetReceivingTimePoint();
				const boost::posix_time::ptime & GetReceivingTime() const;

				void SetSendingStartTimePoint();
				const boost::posix_time::ptime & GetSendingStartTime() const;

				void SetSendingTimePoint();

			public:

				boost::posix_time::time_duration GetReceivingLatency() const;
				boost::posix_time::time_duration GetSendingLatency() const;
				boost::posix_time::time_duration GetProcessingLatency() const;
				boost::posix_time::time_duration GetFullLatency() const;

			private:

				static void SetCurrentTimePoint(boost::posix_time::ptime &);

			private:

				boost::posix_time::ptime m_receivingStartTime;
				boost::posix_time::ptime m_receivingTime;
				boost::posix_time::ptime m_sendingStartTime;
				boost::posix_time::ptime m_sendingTime;

			};

			class Lock : public ACE_Lock {
			public:
				virtual ~Lock();
			public:
				virtual int acquire();
				virtual int remove();
				virtual int tryacquire();
				virtual int release();
				virtual int acquire_read();
				virtual int acquire_write();
				virtual int tryacquire_read();
				virtual int tryacquire_write();
				virtual int tryacquire_write_upgrade();
			private:
				SpinMutex<true> m_mutex;
			};

		public:

			explicit Satellite(boost::shared_ptr<MessagesAllocator>);
			~Satellite() throw();

		public:

#			ifdef DEV_VER
				static long GetInstancesNumber();
#			endif

		public:

			long AddRef() throw();
			long RemoveRef() throw();

			const MessagesAllocator & GetAllocator() const throw();
			MessagesAllocator & GetAllocator() throw();
			boost::shared_ptr<MessagesAllocator> GetAllocatorPtr();

		public:

			Timings & GetTimings();
			const Timings & GetTimings() const;

			Lock & GetLock();
			const Lock & GetLock() const;

		private:

			volatile long m_refsCount;
			boost::shared_ptr<MessagesAllocator> m_allocators;

			Timings m_timings;
			Lock m_lock;

			TUNNELEX_OBJECTS_DELETION_CHECK_DECLARATION(m_instancesNumber);

		};

		typedef Satellite::Timings Timings;
		typedef Satellite::Lock Lock;

	public:

		explicit UniqueMessageBlockHolder() throw();
		explicit UniqueMessageBlockHolder(ACE_Message_Block *) throw();
		explicit UniqueMessageBlockHolder(ACE_Message_Block &) throw();

		virtual ~UniqueMessageBlockHolder() throw();

	public:

		void Reset(ACE_Message_Block * = nullptr) throw();

		void Release() throw();

		bool IsSet() const throw();

	public:

		ACE_Message_Block & Get() throw();

		const ACE_Message_Block & Get() const throw();

	public:

		virtual const char * GetData() const throw();

		virtual char * GetWritableSpace(size_t size);
		virtual void TakeWritableSpace(size_t size);

		virtual size_t GetUnreadedDataSize() const throw();

		virtual void SetData(const char *data, size_t length);

		virtual void MarkAsAddedToQueue() throw();
		virtual bool IsAddedToQueue() const throw();

		virtual bool IsTunnelMessage() const throw();

	public:

		static size_t GetMessageMemorySize(size_t clientSize);

		double GetUsage() const;

	public:

		ACE_Message_Block & Duplicate();

		static ACE_Message_Block & Create(
					size_t size,
					boost::shared_ptr<MessagesAllocator>,
					bool isTunnelMessage);

		static void Delete(ACE_Message_Block &messageBlock) throw();

	public:

		void SetReceivingStartTimePoint();
		
		void SetReceivingTimePoint();
		const boost::posix_time::ptime & GetReceivingTime() const;

		void SetSendingStartTimePoint();
		const boost::posix_time::ptime & GetSendingStartTime() const;
		
		void SetSendingTimePoint();

#		ifdef DEV_VER
			static long GetSatellitesInstancesNumber();
#		endif

	public:

		boost::posix_time::time_duration GetReceivingLatency() const;
		boost::posix_time::time_duration GetSendingLatency() const;
		boost::posix_time::time_duration GetProcessingLatency() const;
		boost::posix_time::time_duration GetFullLatency() const;

	private:

		static Satellite & GetSatellite(ACE_Message_Block &messageBlock);
		Satellite & GetSatellite();
		const Satellite & GetSatellite() const;

		static Timings & GetTimings(ACE_Message_Block &messageBlock);
		Timings & GetTimings();
		const Timings & GetTimings() const;

	private:

		ACE_Message_Block *m_messageBlock;
		bool m_isAddedToQueue;

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // #ifndef MESSAGE_BLOCK_ADAPTER_H_INCLUDED
