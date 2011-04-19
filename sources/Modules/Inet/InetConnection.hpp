/**************************************************************************
 *   Created: 2007/03/01 1:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__AceConnection_h__0703010143
#define INCLUDED_FILE__AceConnection_h__0703010143

#include "Core/Connection.hpp"
#include "Core/Error.hpp"

class ACE_SOCK;

namespace TunnelEx { namespace Mods { namespace Inet {

	//! Abstract network connection.
	template<typename EndpointAddressImplT>
	class InetConnection : public TunnelEx::Connection {

	public:

		typedef TunnelEx::Connection Base;

	protected:

		typedef EndpointAddressImplT EndpointAddressImpl;

	public:

		explicit InetConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Connection(ruleEndpoint, ruleEndpointAddress) {
			//...//
		}
		
		virtual ~InetConnection() throw() {
			//...//
		}
	
	public:
	
		virtual TunnelEx::UniquePtr<TunnelEx::EndpointAddress> GetLocalAddress() const {
			using namespace TunnelEx;
			ACE_INET_Addr addr;			
			if (GetIoStream().get_local_addr(addr) != 0) {
				const Error error(errno);
				WFormat message(L"Failed to get local inet address: \"%1% (%2%)\".");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw SystemException(message.str().c_str());
			}
			return UniquePtr<EndpointAddress>(new EndpointAddressImpl(addr));
		}

	protected:

		virtual TunnelEx::IoHandleInfo GetIoHandle() {
			return IoHandleInfo(GetIoStream().get_handle(), IoHandleInfo::TYPE_SOCKET);
		}

		virtual ACE_SOCK & GetIoStream() = 0;
		virtual const ACE_SOCK & GetIoStream() const = 0;


	};

} } }

#endif // ifndef INCLUDED_FILE__AceConnection_h__0703010143
