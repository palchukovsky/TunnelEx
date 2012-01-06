/**************************************************************************
 *   Created: 2012/1/5/ 1:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LatencyStat_201215124
#define INCLUDED_FILE__TUNNELEX__LatencyStat_201215124


namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	class LatencyStat {

	public:

		typedef T ValueType;

	protected:

		typedef boost::accumulators::accumulator_set<
				ValueType,
				boost::accumulators::features<
					boost::accumulators::tag::mean,
					boost::accumulators::tag::min,
					boost::accumulators::tag::max>>
			Accumulator;

	public:

		LatencyStat() {
			//...//
		}

		LatencyStat(const LatencyStat &rhs)
				: m_accumulator(rhs.m_accumulator) {
			//...//
		}

		LatencyStat & operator =(const LatencyStat &rhs) {
			m_accumulator = rhs.m_accumulator;
			return *this;
		}

		LatencyStat & operator +=(const ValueType &val) {
			m_accumulator(val);
			return *this;
		}

	public:

		double GetMean() const {
			return boost::accumulators::mean(m_accumulator);
		}

		ValueType GetMin() const {
			return boost::accumulators::min(m_accumulator);
		}

		ValueType GetMax() const {
			return boost::accumulators::max(m_accumulator);
		}

		size_t GetCount() const {
			return boost::accumulators::count(m_accumulator);
		}

	public:

		void Reset() {
			m_accumulator = Accumulator();
		}

	private:
	
		Accumulator m_accumulator;

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__LatencyStat_2012$15124
