/**************************************************************************
 *   Created: 2011/02/20 11:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: IncomingTcpSslClientConnection.cpp 1133 2011-02-22 23:18:56Z palchukovsky $
 **************************************************************************/

#include "Prec.h"
#include "IncomingTcpSslClientConnection.hpp"
#include <TunnelEx/Log.hpp>
#include <TunnelEx/Exceptions.hpp>
#include <TunnelEx/Error.hpp>

using namespace std;
using namespace boost;
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
	try {
		CloseDataStream();
		const int result = m_rawStream.close();
		BOOST_ASSERT(result == 0);
		ACE_UNUSED_ARG(result);
	} catch (...) {
		BOOST_ASSERT(false);
	}
}

bool IncomingTcpSslClientConnection::IsOneWay() const {
	return false;
}

ACE_SOCK & IncomingTcpSslClientConnection::GetIoStream() {
	return m_rawStream;
}

const ACE_SOCK & IncomingTcpSslClientConnection::GetIoStream() const {
	return const_cast<IncomingTcpSslClientConnection *>(this)->GetIoStream();
}

UniquePtr<EndpointAddress> IncomingTcpSslClientConnection::GetRemoteAddress() const {
	return UniquePtr<EndpointAddress>(new TcpEndpointAddress(*m_remoteAddress));
}

void IncomingTcpSslClientConnection::AcceptConnection(
			const Acceptor &acceptor,
			const RuleEndpoint &ruleEndpoint,
			const EndpointAddress &ruleEndpointAddress) {
	
	ACE_Time_Value timeout(ruleEndpoint.GetOpenTimeout());
	ACE_INET_Addr aceRemoteAddr;

	if (acceptor.accept(m_rawStream, &aceRemoteAddr, &timeout) != 0) {
		const Error error(errno);
		WFormat message(L"Failed to accept incoming connection: \"%1% (%2%)\"");
		message % error.GetString().GetCStr() % error.GetErrorNo();
		throw ConnectionOpeningException(message.str().c_str());
	}
	
	UniquePtr<const TcpEndpointAddress> remoteAddress(
		new TcpEndpointAddress(aceRemoteAddr));

	auto_ptr<DecodeStream> decodeStream(
		new DecodeStream(
			polymorphic_downcast<const TcpEndpointAddress *>(
					&ruleEndpointAddress)
				->GetSslClientContext()));
	SetDataStream(decodeStream);

	remoteAddress.Swap(m_remoteAddress);

}

void IncomingTcpSslClientConnection::Setup() {
	
	BOOST_ASSERT(!GetDataStream().IsDecryptorEncryptorMode());
	
	GetDataStream().SwitchToDecryptorEncryptorMode();
	
	try {
		GetDataStream().Connect();
	} catch (const TunnelEx::LocalException &ex) {
		WFormat message(L"Failed to create secure (SSL/TLS) connection for %2%: %1%");
		message % ex.GetWhat() % GetInstanceId();
		CancelSetup(message.str().c_str());
		return;
	}

	if (GetDataStream().GetEncryptorDecryptorAnswer().size() > 0) {
		try {
			SendToTunnel(
				&GetDataStream().GetEncryptorDecryptorAnswer()[0],
				GetDataStream().GetEncryptorDecryptorAnswer().size());
		} catch (...) {
			GetDataStream().ResetEncryptorDecryptorAnswer();
			throw;
		}
		GetDataStream().ResetEncryptorDecryptorAnswer();
		StartReadRemote();
	} else if (GetDataStream().IsConnected()) {
		BOOST_ASSERT(
			SSL_get_peer_certificate(GetDataStream().ssl()) != 0
			|| polymorphic_downcast<const TcpEndpointAddress *>(
					GetRuleEndpointAddress().Get())
				->GetRemoteCertificates().GetSize() == 0);
		BOOST_ASSERT(
			SSL_get_peer_certificate(GetDataStream().ssl()) == 0
			|| SSL_get_verify_result(GetDataStream().ssl()) == X509_V_OK);
		Base::Setup();
	} else {
		WFormat message(L"Failed to create secure (SSL/TLS) connection for %1%: unknown error");
		message % GetInstanceId();
		CancelSetup(message.str().c_str());
	}
	
}

void IncomingTcpSslClientConnection::ReadRemote(MessageBlock &messageBlock) {
	
	BOOST_ASSERT(GetDataStream().IsDecryptorEncryptorMode());

	if (	GetDataStream().IsConnected()
			|| messageBlock.GetUnreadedDataSize() == 0) {
		Base::ReadRemote(messageBlock);
		return;
	}

	BOOST_ASSERT(!IsSetupCompleted());

	try {
		GetDataStream().Connect(messageBlock);
	} catch (const TunnelEx::LocalException &ex) {
		StopReadRemote();
		WFormat message(L"Failed to create SSL/TLS connection for %2%: %1%");
		message % ex.GetWhat() % GetInstanceId();
		CancelSetup(message.str().c_str());
		return;
	}
	if (GetDataStream().GetEncryptorDecryptorAnswer().size() > 0) {
		try {
			SendToTunnel(
				&GetDataStream().GetEncryptorDecryptorAnswer()[0],
				GetDataStream().GetEncryptorDecryptorAnswer().size());
		} catch (...) {
			GetDataStream().ResetEncryptorDecryptorAnswer();
			throw;
		}
		GetDataStream().ResetEncryptorDecryptorAnswer();
	}

	BOOST_ASSERT(GetDataStream().IsConnected() || messageBlock.GetUnreadedDataSize() == 0);

	if (GetDataStream().IsConnected()) {
		Log::GetInstance().AppendDebug(
			"SSL/TLS connection for %1% created.",
			GetInstanceId());
		StopReadRemote();
		Base::Setup();
		Base::ReadRemote(messageBlock);
	}

}
