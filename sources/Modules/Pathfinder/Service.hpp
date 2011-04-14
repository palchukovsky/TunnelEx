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

#include "Core/String.hpp"

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
		/** @throw TunnelEx::Mods::Pathfinder::ServiceException
		  * @throw TunnelEx::Mods::Pathfinder::LicensingException
		  */
		bool GetProxy(
					const Mods::Inet::TcpEndpointAddress &target,
					Mods::Inet::ProxyList &result)
				const {
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

		/** @throw TunnelEx::Mods::Pathfinder::ServiceException,
		  * @throw TunnelEx::Mods::Pathfinder::LicensingException
		  */
		bool RequestProxy(
					const Mods::Inet::TcpEndpointAddress &,
					Mods::Inet::ProxyList &)
				const;

		void Report(
					const wchar_t *const,
					const Mods::Inet::TcpEndpointAddress &,
					const Mods::Inet::Proxy &,
					bool)
				const
				throw();

		/** @throw TunnelEx::Mods::Pathfinder::ServiceException
		  */
		void InitConnection(const Mods::Inet::TcpEndpointAddress &);

	private:

		Licensing *m_licensing;
		Handles *m_handles;
		mutable boost::optional<Mods::Inet::Proxy> m_goodProxy;

	};

	typedef ACE_Singleton<ServiceImpl, ACE_Thread_Mutex > Service;

} } }

#endif // INCLUDED_FILE__TUNNELEX__Service_hpp__1003220022
