/**************************************************************************
 *   Created: 2011/02/20 11:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__IncomingTcpSslClientConnection_hpp__1102201135
#define INCLUDED_FILE__TUNNELEX__IncomingTcpSslClientConnection_hpp__1102201135

#include "TcpConnection.hpp"
#include "ConnectionsTraits.hpp"
#include "Core/Endpoint.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	class IncomingTcpSslClientConnection
			: public TcpConnection<typename ConnectionsTraits::Tcp<true, true>::Stream> {

	public:

		typedef ConnectionsTraits::Tcp<true, true> MyTrait;
		typedef SubProtoDirectionTraits::TcpIn<true, false> AcceptionTrait;
		typedef ConnectionsTraits::TcpUnsecureIncoming UnsecureTrait;
		typedef MyTrait::Stream DecodeStream;
		typedef UnsecureTrait::Stream RawStream;
		typedef AcceptionTrait::Acceptor Acceptor;
		typedef TcpConnection<Stream> Base;

	public:

		explicit IncomingTcpSslClientConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress,
					const Acceptor &acceptor);

		virtual ~IncomingTcpSslClientConnection() throw();

	public:

		virtual bool IsOneWay() const;
		virtual AutoPtr<EndpointAddress> GetRemoteAddress() const;

	protected:

		virtual void Setup();
		void ReadRemote(MessageBlock &);

	protected:

		virtual ACE_SOCK & GetIoStream() throw();
		virtual const ACE_SOCK & GetIoStream() const throw();

	private:

		void AcceptConnection(
					const Acceptor &acceptor,
					const RuleEndpoint &ruleEndpoint,
					const EndpointAddress &ruleEndpointAddress);

	private:

		TunnelEx::AutoPtr<const TunnelEx::Mods::Inet::TcpEndpointAddress>
			m_remoteAddress;
		RawStream m_rawStream;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__IncomingTcpSslClientConnection_hpp__1102201135
