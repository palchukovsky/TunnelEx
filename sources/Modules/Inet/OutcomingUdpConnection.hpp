/**************************************************************************
 *   Created: 2008/10/26 20:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: OutcomingUdpConnection.hpp 1133 2011-02-22 23:18:56Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__OutcomingUdpConnection_hpp__0810262023
#define INCLUDED_FILE__TUNNELEX__OutcomingUdpConnection_hpp__0810262023

#include "UdpConnection.hpp"
#include "ConnectionsTraits.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	template<bool isSecured>
	class OutcomingUdpConnection
			: public UdpConnection<typename ConnectionsTraits::Udp<isSecured, false>::Stream> {

	public:

		typedef ConnectionsTraits::Udp<isSecured, false> Trait;
		typedef typename Trait::Stream Stream;
		typedef UdpConnection<Stream> Base;

	public:

		explicit OutcomingUdpConnection(
					const UdpEndpointAddress &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: UdpConnection(ruleEndpoint, ruleEndpointAddress) {
			if (m_stream.open(address.GetAceInetAddr()) != 0) {
				const Error error(errno);
				WFormat message(L"Failed to open UDP endpoint: %1% (%2%)");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
		}

		virtual ~OutcomingUdpConnection() throw() {
			try {
				const int result = m_stream.close();
				BOOST_ASSERT(result == 0);
				ACE_UNUSED_ARG(result);
			} catch (...) {
				BOOST_ASSERT(false);
			}
		}

	public:

		virtual UniquePtr<EndpointAddress> GetRemoteAddress() const {
			using namespace TunnelEx;
			ACE_INET_Addr addr;
			if (m_stream.get_remote_addr(addr) == 0) {
				return UniquePtr<EndpointAddress>(
					new UdpEndpointAddress(
					*boost::polymorphic_downcast<const UdpEndpointAddress *>(
					GetRuleEndpointAddress().Get())));
			} else {
				const Error error(errno);
				WFormat message(
					L"Failed to get remote inet (UDP) address: %1% (%2%)");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw SystemException(message.str().c_str());
			}
		}

	protected:

		virtual ACE_SOCK & GetIoStream() {
			return m_stream;
		}

		virtual const ACE_SOCK & GetIoStream() const {
			return const_cast<OutcomingUdpConnection *>(this)->GetIoStream();
		}

	private:

		Stream m_stream;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__OutcomingUdpConnection_hpp__0810262023
