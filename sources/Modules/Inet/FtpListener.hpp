/**************************************************************************
 *   Created: 2008/03/17 22:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__FtpProxy_hpp__0803172242
#define INCLUDED_FILE__TUNNELEX__FtpProxy_hpp__0803172242

#include "InetEndpointAddress.hpp"

#include "Core/Listener.hpp"
#include "Core/Log.hpp"
#include "Core/Server.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	//////////////////////////////////////////////////////////////////////////

	class FtpListener : public PreListener {

	public:

		FtpListener(
				TunnelEx::Server::Ref,
				const TunnelEx::Connection &currentConnection,
				const TunnelEx::Connection &oppositeConnection,
				const char *cmdRegExp,
				const char *cmdStart);
		virtual ~FtpListener();

	public:

		virtual TunnelEx::DataTransferCommand OnNewMessageBlock(
				TunnelEx::MessageBlock &);
	
	protected:

		virtual void ReplaceCmd(
				MessageBlock &messageBlock,
				const std::string &originalIpAddress,
				NetworkPort originalPort);

		virtual const char * GetCmdTemplate() const = 0;

	private:

		static NetworkPort MakePort(
				const std::string &lowNumb,
				const std::string &highNumb);
		static unsigned short MakeHighPortPart(const NetworkPort port) {
			return (port >> 8) & 0xff;
		}
		static unsigned short MakeLowPortPart(const NetworkPort port) {
			return port & 0xff;
		}

		TunnelEx::TunnelRule CreateRule(
				const std::string &originalIpAddress,
				NetworkPort originalPort)
			const;

	private:
		
		TunnelEx::Server::Ref m_server;
		const TunnelEx::Connection &m_currentConnection;
		const TunnelEx::Connection &m_oppositeConnection;
		boost::regex m_cmdRegExpression;
		const std::string m_cmdStart;
		std::string m_buffer;

	};

	//////////////////////////////////////////////////////////////////////////

	class FtpListenerForActiveMode : public FtpListener {
	public:
		FtpListenerForActiveMode(
					TunnelEx::Server::Ref server,
					const TunnelEx::Connection &currentConnection,
					const TunnelEx::Connection &oppositeConnection)
				: FtpListener(
					server,
					currentConnection,
					oppositeConnection,
					"PORT\\s+(\\d+,\\d+,\\d+,\\d+),(\\d+),(\\d+)\r\n",
					"PORT ") {
			//...//
		}
		virtual ~FtpListenerForActiveMode() throw () {
			//...//
		}
	protected:
		virtual void ReplaceCmd(
					MessageBlock &messageBlock,
					const std::string &originalIpAddress,
					NetworkPort originalPort) {
			Log::GetInstance().AppendDebug(
				"Preparing for FTP data-connection forwarding (original endpoint \"%1%:%2%\", active mode).",
				originalIpAddress, originalPort);
			FtpListener::ReplaceCmd(messageBlock, originalIpAddress, originalPort);
		}
		virtual const char * GetCmdTemplate() const {
			return "PORT %1%,%2%,%3%\r\n";
		}
	};

	//////////////////////////////////////////////////////////////////////////

	class FtpListenerForPassiveMode : public FtpListener {
	public:
		FtpListenerForPassiveMode(
					TunnelEx::Server::Ref server,
					const TunnelEx::Connection &currentConnection,
					const TunnelEx::Connection &oppositeConnection)
				: FtpListener(
					server,
					currentConnection,
					oppositeConnection,
					"227\\s+Entering\\sPassive\\sMode\\s+\\((\\d+,\\d+,\\d+,\\d+),(\\d+),(\\d+)\\)\\.?\r\n",
					"227 ") {
			//...//
		}
		virtual ~FtpListenerForPassiveMode() throw() {
			//...//
		}
	protected:
		virtual void ReplaceCmd(
					MessageBlock &messageBlock,
					const std::string &originalIpAddress,
					NetworkPort originalPort) {
			Log::GetInstance().AppendDebug(
				"Preparing for FTP data-connection forwarding (original endpoint \"%1%:%2%\", passive mode).",
				originalIpAddress, originalPort);
			FtpListener::ReplaceCmd(messageBlock, originalIpAddress, originalPort);
		}
		virtual const char * GetCmdTemplate() const {
			return "227 Entering Passive Mode (%1%,%2%,%3%)\r\n";
		}
	};

	//////////////////////////////////////////////////////////////////////////

} } }

#endif // INCLUDED_FILE__TUNNELEX__FtpProxy_hpp__0803172242
