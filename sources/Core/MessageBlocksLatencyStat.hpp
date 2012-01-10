/**************************************************************************
 *   Created: 2012/1/5/ 2:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__MessageBlocksLatencyStat_201215252
#define INCLUDED_FILE__TUNNELEX__MessageBlocksLatencyStat_201215252

#include "LatencyStat.hpp"
#include "MessageBlockHolder.hpp"
#include "Locking.hpp"
#include "Log.hpp"
#include "Instance.hpp"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	class MessageBlocksLatencyStat : private boost::noncopyable {

	public:

		typedef UniqueMessageBlockHolder::Satellite::Timings::LatencyPolicy
			TimingsPolicy;
		typedef TimingsPolicy::StatValueType TimingValueType;
		typedef double PercentValueType;

	private:

		typedef SpinMutex Mutex;
		typedef Lock<Mutex> Lock;

		typedef LatencyStat<TimingValueType> TimingsStat;
		typedef LatencyStat<PercentValueType> PercentsStat;

		template<typename StatT>
		struct Bundle : private boost::noncopyable {

			typedef StatT Stat;
			typedef typename Stat::ValueType ValueType;

			Stat currentPeriod;
			Stat prevPeriod;
			Stat common;

			void Accumulate(bool isNewPeriod) {
				if (isNewPeriod) {
					prevPeriod = currentPeriod;
					currentPeriod.Reset();
				}
			}

			void Accumulate(const ValueType val, bool isNewPeriod) {
				Accumulate(isNewPeriod);
				common += val;
				currentPeriod += val;
			}

		};

		struct TimingsBundle : public Bundle<TimingsStat> {

			typedef Bundle<TimingsStat> Base;
			typedef Base::Stat Stat;
			typedef boost::posix_time::time_duration(
					UniqueMessageBlockHolder::*TimingsGetter)
				(void) const;

			TimingsGetter timingsGetter;

			explicit TimingsBundle(TimingsGetter timingsGetter)
					: timingsGetter(timingsGetter) {
				//...//
			}

			void Accumulate(
						const UniqueMessageBlockHolder &message,
						bool isNewPeriod) {
				const auto val = (message.*timingsGetter)();
				if (val.is_not_a_date_time()) {
					Base::Accumulate(isNewPeriod);
				} else {
					Base::Accumulate(TimingsPolicy::GetStatValue(val), isNewPeriod);
				}
			}

		};

		typedef Bundle<PercentsStat> PercentsBundle;

	public:

		explicit MessageBlocksLatencyStat(Instance::Id connectionId, size_t periodSec)
				: m_connectionId(connectionId),
				m_period(0, 0, periodSec),
				m_receiving(&UniqueMessageBlockHolder::GetReceivingLatency),
				m_sending(&UniqueMessageBlockHolder::GetSendingLatency),
				m_processing(&UniqueMessageBlockHolder::GetProcessingLatency),
				m_full(&UniqueMessageBlockHolder::GetFullLatency) {
			//...//
		}

	public:

		void Accumulate(
					const MessageBlock &message,
					PercentValueType queueBufferUsage = .0) {

			const Lock lock(m_mutex);

			const UniqueMessageBlockHolder &messageHolder
				= *boost::polymorphic_downcast<const UniqueMessageBlockHolder *>(
				&message);

			const auto &now = messageHolder.GetSendingTime();
			auto isNewPeriod = false;
			if (m_periodStart.is_not_a_date_time()) {
				m_periodStart = now;
			} else {
				isNewPeriod = (now - m_periodStart) > m_period;
			}

			m_receiving.Accumulate(messageHolder, isNewPeriod);
			m_sending.Accumulate(messageHolder, isNewPeriod);
			m_processing.Accumulate(messageHolder, isNewPeriod);
			m_full.Accumulate(messageHolder, isNewPeriod);
			m_blockBuffer.Accumulate(messageHolder.GetUsage(), isNewPeriod);
			if (queueBufferUsage) {
				m_queueBuffer.Accumulate(queueBufferUsage, isNewPeriod);
			} else {
				m_queueBuffer.Accumulate(isNewPeriod);
			}

			if (isNewPeriod) {
				DumpPeriod();
				m_periodStart = now;
			}

		}

		void Dump() const throw() {
			if (!Log::GetInstance().IsDebugRegistrationOn()) {
				return;
			}
			const Lock lock(m_mutex);
			try {
				DumpLatencyStat(
					true,
					m_receiving.common,
					m_sending.common,
					m_processing.common,
					m_full.common,
					m_blockBuffer.common,
					m_queueBuffer.common);
			} catch (...) {
				assert(false);
				Log::GetInstance().AppendDebug("Failed to build latency report.");
			}
		}

	private:

		void DumpPeriod() const throw() {
			if (!Log::GetInstance().IsDebugRegistrationOn()) {
				return;
			}
			try {
				DumpLatencyStat(
					false,
					m_receiving.common,
					m_sending.common,
					m_processing.common,
					m_full.common,
					m_blockBuffer.common,
					m_queueBuffer.common);
			} catch (...) {
				assert(false);
				Log::GetInstance().AppendDebug("Failed to build latency report.");
			}
		}

		void DumpLatencyStat(
					bool isCommon,
					const TimingsBundle::Stat &recv,
					const TimingsBundle::Stat &send,
					const TimingsBundle::Stat &proc,
					const TimingsBundle::Stat &full,
					const PercentsBundle::Stat &block,
					const PercentsBundle::Stat &queue)
				const {
			if (full.GetCount() == 0) {
				return;
			}
			std::ostringstream oss;
			oss << "Latency " << m_connectionId;
			if (!isCommon) {
				oss << " (actual)";
			}
			oss
				<< ": " << long(full.GetMean())
					<< '/' << full.GetMin() << '/' << full.GetMax()
				<< "; proc: " << long(proc.GetMean())
					<< '/' << proc.GetMin() << '/' << proc.GetMax();
			if (recv.GetCount() > 0) {
				oss << "; recv: " << long(recv.GetMean())
					<< '/' << recv.GetMin() << '/' << recv.GetMax();
			}
			oss
				<< "; send: " << long(send.GetMean())
					<< '/' << send.GetMin() << '/' << send.GetMax()
				<< "; block: " << long(block.GetMean())
					<< '/' << block.GetMin() << '/' << block.GetMax() << '%';
			if (queue.GetCount() > 0) {
				oss
					<< "; queue: " << long(queue.GetMean())
						<< '/' << queue.GetMin() << '/' << queue.GetMax() << '%';
			}
			Log::GetInstance().AppendDebug(oss.str().c_str());
		}

	private:

		const Instance::Id m_connectionId;
		
		mutable Mutex m_mutex;

		const boost::posix_time::time_duration m_period;
		boost::posix_time::ptime m_periodStart;

		TimingsBundle m_receiving;
		TimingsBundle m_sending;
		TimingsBundle m_processing;
		TimingsBundle m_full;
		PercentsBundle m_blockBuffer;
		PercentsBundle m_queueBuffer;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__MessageBlocksLatencyStat_2012$15252
