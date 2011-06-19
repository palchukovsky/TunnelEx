/**************************************************************************
 *   Created: 2008/03/22 22:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__DtorTester_hpp__0803222220
#define INCLUDED_FILE__TUNNELEX__DtorTester_hpp__0803222220

namespace TestUtil {

	class DtorTester : private boost::noncopyable {
	public:
		DtorTester(
					unsigned int &deletedObjNumbHolder,
					std::string &lastDeletedNameHolder,
					const char *name = "")
				: m_deletedObjNumbHolder(deletedObjNumbHolder),
				m_lastDeletedNameHolder(lastDeletedNameHolder),
				m_name(name) {
			//...//
		}
		~DtorTester() {
			++m_deletedObjNumbHolder;
			m_lastDeletedNameHolder = m_name;
		}
	private:
		unsigned int &m_deletedObjNumbHolder;
		std::string &m_lastDeletedNameHolder;
		const std::string m_name;
	};

}

#endif // INCLUDED_FILE__TUNNELEX__DtorTester_hpp__0803222220
