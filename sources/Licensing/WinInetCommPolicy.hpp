/**************************************************************************
 *   Created: 2009/11/19 0:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__CommPolicies_hpp__0911190056
#define INCLUDED_FILE__TUNNELEX__CommPolicies_hpp__0911190056

namespace TunnelEx { namespace Licensing {

	template<class ClientTrait, bool isTestMode>
	struct WinInetCommPolicy {
	
		// Wininet.lib

	public:

		typedef typename ClientTrait::License License;

	private:

		struct Handles {
			Handles()
					: inet(0),
					connect(0),
					request(0) {
				//...//
			}
			~Handles() {
				if (request) {
					InternetCloseHandle(request);
				}
				if (connect) {
					InternetCloseHandle(connect);
				}
				if (inet) {
					InternetCloseHandle(inet);
				}
			}
			HINTERNET inet;
			HINTERNET connect;
			HINTERNET request;
		};

	public:
	
		static std::string SendRequest(
					const std::string &request,
					const boost::any &clientParam) {
		
			Handles handles;
			
			handles.inet = InternetOpenA(
				TUNNELEX_FAKE_HTTP_CLIENT,
				INTERNET_OPEN_TYPE_PRECONFIG,
				0,
				0,
				0);
			assert(handles.inet);
			if (!handles.inet) {
				License::RegisterError(
					"40D01445-FA96-4872-9975-1BE1002AE636",
					GetLastError(),
					clientParam);
				return std::string();
			}
			
			boost::format host("%1%%3%%2%");
			host % TUNNELEX_LICENSE_SERVICE_SUBDOMAIN % TUNNELEX_DOMAIN % ".";
			handles.connect = InternetConnectA(
				handles.inet, host.str().c_str(), 80, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
			assert(handles.connect);
			if (!handles.connect) {
				License::RegisterError(
					"ACC91479-5559-4D5C-91E0-9DB41ACF239C",
					GetLastError(),
					clientParam);
				return std::string();
			}

			boost::format action("%2%/%1%");
			action % "request" %  "key";
			LPCSTR accept[2] = {
				"*/*",
				NULL
			};
			handles.request = HttpOpenRequestA(
				handles.connect,
				"POST",
				action.str().c_str(),
				0,
				0,
				accept, 
				INTERNET_FLAG_NO_CACHE_WRITE
					| INTERNET_FLAG_NO_UI
					| INTERNET_FLAG_PRAGMA_NOCACHE
					| INTERNET_FLAG_RELOAD,
				INTERNET_NO_CALLBACK);
			assert(handles.request);
			if (!handles.request) {
				License::RegisterError(
					"CB0F4ED3-4437-4DF7-885E-9B941A830944",
					GetLastError(),
					clientParam);
				return std::string();
			}

			std::ostringstream postData;
			postData << "keyRequest=" << Helpers::StringUtil::EncodeUrl(request);

			{
				const char *const headers
					=	"Content-Type: application/x-www-form-urlencoded";
				const BOOL sendResult = HttpSendRequestA(
					handles.request,
					headers,
					DWORD(strlen(headers)),
					const_cast<char *>(postData.str().c_str()),
					DWORD(postData.str().size()));
				if (!sendResult) {
					License::RegisterError(
						"27575909-A818-43FC-8E28-BDF97AFE57A3",
						GetLastError(),
						clientParam);
					return std::string();
				}
			}
		
			std::vector<char> answer;
			{
#				ifdef _DEBUG
					answer.resize(256);
#				else
					answer.resize(1024 * 4);
#				endif
				size_t realAnswerSize = 0;
				for ( ; ; ) {
					DWORD bytesRead = 0;
					BOOL readResult = InternetReadFile(
						handles.request,
						&answer[realAnswerSize],
						DWORD(answer.size() - realAnswerSize),
						&bytesRead);
					assert(readResult);
					if (!readResult) {
						License::RegisterError(
							"48FC91EB-0FB4-42EE-B303-1E0CE11EE364",
							GetLastError(),
							clientParam);
						return std::string();
					} if (bytesRead == 0) {
						break;
					}
					realAnswerSize += bytesRead;
					if (realAnswerSize >= answer.size()) {
#						ifdef _DEBUG
							answer.resize(answer.size() + 256);
#						else
							answer.resize(answer.size() + 1024);
#						endif
					}
				}
				assert(answer.size() >= realAnswerSize);
				answer.resize(realAnswerSize + 1);
				answer[realAnswerSize] = 0;
			}

			return &answer[0];

		}
		
	};
	
} }

#endif // INCLUDED_FILE__TUNNELEX__CommPolicies_hpp__0911190056
