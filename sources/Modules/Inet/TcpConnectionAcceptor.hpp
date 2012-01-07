/**************************************************************************
 *   Created: 2008/06/19 23:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__IncomingTcpConnectionsEventHandler_hpp__0806192319
#define INCLUDED_FILE__TUNNELEX__IncomingTcpConnectionsEventHandler_hpp__0806192319

#include "Api.h"
#include "IncomingTcpConnection.hpp"
#include "ConnectionsTraits.hpp"
#include "TcpConnectionAcceptor.hpp"
#include "InetEndpointAddress.hpp"
#include "IncomingTcpSslClientConnection.hpp"
#include "Core/Acceptor.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Log.hpp"
#include "Core/Error.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	class InetEndpointAddress;

	template<bool isSecured, bool isServer>
	class TUNNELEX_MOD_INET_API TcpConnectionAcceptor : public TunnelEx::Acceptor {

	public:

		typedef TunnelEx::Acceptor Base;

	private:

		typedef typename SubProtoDirectionTraits::TcpIn<isSecured, isServer>
			AcceptionTrait;
		typedef typename AcceptionTrait::Connection Connection;
		typedef typename AcceptionTrait::Acceptor Acceptor;

	public:

		explicit TcpConnectionAcceptor(
					const InetEndpointAddress &address,
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Base(ruleEndpoint, ruleEndpointAddress) {
			const ACE_INET_Addr &inetAddr = address.GetAceInetAddr();
			if (inetAddr.is_any() && inetAddr.get_port_number() == 0) {
				// see TEX-308
				throw ConnectionOpeningException(
					L"Failed to open socket for listening:"
						L" The requested address is not valid in its context");
			}
			ACE::set_handle_limit();
			if (m_acceptor.open(inetAddr, true) != 0) {
				const Error error(errno);
				WFormat exception(L"Failed to open socket for listening: %1% (%2%)");
				exception % error.GetStringW() % error.GetErrorNo();
				throw ConnectionOpeningException(exception.str().c_str());
			}
		}

		
		virtual ~TcpConnectionAcceptor() throw() {
			try {
				const int result = m_acceptor.close();
				ACE_UNUSED_ARG(result);
				assert(result == 0);
			} catch (...) {
				assert(false);
			}
		}

	public:

		virtual IoHandleInfo GetIoHandle() {
			return IoHandleInfo(m_acceptor.get_handle(), IoHandleInfo::TYPE_SOCKET);
		}

		virtual AutoPtr<TunnelEx::Connection> Accept() {
			AutoPtr<TunnelEx::Connection> result(
				new Connection(
					GetRuleEndpoint(),
					GetRuleEndpointAddress(),
					m_acceptor));
			return result;
		}
		
		virtual bool TryToAttach() {
			return false;
		}
		
		virtual AutoPtr<EndpointAddress> GetLocalAddress() const {
			ACE_INET_Addr addr;
			if (m_acceptor.get_local_addr(addr) != 0) {
				const Error error(errno);
				WFormat exception(L"Failed to get listening socket address: %1% (%2%)");
				exception % error.GetStringW() % error.GetErrorNo();
				throw SystemException(exception.str().c_str());
			}
			return AutoPtr<EndpointAddress>(new TcpEndpointAddress(addr));
		}
		
	private:

		Acceptor m_acceptor;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__IncomingTcpConnectionsEventHandler_hpp__0806192319
