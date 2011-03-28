/**************************************************************************
 *   Created: 2010/03/22 0:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Service.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Service_hpp__1003220022
#define INCLUDED_FILE__TUNNELEX__Service_hpp__1003220022

#include "Modules/Inet/InetEndpointAddress.hpp"
#include "Exceptions.hpp"

#include <TunnelEx/String.hpp>

namespace TunnelEx { namespace Mods { namespace Pathfinder {

	class ServiceImpl : private boost::noncopyable {

	private:

		struct Handles;
		struct Licensing;

	public:

		ServiceImpl();
		~ServiceImpl() throw();

	public:

		//! Returns proxy list for endpoint. Result is true, if list from online service.
		bool GetProxy(
					const Mods::Inet::TcpEndpointAddress &target,
					Mods::Inet::ProxyList &result)
				const
				throw(
					TunnelEx::Mods::Pathfinder::ServiceException,
					TunnelEx::Mods::Pathfinder::LicensingException) {
			return RequestProxy(target, result);
		}

		void ReportSuccess(
					const Mods::Inet::TcpEndpointAddress &target,
					const Mods::Inet::Proxy &proxy)
				const
				throw() {
			Report(L"success", target, proxy, true);
		}

		void ReportConnectError(
					const Mods::Inet::TcpEndpointAddress &target,
					const  Mods::Inet::Proxy &proxy)
				const
				throw() {
			Report(L"error/connect", target, proxy, false);
		}

		void ReportWorkingError(
					const Mods::Inet::TcpEndpointAddress &target,
					const Mods::Inet::Proxy &proxy)
				const
				throw() {
			Report(L"error/work", target, proxy, false);
		}

	private:

		bool RequestProxy(
					const Mods::Inet::TcpEndpointAddress &,
					Mods::Inet::ProxyList &)
				const
				throw(
					TunnelEx::Mods::Pathfinder::ServiceException,
					TunnelEx::Mods::Pathfinder::LicensingException);

		void Report(
					const wchar_t *const,
					const Mods::Inet::TcpEndpointAddress &,
					const Mods::Inet::Proxy &,
					bool)
				const
				throw();

		void InitConnection(
					const Mods::Inet::TcpEndpointAddress &)
				throw(TunnelEx::Mods::Pathfinder::ServiceException);

	private:

		Licensing *m_licensing;
		Handles *m_handles;
		mutable boost::optional<Mods::Inet::Proxy> m_goodProxy;

	};

	typedef ACE_Singleton<ServiceImpl, ACE_Thread_Mutex > Service;

} } }

#endif // INCLUDED_FILE__TUNNELEX__Service_hpp__1003220022
