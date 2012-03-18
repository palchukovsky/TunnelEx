/**************************************************************************
 *   Created: 2007/06/12 21:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#pragma once

#include "TcpConnection.hpp"
#include "InetEndpointAddress.hpp"
#include "ConnectionsTraits.hpp"
#include "Core/Endpoint.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Error.hpp"
#include "Core/Log.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	//////////////////////////////////////////////////////////////////////////

	class OutcomingTcpConnection
			: public TcpConnection<typename ConnectionsTraits::Tcp<false, false>::Stream> {

	public:

		typedef ConnectionsTraits::Tcp<false, false> Trait;
		typedef Trait::Stream Stream;
		typedef TcpConnection<Stream> Base;

	public:

		explicit OutcomingTcpConnection(
					const ACE_INET_Addr &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress);
		~OutcomingTcpConnection();

	protected:

		virtual void CloseIoHandle() throw();

	private:
	
		void OpenConnection(
					const ACE_INET_Addr &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint);

	};

	//////////////////////////////////////////////////////////////////////////

	template<bool isServer>
	class OutcomingSslTcpConnection
			: public TcpConnection<ConnectionsTraits::TcpSecureOutcoming::Stream> {

	public:

		typedef ConnectionsTraits::TcpSecureOutcoming MyTrait;
		typedef ConnectionsTraits::TcpUnsecureOutcoming UnsecureTrait;
		typedef UnsecureTrait::Stream IoStream;
		typedef MyTrait::Stream DataStream;
		typedef TcpConnection<DataStream> Base;

	public:

		explicit OutcomingSslTcpConnection(
					const ACE_INET_Addr &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Base(ruleEndpoint, ruleEndpointAddress) {
			OpenConnection(address, ruleEndpoint, *ruleEndpointAddress);
		}
		
		virtual ~OutcomingSslTcpConnection() {
			assert(m_ioStream.get_handle() == ACE_INVALID_HANDLE);
		}

	protected:

		virtual void Setup() {
			
			assert(!GetDataStream().IsDecryptorEncryptorMode());
			GetDataStream().SwitchToDecryptorEncryptorMode();

			try {
				SslConnect();
			} catch (const TunnelEx::LocalException &ex) {
				WFormat message(L"Failed to create secure (SSL/TLS) connection for %2%: %1%");
				message % ex.GetWhat() % GetInstanceId();
				CancelSetup(message.str().c_str());
				return;
			}

			if (!GetDataStream().GetEncrypted().empty()) {
				auto cleanFunc = [](Stream *stream) {
					stream->ClearEncrypted();
				};
				std::unique_ptr<Stream, decltype(cleanFunc)> cleaner(
					&GetDataStream(),
					cleanFunc);
				WriteDirectly(
					*CreateMessageBlock(
						GetDataStream().GetEncrypted().size(),
						&GetDataStream().GetEncrypted()[0]));
			}
			
			if (!GetDataStream().IsConnected()) {
				StartReadingRemote();
				return;
			}

			CompleteSslConnect();
			Base::Setup();

		}

		virtual void ReadRemote(MessageBlock &messageBlock) {
	
			assert(GetDataStream().IsDecryptorEncryptorMode());

			if (	GetDataStream().IsConnected()
					|| messageBlock.GetUnreadedDataSize() == 0) {
				Base::ReadRemote(messageBlock);
				return;
			}

			assert(!IsSetupCompleted());

			try {
				SslConnect(messageBlock);
			} catch (const TunnelEx::LocalException &ex) {
				StopReadingRemote();
				WFormat message(L"Failed to create SSL/TLS connection for %2%: %1%");
				message % ex.GetWhat() % GetInstanceId();
				CancelSetup(message.str().c_str());
				return;
			}
			if (!GetDataStream().GetEncrypted().empty()) {
				auto cleanFunc = [](Stream *stream) {
					stream->ClearEncrypted();
				};
				std::unique_ptr<Stream, decltype(cleanFunc)> cleaner(
					&GetDataStream(),
					cleanFunc);
				WriteDirectly(
					*CreateMessageBlock(
						GetDataStream().GetEncrypted().size(),
						&GetDataStream().GetEncrypted()[0]));
			}

			assert(
				GetDataStream().IsConnected()
				|| messageBlock.GetUnreadedDataSize() == 0);
			if (!GetDataStream().IsConnected()) {
				return;
			}

			CompleteSslConnect();
			StopReadingRemote();
			Base::Setup();
			Base::ReadRemote(messageBlock);

		}

	protected:

		virtual void CloseIoHandle() throw() {
			assert(m_ioStream.get_handle() != ACE_INVALID_HANDLE);
			CloseDataStream();
			m_ioStream.close();
		}

		virtual ACE_SOCK & GetIoStream() throw() {
			return m_ioStream;
		}
		virtual const ACE_SOCK & GetIoStream() const throw() {
			return const_cast<OutcomingSslTcpConnection *>(this)->GetIoStream();
		}

	private:

		void SslConnect() {
			static_assert(
				false,
				"Implements SSL connection process (connect or accept).");
		}
		void SslConnect(MessageBlock &) {
			static_assert(
				false,
				"Implements SSL connection process (connect or accept).");
		}

		void CompleteSslConnect() {
			static_assert(
				false,
				"Implements SSL connection process (connect or accept).");
		}

		const ACE_SSL_Context & GetSslContext(const TcpEndpointAddress &) const {
			static_assert(
				false,
				"Implements SSL connection process (connect or accept).");
		}

	private:

		void OpenConnection(
					const ACE_INET_Addr &address,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					const EndpointAddress &ruleEndpointAddress) {

			ACE_SOCK_Connector connector;
			const ACE_Time_Value timeout(ruleEndpoint.GetOpenTimeout());
			if (0 != connector.connect(m_ioStream, address, &timeout, ACE_Addr::sap_any, 1)) {
				const Error error(errno);
				WFormat message(L"%1% (%2%)");
				message % error.GetStringW() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
		
			std::auto_ptr<DataStream> codeStream(
				new DataStream(
					GetSslContext(
						*boost::polymorphic_downcast<const TcpEndpointAddress *>(&ruleEndpointAddress))));
			codeStream->set_handle(m_ioStream.get_handle());
			SetDataStream(codeStream);
	
		}
		
		void OpenSslConnection(const EndpointAddress &ruleEndpointAddress);

	private:

		IoStream m_ioStream;

	};

	template<>
	void OutcomingSslTcpConnection<false>::SslConnect() {
		GetDataStream().Connect();
	}
	template<>
	void OutcomingSslTcpConnection<false>::SslConnect(
				MessageBlock &messageBlock) {
		GetDataStream().Connect(messageBlock);
	}

	template<>
	void OutcomingSslTcpConnection<false>::CompleteSslConnect() {
		assert(
 			SSL_get_peer_certificate(GetDataStream().ssl()) != 0
 			|| boost::polymorphic_downcast<const TcpEndpointAddress *>(
 					GetRuleEndpointAddress().Get())
 				->GetRemoteCertificates().GetSize() == 0);
		Log::GetInstance().AppendDebug(
			"SSL/TLS connection for %1% created (connected).",
			GetInstanceId());
	}

	template<>
	const ACE_SSL_Context & OutcomingSslTcpConnection<false>::GetSslContext(
				const TcpEndpointAddress &address)
			const {
		return address.GetSslClientContext();
	}

	template<>
	void OutcomingSslTcpConnection<true>::SslConnect() {
		GetDataStream().Accept();
	}
	template<>
	void OutcomingSslTcpConnection<true>::SslConnect(MessageBlock &messageBlock) {
		GetDataStream().Accept(messageBlock);
	}

	template<>
	void OutcomingSslTcpConnection<true>::CompleteSslConnect() {
		assert(
			SSL_get_peer_certificate(GetDataStream().ssl()) != 0
			|| boost::polymorphic_downcast<const TcpEndpointAddress *>(
					GetRuleEndpointAddress().Get())
				->GetRemoteCertificates().GetSize() == 0);
		assert(
			SSL_get_peer_certificate(GetDataStream().ssl()) == 0
			|| SSL_get_verify_result(GetDataStream().ssl()) == X509_V_OK);
		Log::GetInstance().AppendDebug(
			"SSL/TLS connection for %1% created (accepted).",
			GetInstanceId());
	}

	template<>
	const ACE_SSL_Context & OutcomingSslTcpConnection<true>::GetSslContext(
				const TcpEndpointAddress &address)
			const {
		return address.GetSslServerContext();
	}

	//////////////////////////////////////////////////////////////////////////

} } }
