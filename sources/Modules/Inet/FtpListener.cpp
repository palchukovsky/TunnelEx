/**************************************************************************
 *   Created: 2008/03/17 22:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "FtpListener.hpp"
#include "Licensing.hpp"

#include "Core/Rule.hpp"
#include "Core/Server.hpp"
#include "Core/Connection.hpp"
#include "Core/EndpointAddress.hpp"
#include "Core/MessageBlock.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Mods::Inet;

FtpListener::FtpListener(
			TunnelEx::Server::Ref server,
			const TunnelEx::Connection &currentConnection,
			const TunnelEx::Connection &oppositeConnection,
			const char *cmdRegExp,
			const char *cmdStart)
		: m_server(server),
		m_currentConnection(currentConnection),
		m_oppositeConnection(oppositeConnection),
		m_cmdRegExpression(cmdRegExp, boost::regex_constants::icase),
		m_cmdStart(cmdStart) {
	static Licensing::FsLocalStorageState licenseState;
	static Licensing::FtpTunnelLicense license(&licenseState);
	if (	!m_currentConnection.GetRuleEndpoint().IsCombined()
			|| !m_currentConnection.GetRuleEndpoint().CheckCombinedAddressType<TcpEndpointAddress>()
			|| !m_oppositeConnection.GetRuleEndpoint().IsCombined()
			|| !m_oppositeConnection.GetRuleEndpoint().CheckCombinedAddressType<TcpEndpointAddress>()) {
		throw LogicalException(L"FTP forwarder works only with combined inet endpoints");
	} else if (!license.IsFeatureAvailable(true)) {
		Log::GetInstance().AppendWarn(
			"Could not activate FTP tunneling."
				" The functionality you have requested requires"
				" a License Upgrade. Please purchase a License that"
				" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
				" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
		throw LocalException(
			L"Could not activate FTP tunneling, License Upgrade required");
	}
}

FtpListener::~FtpListener() throw() {
	//...//
}

inline NetworkPort FtpListener::MakePort(
			const std::string &highStr,
			const std::string &lowStr) {
	try {
		const unsigned short high = boost::lexical_cast<unsigned short>(highStr);
		const unsigned short low = boost::lexical_cast<unsigned short>(lowStr);
		return ((high & 0xff) << 8) | (low & 0xff);
	} catch (const boost::bad_lexical_cast &) {
		return std::numeric_limits<NetworkPort>::max();
	}
}

TunnelRule FtpListener::CreateRule(
			const std::string &originalIpAddress,
			NetworkPort originalPort)
		const {

	TunnelRule result;
	WString wStrIpAddressBuffer;
	std::string inputIpAddress;

	{
		const SharedPtr<const EndpointAddress> currentInputRuleEndpointHolder(
			m_currentConnection.GetRuleEndpointAddress());
		const TcpEndpointAddress &currentInputRuleEndpoint
			= *boost::polymorphic_downcast<const TcpEndpointAddress *>(
				currentInputRuleEndpointHolder.Get());
		const AutoPtr<const TcpEndpointAddress> currentInputEndpoint(
			boost::polymorphic_downcast<const TcpEndpointAddress *>(
				m_currentConnection.GetLocalAddress().Release()));
		AutoPtr<TcpEndpointAddress> input(
			new TcpEndpointAddress(
				ConvertString(
						currentInputEndpoint->GetHostAddress(),
						wStrIpAddressBuffer)
					.GetCStr(),
				0,
				currentInputRuleEndpoint.GetServer()));
		const AutoPtr<const EndpointAddress> remoteAddress(
			m_currentConnection.GetRemoteAddress());
		input->CopyCertificate(
			currentInputRuleEndpoint,
			*boost::polymorphic_downcast<const TcpEndpointAddress *>(
				remoteAddress.Get()));
		result.GetInputs().Append(RuleEndpoint(input, true));
	}

	{
		AutoPtr<TcpEndpointAddress> destination(
			boost::polymorphic_downcast<TcpEndpointAddress *>(
				m_oppositeConnection.GetRuleEndpointAddress()->Clone().Release()));
		const AutoPtr<const EndpointAddress> oppositeAddress(
			m_oppositeConnection.GetRemoteAddress());
		destination->CopyRemoteCertificate(
			*boost::polymorphic_downcast<const TcpEndpointAddress *>(
				oppositeAddress.Get()));
		destination->SetHost(
			ConvertString(originalIpAddress.c_str(), wStrIpAddressBuffer).GetCStr());
		destination->SetPort(originalPort);
		result.GetDestinations().Append(RuleEndpoint(destination, false));
	}

	result.SetAcceptedConnectionsLimit(1);
	
	return result;

}

void FtpListener::ReplaceCmd(
			MessageBlock &messageBlock,
			const std::string &originalIpAddress,
			NetworkPort originalPort) {

	const TunnelRule dataConnectionRule = CreateRule(originalIpAddress, originalPort);
	if (Log::GetInstance().IsDebugRegistrationOn()) {
		Log::GetInstance().AppendDebug(
			"Adding rule \"%1%\" for FTP data-connection...",
			ConvertString<String>(dataConnectionRule.GetUuid()).GetCStr());
	}
	if (!m_server.UpdateRule(dataConnectionRule)) {
		return;
	}

	try {
		
		const WString inputAddress = dataConnectionRule
			.GetInputs()[0]
			.GetCombinedTypedAddress<const InetEndpointAddress>()
			.GetHostName()
			.c_str();
		std::string outcomingIpForCmd = ConvertString<String>(inputAddress).GetCStr();
		boost::replace_all(outcomingIpForCmd, ".", ",");

		const AutoPtr<const InetEndpointAddress> input(
			boost::polymorphic_downcast<InetEndpointAddress *>(
				m_server.GetRealOpenedEndpointAddress(
						dataConnectionRule.GetUuid(),
						dataConnectionRule.GetInputs()[0].GetUuid())
					.Release()));
		const NetworkPort port = input->GetPort();

		Format newCmd(GetCmdTemplate());
		newCmd % outcomingIpForCmd;
		newCmd % MakeHighPortPart(port);
		newCmd % MakeLowPortPart(port);

		messageBlock.SetData(newCmd.str().c_str(), newCmd.size());

	} catch (...) {
		try {
			m_server.DeleteRule(dataConnectionRule.GetUuid());
		} catch (...) {
			Format message(
				"Unknown system error occurred: %1%:%2%."
					" Please restart the service"
					" and contact product support to resolve this issue."
					" %3% %4%");
			message
				% __FILE__ % __LINE__
				% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
			Log::GetInstance().AppendFatalError(message.str());
			assert(false);
		}
		throw;
	}

}

DataTransferCommand FtpListener::OnNewMessageBlock(MessageBlock &messageBlock) {

	const size_t dataLen = messageBlock.GetUnreadedDataSize();
	assert(dataLen > 0);
	const unsigned int maxPacketLenToParse = 70;
	if (dataLen > maxPacketLenToParse || dataLen < 1) {
		return DATA_TRANSFER_CMD_SEND_PACKET;
	}
	const size_t prevBufferSize = m_buffer.size();
	const char *const data = messageBlock.GetData();

	if (prevBufferSize == 0) {
		size_t i = 0;
		foreach (char ch, m_cmdStart) {
			if (i >= dataLen) {
				break;
			} else if (ch != data[i++]) {
				return DATA_TRANSFER_CMD_SEND_PACKET;
			}
		}
	}

	m_buffer.append(messageBlock.GetData(), dataLen);
	if (m_buffer.size() > maxPacketLenToParse) {
		assert(false);
		Format message("File Transfer Protocol error: failed to parse \"%1%\".");\
		message % m_buffer;
		Log::GetInstance().AppendWarn(message.str().c_str());
		messageBlock.SetData(m_buffer.c_str(), m_buffer.size());
		m_buffer.clear();
		return DATA_TRANSFER_CMD_SEND_PACKET;
	} else if (prevBufferSize > 0 && prevBufferSize < m_cmdStart.size()) {
		for (
				size_t i = prevBufferSize;
				i < m_buffer.size() && i < m_cmdStart.size();
				++i) {
			if (m_buffer[i] != m_cmdStart[i]) {
				messageBlock.SetData(m_buffer.c_str(), m_buffer.size());
				m_buffer.clear();
				return DATA_TRANSFER_CMD_SEND_PACKET;
			}
		}
	}

	boost::smatch what;
	if (!boost::regex_match(m_buffer, what, m_cmdRegExpression)) {
		return DATA_TRANSFER_CMD_SKIP_PACKET;
	}

	std::string originalIpAddress(what[1]);
	boost::replace_all(originalIpAddress, ",", ".");
	try {
		ReplaceCmd(messageBlock, originalIpAddress, MakePort(what[2], what[3]));
		if (Log::GetInstance().IsDebugRegistrationOn()) {
			std::string oldCmd = m_buffer;
			boost::trim(oldCmd);
			std::string newCmd(messageBlock.GetData(), messageBlock.GetUnreadedDataSize());
			boost::trim(newCmd);
			Log::GetInstance().AppendDebug(
				"FTP-command has been changed from \"%1%\" to \"%2%\".", oldCmd, newCmd);
		}
	} catch (const ::TunnelEx::LocalException &ex) {
		Format message("Failed to change FTP-command (%1%).");
		message % ConvertString<String>(ex.GetWhat()).GetCStr();
		Log::GetInstance().AppendError(message.str());
		messageBlock.SetData(m_buffer.c_str(), m_buffer.size());
	}
	m_buffer.clear();

	return DATA_TRANSFER_CMD_SEND_PACKET;

}

namespace TunnelEx { namespace Mods { namespace Inet {
	
	SharedPtr<PreListener> CreateActiveFtpListener(
			Server::Ref server,
			const RuleEndpoint::ListenerInfo &,
			const TunnelRule &,
			const Connection &currentConnection,
			const Connection &oppositeConnection) {
		return SharedPtr<PreListener>(
			new FtpListenerForActiveMode(server, currentConnection, oppositeConnection));
	}

	SharedPtr<PreListener> CreatePassiveFtpListener(
			Server::Ref server,
			const RuleEndpoint::ListenerInfo &,
			const TunnelRule &,
			const Connection &currentConnection,
			const Connection &oppositeConnection) {
		return SharedPtr<PreListener>(
			new FtpListenerForPassiveMode(server, currentConnection, oppositeConnection));
	}

} } }
