/**************************************************************************
 *   Created: 2010/06/07 2:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: UdpConnection.hpp 1090 2010-12-12 07:52:10Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UdpConnection_hpp__1006070207
#define INCLUDED_FILE__TUNNELEX__UdpConnection_hpp__1006070207

#include "UpnpcService.hpp"

#include "Modules/Inet/OutcomingUdpConnection.hpp"

namespace TunnelEx { namespace Mods { namespace Upnp {

	class UdpConnection : public Inet::OutcomingUdpConnection<false> {

	public:

		typedef Inet::OutcomingUdpConnection<false> Base;

	public:

		/** @throw TunnelEx::ConnectionOpeningException
		  */
		explicit UdpConnection(
					unsigned short externalPort,
					const Inet::UdpEndpointAddress &address,
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress);

		virtual ~UdpConnection() throw();

	private:

		std::auto_ptr<UpnpcService> m_upnpcService;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__UdpConnection_hpp__1006070207
