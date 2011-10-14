/**************************************************************************
 *   Created: 2009/09/17 16:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__SerialPortEndpointAddress_hpp__0909171615
#define INCLUDED_FILE__TUNNELEX__SerialPortEndpointAddress_hpp__0909171615

#include "Api.h"

#include "Core/EndpointAddress.hpp"
#include "Core/String.hpp"

class ACE_DEV_Addr;

namespace TunnelEx { namespace Mods { namespace Serial {

	class TUNNELEX_MOD_SERIAL_API SerialEndpointAddress
			: public TunnelEx::EndpointAddress {

	public:

		enum Parity {
			P_NONE,
			P_ODD,
			P_EVEN,
			P_MARK,
			P_SPACE
		};

		enum FlowControl {
			FC_NONE,
			FC_XON_XOFF,
			FC_RTS_CTS,
			FC_DSR_DTR
		};

	public:

		SerialEndpointAddress();
		
		explicit SerialEndpointAddress(const WString &path);

		SerialEndpointAddress(const SerialEndpointAddress &rhs);
		
		virtual ~SerialEndpointAddress() throw();

		const SerialEndpointAddress & operator =(
				const SerialEndpointAddress &rhs);

		void Swap(SerialEndpointAddress &rhs) throw();

	public:

		virtual const TunnelEx::WString & GetResourceIdentifier() const;
		
		virtual bool IsHasMultiClientsType(void) const;
		
		virtual AutoPtr<Acceptor> OpenForIncomingConnections(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				const;

		virtual AutoPtr<Connection> CreateRemoteConnection(
					const TunnelEx::RuleEndpoint &,
					SharedPtr<const EndpointAddress>) 
				const;

		virtual AutoPtr<Connection> CreateLocalConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress) 
				const;

		virtual bool IsReadyToRecreateRemoteConnection() const {
			return false;
		}
		virtual bool IsReadyToRecreateLocalConnection() const {
			return false;
		}

		virtual AutoPtr<EndpointAddress> Clone() const;

	public:

		const wchar_t * GetLine() const;
		int GetBaudRate() const;
		unsigned char GetDataBits() const;
		unsigned char GetStopBits() const;
		Parity GetParity() const;
		FlowControl GetFlowControl() const;

		std::wstring GetHumanReadable() const;

		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &line,
				int baudRate,
				unsigned char dataBits,
				unsigned char stopBits,
				Parity parity,
				FlowControl flowControl);
		
	public:
	
		const ACE_DEV_Addr & GetAceDevAddr() const;

		ACE_DEV_Addr & GetAceDevAddr();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__SerialPortEndpointAddress_hpp__0909171615
