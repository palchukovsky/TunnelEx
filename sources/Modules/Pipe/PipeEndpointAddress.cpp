/**************************************************************************
 *   Created: 2008/11/27 1:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "PipeEndpointAddress.hpp"
#include "PipeConnectionAcceptor.hpp"
#include "OutcomingPipeConnection.hpp"
#include "Core/Server.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Pipe;

PipeEndpointAddress::PipeEndpointAddress(const WString &pathIn) {
	std::wstring path(pathIn.GetCStr());
	//! @todo: make one expression
	if (
			!boost::regex_match(path, boost::wregex(L"([ \\d\\W_A-Za-z]*/+)*[ \\d\\W_A-Za-z]+"))
			|| boost::find_first(path, L"\\")
			|| boost::regex_match(path, boost::wregex(L"[/\\\\]+"))) {
		throw InvalidLinkException(
			(WFormat(L"Could not parse pipe resource identifier \"%1%\".")
				% path).str().c_str());
	}
	boost::replace_all(path, L"/", L"\\");
	m_impl.set(path.c_str());
}

const WString & PipeEndpointAddress::GetResourceIdentifier() const {
	if (m_identifier.IsEmpty()) {
		if (m_impl.get_path_name()[0] != 0) {
			WString identifier = L"pipe://";
			assert(boost::starts_with(m_impl.get_path_name(), L"\\\\.\\pipe\\"));
			identifier += boost::starts_with(m_impl.get_path_name(), L"\\\\.\\pipe\\")
				?	m_impl.get_path_name() + 9
				:	m_impl.get_path_name();
			const WString::SizeType length = identifier.GetLength();
			for (WString::SizeType i = 8; i < length; ++i) {
				if (identifier[i] == L'\\') {
					identifier[i] = L'/';
				}
			}
			m_identifier.Swap(identifier);
		}
	}
	return m_identifier;
}

UniquePtr<Acceptor> PipeEndpointAddress::OpenForIncomingConnections(
			const RuleEndpoint &ruleEndpoint,
			const SharedPtr<const EndpointAddress> ruleEndpointAddress)
		const {
	return UniquePtr<Acceptor>(
		new PipeConnectionAcceptor(
			*this,
			ruleEndpoint,
			ruleEndpointAddress));
}

bool PipeEndpointAddress::IsHasMultiClientsType(void) const {
	return true;
}

UniquePtr<Connection> PipeEndpointAddress::CreateRemoteConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress) 
		const {
	return UniquePtr<Connection>(
		new OutcomingPipeConnection(*this, ruleEndpoint, ruleEndpointAddress));
}

UniquePtr<Connection> PipeEndpointAddress::CreateLocalConnection(
			const RuleEndpoint &ruleEndpoint,
			SharedPtr<const EndpointAddress> ruleEndpointAddress) 
		const {
	return UniquePtr<Connection>(
		new OutcomingPipeConnection(*this, ruleEndpoint, ruleEndpointAddress));
}

UniquePtr<EndpointAddress> PipeEndpointAddress::Clone() const {
	return UniquePtr<EndpointAddress>(new PipeEndpointAddress(*this));
}

//////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Mods { namespace Pipe {

	UniquePtr<EndpointAddress> CreateEndpointAddress(
				Server::ConstRef,
				const WString &resourceIdentifier) {
		return UniquePtr<EndpointAddress>(new PipeEndpointAddress(resourceIdentifier));
	}

} } }
