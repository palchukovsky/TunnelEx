/**************************************************************************
 *   Created: 2010/05/25 3:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: UpnpcService.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Service_hpp__1005250329
#define INCLUDED_FILE__TUNNELEX__Service_hpp__1005250329

#include "Api.h"

#include "Client.hpp"

#include <TunnelEx/Rule.hpp>
#include <TunnelEx/Service.hpp>
#include <TunnelEx/Exceptions.hpp>
#include <TunnelEx/String.hpp>

namespace TunnelEx { namespace Mods { namespace Upnp {

	class TUNNELEX_MOD_UPNP_API UpnpcService : public TunnelEx::Service {

	public:

		explicit UpnpcService(
				SharedPtr<const ServiceRule>,
				const ServiceRule::Service &)
			throw(TunnelEx::EndpointException);
		virtual ~UpnpcService() throw();

	public:

		virtual void Start() throw(TunnelEx::EndpointException);
		virtual void Stop() throw();
		virtual void DoWork() throw(TunnelEx::EndpointException);

	public:
		
		static void ParseParam(
				const TunnelEx::WString &param,
				Client::Proto &proto,
				unsigned short &externalPort,
				std::wstring &destinationHost,
				unsigned short &destinationPort,
				bool &isForceMode,
				bool &isPersistent)
			throw(TunnelEx::InvalidLinkException);

		static TunnelEx::WString CreateParam(
				Client::Proto proto,
				unsigned short externalPort,
				const std::wstring &destinationHost,
				unsigned short destinationPort,
				bool isForceMode,
				bool isPersistent);

		static std::wstring GetHumanReadableExternalPort(
					const TunnelEx::WString &param,
					const std::wstring &externalIp)
				throw(TunnelEx::InvalidLinkException);
		static std::wstring GetHumanReadableDestination(
					const TunnelEx::WString &param)
				throw(TunnelEx::InvalidLinkException);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__Service_hpp__1005250329
