/**************************************************************************
 *   Created: 2011/02/20 11:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "IncomingTcpSslClientConnection.hpp"
#include "Core/Log.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Error.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Inet;

IncomingTcpSslClientConnection::IncomingTcpSslClientConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress,
			const Acceptor &acceptor)
		: Base(ruleEndpoint, ruleEndpointAddress) {
	AcceptConnection(acceptor, ruleEndpoint, *ruleEndpointAddress);
}

IncomingTcpSslClientConnection::~IncomingTcpSslClientConnection() throw() {
	assert(m_rawStream.get_handle() == ACE_INVALID_HANDLE);
}

void IncomingTcpSslClientConnection::CloseIoHandle() throw() {
	assert(m_rawStream.get_handle() != ACE_INVALID_HANDLE);
	CloseDataStream();
	m_rawStream.close();
}

bool IncomingTcpSslClientConnection::IsOneWay() const {
	return false;
}

ACE_SOCK & IncomingTcpSslClientConnection::GetIoStream() throw() {
	return m_rawStream;
}

const ACE_SOCK & IncomingTcpSslClientConnection::GetIoStream() const throw() {
	return const_cast<IncomingTcpSslClientConnection *>(this)->GetIoStream();
}

AutoPtr<EndpointAddress> IncomingTcpSslClientConnection::GetRemoteAddress() const {
	return AutoPtr<EndpointAddress>(new TcpEndpointAddress(*m_remoteAddress));
}

void IncomingTcpSslClientConnection::AcceptConnection(
			const Acceptor &acceptor,
			const RuleEndpoint &ruleEndpoint,
			const EndpointAddress &ruleEndpointAddress) {
	ACE_Time_Value timeout(ruleEndpoint.GetOpenTimeout());
	ACE_INET_Addr aceRemoteAddr;
	if (acceptor.accept(m_rawStream, &aceRemoteAddr, &timeout) != 0) {
		HandleAcceptError(m_rawStream);
	} else {
		HandleAcceptSuccess(m_rawStream);
	}
	AutoPtr<const TcpEndpointAddress> remoteAddress(
		new TcpEndpointAddress(aceRemoteAddr));
	std::auto_ptr<DecodeStream> decodeStream = CreateStream(ruleEndpointAddress);
	SetDataStream(decodeStream);
	remoteAddress.Swap(m_remoteAddress);
}

std::auto_ptr<IncomingTcpSslClientConnection::DecodeStream>
IncomingTcpSslClientConnection::CreateStream(
			const EndpointAddress &address)
		const {
	std::auto_ptr<DecodeStream> result(
		new DecodeStream(
			boost::polymorphic_downcast<const TcpEndpointAddress *>(
					&address)
				->GetSslClientContext()));
	result->set_handle(m_rawStream.get_handle());
	return result;
}

bool IncomingTcpSslClientConnection::SetupSslConnection(
			bool isReadingStarted,
			MessageBlock *messageBlock /*= nullptr*/) {
	
	DecodeStream &stream
		= *boost::polymorphic_downcast<DecodeStream *>(&GetDataStream());
	
	try {
		!messageBlock
			?	stream.Connect()
			:	stream.Connect(*messageBlock);
	} catch (const TunnelEx::LocalException &ex) {
		if (isReadingStarted) {
			StopReadingRemote();
		}
		WFormat message(
			L"Failed to create secure (SSL/TLS) incoming connection for %2%: %1%");
		message % ex.GetWhat() % GetInstanceId();
		CancelSetup(message.str().c_str());
		return false;
	}
//	assert(!isReadingStarted || !GetDataStream().GetEncrypted().empty());

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
		if (!isReadingStarted) {
			StartReadingRemote();
		}
		return true;
	}

	HandleAcceptSuccess(stream);
	if (isReadingStarted) {
		StopReadingRemote();
	}
	Base::Setup();
	return true;

}

void IncomingTcpSslClientConnection::Setup() {
	assert(!GetDataStream().IsDecryptorEncryptorMode());
	GetDataStream().SwitchToDecryptorEncryptorMode();
	SetupSslConnection(false);
}

void IncomingTcpSslClientConnection::ReadRemote(MessageBlock &messageBlock) {
	assert(GetDataStream().IsDecryptorEncryptorMode());
	if (	GetDataStream().IsConnected()
			|| messageBlock.GetUnreadedDataSize() == 0) {
		Base::ReadRemote(messageBlock);
		return;
	}
	assert(!IsSetupCompleted());
	if (!SetupSslConnection(true, &messageBlock)) {
		return;
	}
	assert(
		GetDataStream().IsConnected()
		|| messageBlock.GetUnreadedDataSize() == 0);
	if (GetDataStream().IsConnected()) {
		Base::ReadRemote(messageBlock);
	}
}

void IncomingTcpSslClientConnection::HandleAcceptError(RawStream &) const {
	const Error error(errno);
	WFormat message(L"Failed to accept incoming connection: \"%1% (%2%)\"");
	message % error.GetStringW() % error.GetErrorNo();
	throw ConnectionOpeningException(message.str().c_str());
}

void IncomingTcpSslClientConnection::HandleAcceptSuccess(RawStream &) const {
	//...//
}

void IncomingTcpSslClientConnection::HandleAcceptSuccess(
			DecodeStream &stream)
		const {
	assert(
		SSL_get_peer_certificate(stream.ssl()) != 0
		|| boost::polymorphic_downcast<const TcpEndpointAddress *>(
				GetRuleEndpointAddress().Get())
			->GetRemoteCertificates().GetSize() == 0);
	assert(
		SSL_get_peer_certificate(stream.ssl()) == 0
		|| SSL_get_verify_result(stream.ssl()) == X509_V_OK);
	Log::GetInstance().AppendDebug(
		"SSL/TLS connection for %1% created (connected).",
		GetInstanceId());

}
