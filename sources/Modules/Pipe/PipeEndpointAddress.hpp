/**************************************************************************
 *   Created: 2008/11/26 14:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeEndpointAddress_hpp__0811261434
#define INCLUDED_FILE__TUNNELEX__PipeEndpointAddress_hpp__0811261434

#include "Core/EndpointAddress.hpp"
#include "Core/String.hpp"

namespace TunnelEx { namespace Mods { namespace Pipe {

	//! Typed endpoint address for pipe path.
	/** @sa	Endpoint
	  * @sa	EndpointAddress
	  */
	class PipeEndpointAddress : public TunnelEx::EndpointAddress {

	public:

		PipeEndpointAddress() {
			//...//
		}
		
		explicit PipeEndpointAddress(const WString &path);

		PipeEndpointAddress(const PipeEndpointAddress &rhs)
				: m_impl(rhs.m_impl) {
			//! @todo: workaround for bug with hash in get_path_name, fix it after ACE update [2008/12/15 5:56]
			wcscpy(const_cast<ACE_TCHAR *>(m_impl.get_path_name()), rhs.m_impl.get_path_name());
		}
		
		virtual ~PipeEndpointAddress() throw() {
			//...//
		}

		const PipeEndpointAddress & operator =(const PipeEndpointAddress &rhs) {
			EndpointAddress::operator =(rhs);
			m_impl = ACE_SPIPE_Addr(rhs.m_impl);
			return *this;
		}

		void Swap(PipeEndpointAddress &rhs) throw() {
			EndpointAddress::Swap(rhs);
			ACE_SPIPE_Addr oldImpl = m_impl;
			m_impl = rhs.m_impl;
			rhs.m_impl = oldImpl;
		}

	public:

		virtual const TunnelEx::WString & GetResourceIdentifier() const;
		
		virtual bool IsHasMultiClientsType(void) const;
		
		virtual AutoPtr<Acceptor> OpenForIncomingConnections(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				const;

		virtual AutoPtr<Connection> CreateRemoteConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress) 
				const;

		virtual bool IsReadyToRecreateRemoteConnection() const {
			return false;
		}
		virtual bool IsReadyToRecreateLocalConnection() const {
			return false;
		}

		virtual AutoPtr<Connection> CreateLocalConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress) 
				const;

		virtual AutoPtr<EndpointAddress> Clone() const;
		
	public:
	
		const ACE_SPIPE_Addr & GetAcePipeAddr() const {
			return const_cast<PipeEndpointAddress *>(this)->GetAcePipeAddr();
		}

		ACE_SPIPE_Addr & GetAcePipeAddr() {
			return m_impl;
		}

	private:

		ACE_SPIPE_Addr m_impl;
		mutable WString m_identifier;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__PipeEndpointAddress_hpp__0811261434