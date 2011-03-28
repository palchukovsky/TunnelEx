/**************************************************************************
 *   Created: 2010/12/03 20:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ConnectionsTraits.hpp 1133 2011-02-22 23:18:56Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ConnectionTrait_hpp__1012032012
#define INCLUDED_FILE__TUNNELEX__ConnectionTrait_hpp__1012032012

#include "SslSockStream.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	template<bool isSecured>
	class IncomingTcpConnection;

	class IncomingTcpSslClientConnection;

	class OutcomingTcpConnection;
	
	template<bool isServer>
	class OutcomingSslTcpConnection;

	//////////////////////////////////////////////////////////////////////////

	namespace AcceptionTraits {

		template<bool isSecured>
		struct Tcp {
			BOOST_STATIC_ASSERT(!isSecured);
			typedef ACE_SOCK_Acceptor Acceptor;
		};

		template<>
		struct Tcp<true> {
			typedef ACE_SSL_SOCK_Acceptor Acceptor;
		};

	}

	//////////////////////////////////////////////////////////////////////////

	namespace ConnectionsTraits {

		template<bool isSecured, bool isInput>
		struct Tcp {

			BOOST_STATIC_ASSERT(!isSecured);
			BOOST_STATIC_ASSERT(isInput);
			
			typedef ACE_SOCK_Stream Stream;
		
		};

		template<>
		struct Tcp<false, false> {
			typedef ACE_SOCK_Stream Stream;
		};

		template<>
		struct Tcp<true, true> {
			typedef TunnelEx::Mods::Inet::SslSockStream Stream;
		};

		template<>
		struct Tcp<true, false> {
			typedef TunnelEx::Mods::Inet::SslSockStream Stream;
		};

		typedef Tcp<false, true> TcpUnsecureIncoming;
		typedef Tcp<false, false> TcpUnsecureOutcoming;
		typedef Tcp<true, true> TcpSecureIncoming;
		typedef Tcp<true, false> TcpSecureOutcoming;
	
		//////////////////////////////////////////////////////////////////////////

		template<bool isSecured, bool isInput>
		struct Udp {
			
			BOOST_STATIC_ASSERT(!isSecured);
			BOOST_STATIC_ASSERT(isInput);

			typedef ACE_SOCK_Dgram Stream;

		};

		template<>
		struct Udp<false, false> {
			typedef ACE_SOCK_CODgram Stream;
		};

		template<bool isInput>
		struct Udp<true, isInput> {
			BOOST_STATIC_ASSERT(false); // DTLS does not implemented yet!
		};

		typedef Udp<false, true> UdpUnsecureIncoming;
		typedef Udp<false, false> UdpUnsecureOutcoming;

	}

	//////////////////////////////////////////////////////////////////////////

	// Sub protocols direction traits.
	namespace SubProtoDirectionTraits {

		template<bool isSecured, bool isServer>
		struct TcpIn {
			BOOST_STATIC_ASSERT(isServer);
			typedef AcceptionTraits::Tcp<isSecured> AcceptionTrait;
			typedef typename AcceptionTrait::Acceptor Acceptor;
			typedef IncomingTcpConnection<isSecured> Connection;
		};

		template<bool isSecured>
		struct TcpIn<isSecured, false> {
			// Scenario not implemented yet
			BOOST_STATIC_ASSERT(false);
		};

		template<>
		struct TcpIn<true, false> {
			// always unsecured acceptor - tcp here is only transport
			typedef AcceptionTraits::Tcp<false> AcceptionTrait;
			typedef AcceptionTrait::Acceptor Acceptor;
			typedef IncomingTcpSslClientConnection Connection;
		};

		template<bool isSecured, bool isServer>
		struct TcpOut {
			BOOST_STATIC_ASSERT(isSecured == false);
			BOOST_STATIC_ASSERT(isServer == false);
			typedef OutcomingTcpConnection Connection;
		};

		template<>
		struct TcpOut<false, true> {
			// Scenario (not secured sub-protocol server) not implemented yet
		};

		template<bool isServer>
		struct TcpOut<true, isServer> {
			typedef OutcomingSslTcpConnection<isServer> Connection;
		};

	}

	//////////////////////////////////////////////////////////////////////////

} } }

#endif // INCLUDED_FILE__TUNNELEX__ConnectionTrait_hpp__1012032012
