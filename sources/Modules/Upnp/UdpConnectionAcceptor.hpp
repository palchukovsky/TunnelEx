/**************************************************************************
 *   Created: 2010/09/04 3:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UdpConnectionAcceptor_hpp__1009040337
#define INCLUDED_FILE__TUNNELEX__UdpConnectionAcceptor_hpp__1009040337

#include "UpnpcService.hpp"
#include "Modules/Inet/UdpConnectionAcceptor.hpp"

namespace TunnelEx { namespace Mods { namespace Upnp {

	class UdpConnectionAcceptor
		: public TunnelEx::Mods::Inet::UdpConnectionAcceptor<false> {

	public:

		typedef Inet::UdpConnectionAcceptor<false> Base;

	public:

		explicit UdpConnectionAcceptor(
				unsigned short externalPort,
				const Inet::InetEndpointAddress &,
				const RuleEndpoint &,
				SharedPtr<const EndpointAddress>);
		
		virtual ~UdpConnectionAcceptor() throw();

	private:

		std::auto_ptr<UpnpcService> m_upnpcService;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__UdpConnectionAcceptor_hpp__1009040337
