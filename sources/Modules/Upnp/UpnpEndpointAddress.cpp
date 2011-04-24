/**************************************************************************
 *   Created: 2010/05/31 9:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "UpnpEndpointAddress.hpp"
#include "UpnpcService.hpp"
#include "TcpConnectionAcceptor.hpp"
#include "UdpConnectionAcceptor.hpp"
#include "Client.hpp"
#include "Modules/Inet/InetEndpointAddress.hpp"
#include "EndpointResourceIdentifierParsers.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods;
using namespace TunnelEx::Mods::Upnp;
using namespace TunnelEx::Helpers;

namespace TunnelEx { namespace Mods { namespace Upnp {

	WString CreateEndpointResourceIdentifier(
				const wchar_t *const proto,
				unsigned short externalPort,
				const SslCertificateId &certificate,
				const SslCertificateIdCollection &remoteCertificates) {
		assert(
			(std::wstring(proto) == Inet::TcpEndpointAddress::GetProto())
			|| (std::wstring(proto) == Inet::UdpEndpointAddress::GetProto()));
		assert(externalPort != 0);
		WFormat host(L"upnp_%1%://*:%2%");
		host % proto % externalPort;
		WString result = host.str().c_str();
		assert(!certificate.IsEmpty() || remoteCertificates.GetSize() == 0);
		if (!certificate.IsEmpty()) {
			result += L"?certificate=";
			result += certificate.EncodeUrlClone();
			const size_t remoteCertificatesSize = remoteCertificates.GetSize();
			for (size_t i = 0; i < remoteCertificatesSize; ++i) {
				const SslCertificateId &certificate = remoteCertificates[i];
				assert(!certificate.IsEmpty());
				if (certificate.IsEmpty()) {
					continue;
				}
				result += L"&remote_certificate=";
				result += certificate.EncodeUrlClone();
			}
		}
		return result;
	}

} } } 

//////////////////////////////////////////////////////////////////////////

class UpnpEndpointAddress::Implementation {

public:

	Implementation()
			: m_externalPort(0) {
		//...//
	}

public:

	unsigned short m_externalPort;
	SslCertificateId m_certificate;
	SslCertificateIdCollection m_remoteCertificates;
	WString m_resourceIdentifier;

};

UpnpEndpointAddress::UpnpEndpointAddress(const WString &resourceIdentifier) {
	const std::wstring path = resourceIdentifier.GetCStr();
	EndpointResourceIdentifierParsers::UrlSplitConstIterator pathIt
		= boost::make_split_iterator(path, boost::token_finder(boost::is_any_of(L"?&")));
	std::wstring host;
	std::auto_ptr<Implementation> pimpl(new Implementation);
	EndpointResourceIdentifierParsers::ParseEndpointHostPort(
		pathIt,
		host,
		pimpl->m_externalPort,
		true,
		false);
	if (host != L"*") {
		throw TunnelEx::InvalidLinkException(
			L"Could not parse resource identifier");
	}
	++pathIt;
	EndpointResourceIdentifierParsers::ParseEndpointCertificates(
		pathIt,
		pimpl->m_certificate,
		pimpl->m_remoteCertificates);
	m_pimpl = pimpl.release();
}

UpnpEndpointAddress::UpnpEndpointAddress(const UpnpEndpointAddress &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)){
	//...//
}

UpnpEndpointAddress::~UpnpEndpointAddress() throw() {
	delete m_pimpl;
}

UpnpEndpointAddress & UpnpEndpointAddress::operator =(const UpnpEndpointAddress &rhs) {
	std::auto_ptr<Implementation> newImpl(new Implementation(*rhs.m_pimpl));
	EndpointAddress::operator =(rhs);
	delete m_pimpl;
	m_pimpl = newImpl.release();
	return *this;
}

void UpnpEndpointAddress::Swap(UpnpEndpointAddress &rhs) throw() {
	std::swap(m_pimpl, rhs.m_pimpl);
}

const SslCertificateId & UpnpEndpointAddress::GetCertificate() const {
	return m_pimpl->m_certificate;
}

void UpnpEndpointAddress::SetCertificate(const SslCertificateId &certificate) {
	m_pimpl->m_certificate = certificate;
	m_pimpl->m_resourceIdentifier.Clear();
}

const SslCertificateIdCollection & UpnpEndpointAddress::GetRemoteCertificates() const {
	return m_pimpl->m_remoteCertificates;
}

void UpnpEndpointAddress::SetRemoteCertificates(
			const SslCertificateIdCollection &certificates) {
	m_pimpl->m_remoteCertificates = certificates;
	m_pimpl->m_resourceIdentifier.Clear();
}

const WString & UpnpEndpointAddress::GetResourceIdentifier() const {
	if (m_pimpl->m_resourceIdentifier.IsEmpty()) {
		CreateEndpointResourceIdentifier(
				GetSubProto(),
				GetExternalPort(),
				GetCertificate(),
				GetRemoteCertificates())
			.Swap(const_cast<UpnpEndpointAddress *>(this)->m_pimpl->m_resourceIdentifier);
	}
	return m_pimpl->m_resourceIdentifier;
}

UniquePtr<Connection> UpnpEndpointAddress::CreateRemoteConnection(
			const RuleEndpoint &,
			SharedPtr<const EndpointAddress>)
		const {
	throw TunnelEx::ConnectionOpeningException(
		L"Could not create network connection with UPnP"
			L" (only \"acceptable\" the type of endpoint)");
}

bool UpnpEndpointAddress::IsReadyToRecreateLocalConnection() const {
	return false;
}

bool UpnpEndpointAddress::IsReadyToRecreateRemoteConnection() const {
	return false;
}

unsigned short UpnpEndpointAddress::GetExternalPort() const {
	return m_pimpl->m_externalPort;
}

std::wstring UpnpEndpointAddress::GetHumanReadable(const std::wstring &externalIp) const {
	WFormat result(L"> %1%://%2%:%3%");
	result % GetSubProto() % externalIp % GetExternalPort();
	return GetCertificate().IsEmpty()
		?	result.str()
		:	result.str() + L" (secured)";
}

//////////////////////////////////////////////////////////////////////////

UpnpTcpEndpointAddress::UpnpTcpEndpointAddress(const WString &resourceIdentifier)
		: UpnpEndpointAddress(resourceIdentifier) {
	//...//
}

UpnpTcpEndpointAddress::UpnpTcpEndpointAddress(const UpnpTcpEndpointAddress &rhs)
		: UpnpEndpointAddress(rhs) {
	//...//
}

UpnpTcpEndpointAddress::~UpnpTcpEndpointAddress() throw() {
	//...//
}

UpnpTcpEndpointAddress & UpnpTcpEndpointAddress::operator =(const UpnpTcpEndpointAddress &rhs) {
	UpnpTcpEndpointAddress tmp(rhs);
	Swap(tmp);
	return *this;
}

void UpnpTcpEndpointAddress::Swap(UpnpTcpEndpointAddress &rhs) throw() {
	UpnpEndpointAddress::Swap(rhs);
}

UniquePtr<EndpointAddress> UpnpTcpEndpointAddress::Clone() const {
	return UniquePtr<EndpointAddress>(new UpnpTcpEndpointAddress(*this));
}

WString UpnpTcpEndpointAddress::CreateResourceIdentifier(
			unsigned short externalPort,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificate) {
	return Mods::Upnp::CreateEndpointResourceIdentifier(
		GetSubProtoImpl(),
		externalPort,
		certificate,
		remoteCertificate);
}

const wchar_t * UpnpTcpEndpointAddress::GetSubProto() const {
	return GetSubProtoImpl();
}

const wchar_t * UpnpTcpEndpointAddress::GetSubProtoImpl() {
	return Inet::TcpEndpointAddress::GetProto();
}

UniquePtr<Acceptor> UpnpTcpEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		const {

	Inet::TcpEndpointAddress tcpAddress(
		ConvertString<WString>(Client().GetLocalIpAddress().c_str()).GetCStr(),
		0);
	
	UniquePtr<TcpConnectionAcceptor> result(
		new TcpConnectionAcceptor(
			GetExternalPort(),
			tcpAddress,
			ruleEndpoint,
			ruleEndpointAddress));
	
	return result;

}

bool UpnpTcpEndpointAddress::IsHasMultiClientsType() const {
	return true;
}

UniquePtr<Connection> UpnpTcpEndpointAddress::CreateLocalConnection(
			const RuleEndpoint &,
			SharedPtr<const EndpointAddress>) 
		const {
	throw TunnelEx::ConnectionOpeningException(
		L"Could not create network connection with UPnP"
			L" (only \"acceptable\" the type of endpoint)");
}

//////////////////////////////////////////////////////////////////////////

UpnpUdpEndpointAddress::UpnpUdpEndpointAddress(const WString &resourceIdentifier)
		: UpnpEndpointAddress(resourceIdentifier) {
	//...//
}

UpnpUdpEndpointAddress::UpnpUdpEndpointAddress(const UpnpUdpEndpointAddress &rhs)
		: UpnpEndpointAddress(rhs) {
	//...//
}

UpnpUdpEndpointAddress::~UpnpUdpEndpointAddress() throw() {
	//...//
}

UpnpUdpEndpointAddress & UpnpUdpEndpointAddress::operator =(
			const UpnpUdpEndpointAddress &rhs) {
	UpnpUdpEndpointAddress tmp(rhs);
	Swap(tmp);
	return *this;
}

void UpnpUdpEndpointAddress::Swap(UpnpUdpEndpointAddress &rhs) throw() {
	UpnpEndpointAddress::Swap(rhs);
}

UniquePtr<EndpointAddress> UpnpUdpEndpointAddress::Clone() const {
	return UniquePtr<EndpointAddress>(new UpnpUdpEndpointAddress(*this));
}

WString UpnpUdpEndpointAddress::CreateResourceIdentifier(
			unsigned short externalPort,
			const SslCertificateId &certificate,
			const SslCertificateIdCollection &remoteCertificates) {
	return Mods::Upnp::CreateEndpointResourceIdentifier(
		GetSubProtoImpl(),
		externalPort,
		certificate,
		remoteCertificates);
}

const wchar_t * UpnpUdpEndpointAddress::GetSubProto() const {
	return GetSubProtoImpl();
}

const wchar_t * UpnpUdpEndpointAddress::GetSubProtoImpl() {
	return Inet::UdpEndpointAddress::GetProto();
}

UniquePtr<Acceptor> UpnpUdpEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress)
		const {
	
	Inet::UdpEndpointAddress udpAddress(
		ConvertString<WString>(Client().GetLocalIpAddress().c_str()).GetCStr(),
		0);
	
	UniquePtr<UdpConnectionAcceptor> result(
		new UdpConnectionAcceptor(
			GetExternalPort(),
			udpAddress,
			ruleEndpoint,
			ruleEndpointAddress));
	
	return result;
}

bool UpnpUdpEndpointAddress::IsHasMultiClientsType() const {
	return true;
}

UniquePtr<Connection> UpnpUdpEndpointAddress::CreateLocalConnection(
			const RuleEndpoint &,
			SharedPtr<const EndpointAddress>) 
		const {
	throw TunnelEx::ConnectionOpeningException(
		L"Could not create network connection with UPnP"
			L" (only \"acceptable\" the type of endpoint)");
}


//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Upnp {

	UniquePtr<EndpointAddress> CreateUpnpTcpEndpointAddress(
				Server::ConstRef,
				const WString &resourceIdentifier) {
		return UniquePtr<EndpointAddress>(
			new UpnpTcpEndpointAddress(resourceIdentifier));
	}

	UniquePtr<EndpointAddress> CreateUpnpUdpEndpointAddress(
				Server::ConstRef,
				const WString &resourceIdentifier) {
		return UniquePtr<EndpointAddress>(
			new UpnpUdpEndpointAddress(resourceIdentifier));
	}

} } }

