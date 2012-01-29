/**************************************************************************
 *   Created: 2008/10/26 20:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__OutcomingUdpConnection_hpp__0810262023
#define INCLUDED_FILE__TUNNELEX__OutcomingUdpConnection_hpp__0810262023

#include "UdpConnection.hpp"
#include "ConnectionsTraits.hpp"
#include "Core/Log.hpp"

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
				message % error.GetStringW() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
		}

		virtual ~OutcomingUdpConnection() throw() {
			try {
				const int result = m_stream.close();
				assert(result == 0);
				ACE_UNUSED_ARG(result);
			} catch (...) {
				Format message(
					"Unknown system error occurred: %1%:%2%."
						" Please restart the service"
						" and contact product support to resolve this issue."
						" %3% %4%");
				message
					% __FILE__ % __LINE__
					% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
				Log::GetInstance().AppendFatalError(message.str());
				assert(false);
			}
		}

	public:

		virtual AutoPtr<EndpointAddress> GetRemoteAddress() const {
			using namespace TunnelEx;
			ACE_INET_Addr addr;
			if (m_stream.get_remote_addr(addr) == 0) {
				return AutoPtr<EndpointAddress>(
					new UdpEndpointAddress(
						*boost::polymorphic_downcast<const UdpEndpointAddress *>(
						GetRuleEndpointAddress().Get())));
			} else {
				const Error error(errno);
				WFormat message(
					L"Failed to get remote inet (UDP) address: %1% (%2%)");
				message % error.GetStringW() % error.GetErrorNo();
				throw SystemException(message.str().c_str());
			}
		}

	protected:

		virtual void CloseIoHandle() throw() {
			m_stream.close();
		}

		virtual ACE_SOCK & GetIoStream() throw() {
			return m_stream;
		}

		virtual const ACE_SOCK & GetIoStream() const throw() {
			return const_cast<OutcomingUdpConnection *>(this)->GetIoStream();
		}

	private:

		Stream m_stream;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__OutcomingUdpConnection_hpp__0810262023
