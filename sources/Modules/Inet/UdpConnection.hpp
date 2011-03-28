/**************************************************************************
 *   Created: 2008/10/24 18:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: UdpConnection.hpp 1133 2011-02-22 23:18:56Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UdpConnection_hpp__0810241857
#define INCLUDED_FILE__TUNNELEX__UdpConnection_hpp__0810241857

#include "InetConnection.hpp"
#include "InetEndpointAddress.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	template<typename StreamT>
	class UdpConnection : public InetConnection<UdpEndpointAddress> {

	public:

		typedef StreamT Stream;
		typedef InetConnection<TcpEndpointAddress> Base;

	protected:

		explicit UdpConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: InetConnection(ruleEndpoint, ruleEndpointAddress) {
			//...//
		}

		virtual ~UdpConnection() throw() {
			//...//
		}

	};
	
} } }


#endif // INCLUDED_FILE__TUNNELEX__UdpConnection_hpp__0810241857
