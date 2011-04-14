/**************************************************************************
 *   Created: 2010/06/06 23:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: TcpConnectionAcceptor.hpp 1133 2011-02-22 23:18:56Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Acceptor_hpp__1006062347
#define INCLUDED_FILE__TUNNELEX__Acceptor_hpp__1006062347

#include "UpnpcService.hpp"
#include "Modules/Inet/TcpConnectionAcceptor.hpp"

#include "Core/Server.hpp"


namespace TunnelEx { namespace Mods { namespace Upnp {

	class TcpConnectionAcceptor
		: public TunnelEx::Mods::Inet::TcpConnectionAcceptor<false, true> {

	public:

		typedef Inet::TcpConnectionAcceptor<false, true> Base;

	public:

		explicit TcpConnectionAcceptor(
				unsigned short externalPort,
				const Inet::InetEndpointAddress &,
				const RuleEndpoint &,
				SharedPtr<const EndpointAddress>);
		
		virtual ~TcpConnectionAcceptor() throw();

	private:

		std::auto_ptr<UpnpcService> m_upnpcService;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__Acceptor_hpp__1006062347
