/**************************************************************************
 *   Created: 2008/11/27 1:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "SerialEndpointAddress.hpp"
#include "SerialConnection.hpp"
#include "EndpointResourceIdentifierParsers.hpp"
#include "Core/Server.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Mods::Serial;

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Serial {

	void ParseEndpointParity(
				EndpointResourceIdentifierParsers::UrlSplitConstIterator source,
				SerialEndpointAddress::Parity &result) {
		std::wstring tmp;
		EndpointResourceIdentifierParsers::ParseUrlParamValue(source, tmp);
		boost::to_lower(tmp);
		typedef std::map<std::wstring, SerialEndpointAddress::Parity> Map;
		Map valsMap;
		valsMap.insert(std::make_pair(L"none", SerialEndpointAddress::P_NONE));
		valsMap.insert(std::make_pair(L"odd", SerialEndpointAddress::P_ODD));
		valsMap.insert(std::make_pair(L"even", SerialEndpointAddress::P_EVEN));
		valsMap.insert(std::make_pair(L"mark", SerialEndpointAddress::P_MARK));
		valsMap.insert(std::make_pair(L"space", SerialEndpointAddress::P_SPACE));
		const Map::const_iterator pos = valsMap.find(tmp);
		if (pos == valsMap.end()) {
			throw InvalidLinkException(L"Could not resolve parity parameter value");
		}
		result = pos->second;
	}

	void ParseEndpointFlowControl(
				EndpointResourceIdentifierParsers::UrlSplitConstIterator source,
				SerialEndpointAddress::FlowControl &result) {
		std::wstring tmp;
		EndpointResourceIdentifierParsers::ParseUrlParamValue(source, tmp);
		boost::to_lower(tmp);
		typedef std::map<std::wstring, SerialEndpointAddress::FlowControl> Map;
		Map valsMap;
		valsMap.insert(std::make_pair(L"none", SerialEndpointAddress::FC_NONE));
		valsMap.insert(std::make_pair(L"xon/xoff", SerialEndpointAddress::FC_XON_XOFF));
		valsMap.insert(std::make_pair(L"rts/cts", SerialEndpointAddress::FC_RTS_CTS));
		valsMap.insert(std::make_pair(L"dsr/dtr", SerialEndpointAddress::FC_DSR_DTR));
		const Map::const_iterator pos = valsMap.find(tmp);
		if (pos == valsMap.end()) {
			throw InvalidLinkException(L"Could not resolve flow control parameter value");
		}
		result = pos->second;
	}

	void ParseEndpoint(
				const std::wstring &source,
				std::wstring &line,
				int &baudRate,
				unsigned char &dataBits,
				unsigned char &stopBits,
				SerialEndpointAddress::Parity &parity,
				SerialEndpointAddress::FlowControl &flowControl) {

		EndpointResourceIdentifierParsers::UrlSplitConstIterator pathIt
			= boost::make_split_iterator(source, boost::token_finder(boost::is_any_of(L"?&")));

		line = boost::copy_range<std::wstring>(*pathIt++);
		if (line.empty()) {
			throw InvalidLinkException(L"Format is invalid");
		}

		EndpointResourceIdentifierParsers::ParseUrlParam(
			pathIt,
			L"baudrate",
			boost::bind(
				&EndpointResourceIdentifierParsers::ParseUrlParamValue<int>,
				_1,
				boost::ref(baudRate)),
			false);
		EndpointResourceIdentifierParsers::ParseUrlParam(
			pathIt,
			L"databits",
			boost::bind(
				&EndpointResourceIdentifierParsers::ParseUrlParamValue<unsigned char>,
				_1,
				boost::ref(dataBits)),
			false);
		EndpointResourceIdentifierParsers::ParseUrlParam(
			pathIt,
			L"stopbits",
			boost::bind(
				&EndpointResourceIdentifierParsers::ParseUrlParamValue<unsigned char>,
				_1,
				boost::ref(stopBits)),
			false);
		EndpointResourceIdentifierParsers::ParseUrlParam(
			pathIt,
			L"parity",
			boost::bind(&ParseEndpointParity, _1, boost::ref(parity)),
			false);
		EndpointResourceIdentifierParsers::ParseUrlParam(
			pathIt,
			L"flowcontrol",
			boost::bind(&ParseEndpointFlowControl, _1, boost::ref(flowControl)),
			false);

	}

} } }

//////////////////////////////////////////////////////////////////////////

class SerialEndpointAddress::Implementation {

public:

	Implementation()
			: m_baudRate(9600),
			m_dataBits(8),
			m_stopBits(1),
			m_parity(SerialEndpointAddress::P_NONE),
			m_flowControl(SerialEndpointAddress::FC_NONE) {
		//...//
	}

	ACE_DEV_Addr m_addr;

	int m_baudRate;
	unsigned char m_dataBits;
	unsigned char m_stopBits;
	SerialEndpointAddress::Parity m_parity;
	SerialEndpointAddress::FlowControl m_flowControl;

	WString m_identifier;

};

//////////////////////////////////////////////////////////////////////////

SerialEndpointAddress::SerialEndpointAddress()
		: m_pimpl(new Implementation()) {
	//...//
}
		
SerialEndpointAddress::SerialEndpointAddress(const WString &path)
		: m_pimpl(new Implementation()) {
	std::wstring line;
	ParseEndpoint(
		path.GetCStr(),
		line,
		m_pimpl->m_baudRate,
		m_pimpl->m_dataBits,
		m_pimpl->m_stopBits,
		m_pimpl->m_parity,
		m_pimpl->m_flowControl);
	m_pimpl->m_addr.set(line.c_str());
}

SerialEndpointAddress::SerialEndpointAddress(const SerialEndpointAddress &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}
		
SerialEndpointAddress::~SerialEndpointAddress() throw() {
	delete m_pimpl;
}

const SerialEndpointAddress & SerialEndpointAddress::operator =(
			const SerialEndpointAddress &rhs) {
	SerialEndpointAddress(rhs).Swap(*this);
	return *this;
}

void SerialEndpointAddress::Swap(SerialEndpointAddress &rhs) throw() {
	EndpointAddress::Swap(rhs);
	Implementation *tmp = m_pimpl;
	m_pimpl = rhs.m_pimpl;
	rhs.m_pimpl = tmp;
}

const WString & SerialEndpointAddress::GetResourceIdentifier() const {
	if (m_pimpl->m_identifier.IsEmpty()) {
		if (GetLine()[0] != 0) {
			m_pimpl->m_identifier = CreateResourceIdentifier(
				GetLine(),
				GetBaudRate(),
				GetDataBits(),
				GetStopBits(),
				GetParity(),
				GetFlowControl());
		}
	}
	return m_pimpl->m_identifier;
}

UniquePtr<Acceptor> SerialEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &,
			SharedPtr<const EndpointAddress>)
		const {
	throw EndpointHasNotMultiClientsTypeException(
		L"Could not create acceptor for serial line.");
}

bool SerialEndpointAddress::IsHasMultiClientsType(void) const {
	return false;
}

UniquePtr<Connection> SerialEndpointAddress::CreateRemoteConnection(
			const TunnelEx::RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress) 
		const {
	return UniquePtr<Connection>(
		new SerialConnection(*this, ruleEndpoint, ruleEndpointAddress));
}

UniquePtr<Connection> SerialEndpointAddress::CreateLocalConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress) 
		const {
	return UniquePtr<Connection>(
		new SerialConnection(*this, ruleEndpoint, ruleEndpointAddress));
}

UniquePtr<EndpointAddress> SerialEndpointAddress::Clone() const {
	return UniquePtr<EndpointAddress>(new SerialEndpointAddress(*this));
}

const ACE_DEV_Addr & SerialEndpointAddress::GetAceDevAddr() const {
	return const_cast<SerialEndpointAddress *>(this)->GetAceDevAddr();
}

ACE_DEV_Addr & SerialEndpointAddress::GetAceDevAddr() {
	return m_pimpl->m_addr;
}

std::wstring SerialEndpointAddress::GetHumanReadable() const {
	std::wstring result(L"serial://");
	result += GetLine();
	return result;
}

const wchar_t * SerialEndpointAddress::GetLine() const {
	return m_pimpl->m_addr.get_path_name();
}

int SerialEndpointAddress::GetBaudRate() const {
	return m_pimpl->m_baudRate;
}

unsigned char SerialEndpointAddress::GetDataBits() const {
	return m_pimpl->m_dataBits;
}

unsigned char SerialEndpointAddress::GetStopBits() const {
	return m_pimpl->m_stopBits;
}

SerialEndpointAddress::Parity SerialEndpointAddress::GetParity() const {
	return m_pimpl->m_parity;
}

SerialEndpointAddress::FlowControl SerialEndpointAddress::GetFlowControl() const {
	return m_pimpl->m_flowControl;
}

WString SerialEndpointAddress::CreateResourceIdentifier(
			const std::wstring &line,
			int baudRate,
			unsigned char dataBits,
			unsigned char stopBits,
			Parity parity,
			FlowControl flowControl) {
	BOOST_ASSERT(!line.empty());
	BOOST_ASSERT(baudRate > 0);
	std::wostringstream result;
	result
		<< L"serial://" << StringUtil::EncodeUrl(line)
		<< L"?baudrate=" << baudRate
		<< L"&databits=" << dataBits
		<< L"&stopbits=" << stopBits
		<< L"&parity=";
	switch (parity) {
		default:
			BOOST_ASSERT(false);
		case P_NONE:
			result << L"none";
			break;
		case P_ODD:
			result << L"odd";
			break;
		case P_EVEN:
			result << L"even";
			break;
		case P_MARK:
			result << L"mark";
			break;
		case P_SPACE:
			result << L"space";
			break;
	}
	result << L"&flowcontrol=";
	switch (flowControl) {
		default:
			BOOST_ASSERT(false);
		case FC_NONE:
			result << L"none";
			break;
		case FC_XON_XOFF:
			result << StringUtil::EncodeUrl<wchar_t>(L"xon/xoff");
			break;
		case FC_RTS_CTS:
			result << StringUtil::EncodeUrl<wchar_t>(L"rts/cts");
			break;
		case FC_DSR_DTR:
			result << StringUtil::EncodeUrl<wchar_t>(L"dsr/dtr");
			break;
	}
	return result.str().c_str();
}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Serial {

	UniquePtr<EndpointAddress> CreateEndpointAddress(
				Server::ConstRef,
				const WString &resourceIdentifier) {
		return UniquePtr<EndpointAddress>(new SerialEndpointAddress(resourceIdentifier));
	}

} } }
