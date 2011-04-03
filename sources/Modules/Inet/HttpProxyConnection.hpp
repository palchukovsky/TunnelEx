/**************************************************************************
 *   Created: 2009/12/15 17:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: HttpProxyConnection.hpp 1133 2011-02-22 23:18:56Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__HttpProxyConnection_hpp__091215
#define INCLUDED_FILE__TUNNELEX__HttpProxyConnection_hpp__091215

#include "OutcomingTcpConnection.hpp"
#include "ProxyExceptions.hpp"

#include <TunnelEx/MessageBlock.hpp>
#include <TunnelEx/Log.hpp>
#include <TunnelEx/String.hpp>

namespace TunnelEx { namespace Mods { namespace Inet {

	template<typename ConnectionTypename>
	class HttpProxyConnection : public ConnectionTypename {

	public:

		typedef ConnectionTypename Base;

	public:

		explicit HttpProxyConnection(
					const ACE_INET_Addr &proxyAddress,
					const TunnelEx::RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Base(proxyAddress, ruleEndpoint, ruleEndpointAddress),
				m_address(
					*boost::polymorphic_downcast<const TcpEndpointAddress *>(
						GetRuleEndpointAddress().Get())),
				m_isSetupComplited(false) {
			//...//
		}

		virtual ~HttpProxyConnection() {
			if (!m_isSetupComplited) {
				try {
					Format message(
						"Proxy server configuration for connection %1% has not been successfully completed."
							" Please check endpoint proxy server settings.");
					message % GetInstanceId();
					Log::GetInstance().AppendWarn(message.str().c_str());
				} catch (...) {
					BOOST_ASSERT(false);
				}
			}
		}

	protected:

		virtual void Setup() {
			StartReadRemote();
			m_currentProxy = m_address.GetProxyList().begin();
			BOOST_ASSERT(m_currentProxy != m_address.GetProxyList().end());
			SetupCurrentProxy();
		}

		virtual void ReadRemote(MessageBlock &messageBlock) {
			
			using namespace std;
			using namespace boost;

			if (m_isSetupComplited) {
				Base::ReadRemote(messageBlock);
				return;
			}
			
			m_currentProxyAnswer
				+= string(messageBlock.GetData(), messageBlock.GetUnreadedDataSize());
			if (!contains(m_currentProxyAnswer, "\r\n\r\n")) {
				return;
			}
			
			string proxyAnswer;
			proxyAnswer.swap(m_currentProxyAnswer);
#			if defined(_DEBUG) || defined(TEST)
			{
				filesystem::path dumpPath
					= Helpers::GetModuleFilePathA().branch_path();
				dumpPath /= "HttpProxyAnswer.html";
				ofstream f(dumpPath.string().c_str(), ios::trunc | ios::binary);
				f.write(&proxyAnswer[0], streamsize(proxyAnswer.size()));
			}
#			endif
			
			try {
				ExtractHeaderInfo(proxyAnswer);
				if (Log::GetInstance().IsDebugRegistrationOn()) {
					Log::GetInstance().AppendDebug(
						"Proxy server %1%:%2% setup completed for %3%.",
						ConvertString<String>(m_currentProxy->host.c_str()).GetCStr(),
						m_currentProxy->port,
						GetInstanceId());
				}
				++m_currentProxy;
				m_isSetupComplited
					= m_currentProxy == m_address.GetProxyList().end();
				if (m_isSetupComplited) {
					StopReadRemote();
					Log::GetInstance().AppendDebug(
						"Proxy servers setup completed for %1%.",
						GetInstanceId());
					Base::Setup();
				} else {
					SetupCurrentProxy();
				}
			} catch (const ProxyWorkingException &ex) {
				m_isSetupComplited = true;
				StopReadRemote();
				WFormat message(
					L"Failed to open connection %2% through proxy server: %1%");
				message % ex.GetWhat() % GetInstanceId();
				CancelSetup(message.str().c_str());
			}

		}

	protected:

		void SetupCurrentProxy() {
			ProxyList::const_iterator nextProxy = m_currentProxy;
			std::advance(nextProxy, 1);
			if (nextProxy != m_address.GetProxyList().end()) {
				SetupProxy(*m_currentProxy, nextProxy->host, nextProxy->port);
			} else {
				SetupProxy(
					*m_currentProxy,
					m_address.GetHostName(),
					m_address.GetPort());
			}
		}

		void SetupProxy(
					const Proxy &proxy,
					const std::wstring &targetHost,
					const NetworkPort targetPort) {

			using namespace std;

			if (Log::GetInstance().IsDebugRegistrationOn()) {
				Log::GetInstance().AppendDebug(
					"Setup proxy server %1%:%2% for %3%...",
					ConvertString<String>(proxy.host.c_str()).GetCStr(),
					proxy.port,
					GetInstanceId());
			}

			String targetHostA;
			ConvertString(targetHost.c_str(), targetHostA);

			ostringstream cmd;
			cmd
				<< "CONNECT "
					<< targetHostA.GetCStr() << ":" << targetPort
					<< " HTTP/1.0"
					<< "\r\n"
				<< "User-Agent: " TUNNELEX_FAKE_HTTP_CLIENT "\r\n"
				<< "Host: " << targetHostA.GetCStr() << ":" << targetPort << "\r\n"
				<< "Content-Length: 0\r\n"
				<< "Proxy-Connection: Keep-Alive\r\n";
			if (!proxy.user.empty()) {
				Helpers::Crypto::InBase64Stream base64(false);
				base64
					<< ConvertString<String>(proxy.user.c_str()).GetCStr()
					<< ":"
					<< ConvertString<String>(proxy.password.c_str()).GetCStr();
				cmd << "Proxy-Authorization: Basic " << base64.GetString() << "\r\n";
			}
			cmd << "\r\n";

			const string &cmdStr = cmd.str();
			SendToRemote(cmdStr.c_str(), cmdStr.size());
		
		}

	private:

		static void ExtractHeaderInfo(const std::string &headers) {
			using namespace std;
			using namespace boost;
			bool isAnswerChecked = false;
			typedef split_iterator<string::const_iterator> It;
			for (	It i = make_split_iterator(headers, first_finder("\r\n", is_equal()));
					i != It();
					++i) {
				if (!isAnswerChecked) {
					CheckAnswer(*i);
					isAnswerChecked = true;
				} else {
					// extract other headers here
					break;
				}
			}
			if (!isAnswerChecked) {
				throw ProxyWorkingException(L"Failed to get proxy server answer");
			}
		}

		template<class Range>
		static void CheckAnswer(const Range &line) {
			using namespace boost;
			smatch what;
			const regex expr("HTTP/\\d\\.\\d\\s+(\\d+)\\s+(.+)", regex_constants::icase);
			if (!regex_match(begin(line), end(line), what, expr)) {
				throw ProxyWorkingException(L"Failed to parse proxy server HTTP-headers");
			} else if (what[1] != "200") {
				Format format("%1% (error code: %2%)");
				format % what[2] % what[1];
				throw ProxyWorkingException(ConvertString<WString>(format.str().c_str()).GetCStr());
			}
		}

	private:

		const TcpEndpointAddress &m_address;

		bool m_isSetupComplited;

		std::string m_currentProxyAnswer;
		ProxyList::const_iterator m_currentProxy;

	};

} } }

#endif // #define INCLUDED_FILE__TUNNELEX__HttpProxyConnection_hpp__091215
