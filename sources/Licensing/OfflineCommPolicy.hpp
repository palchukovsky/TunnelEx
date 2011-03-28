/**************************************************************************
 *   Created: 2010/03/16 22:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: OfflineCommPolicy.hpp 1033 2010-10-15 20:27:41Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__OfflineCommPolicy_hpp__1003162252
#define INCLUDED_FILE__TUNNELEX__OfflineCommPolicy_hpp__1003162252

#include "Version/Version.h"

namespace TunnelEx { namespace Licensing {

	template<class ClientTrait, bool isTestMode>
	struct OfflineCommPolicy {
	
		static std::string SendRequest(const std::string &request) {
		
			using namespace std;
			using namespace boost;
			
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
			} handles;
			
			handles.inet = InternetOpenA(TUNNELEX_NAME_W, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
			BOOST_ASSERT(handles.inet);
			if (!handles.inet) {
				return string();
			}
			
			format host("%3%%1%%2%");
			host % "." % TUNNELEX_DOMAIN % TUNNELEX_LICENSE_SERVICE_SUBDOMAIN;
			handles.connect = InternetConnectA(
				handles.inet, host.str().c_str(), 80, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
			BOOST_ASSERT(handles.connect);
			if (!handles.connect) {
				return string();
			}

			format action("%2%/%1%");
			action % "request" %  "key";
			handles.request = HttpOpenRequestA(
				handles.connect,
				"POST",
				action.str().c_str(),
				0,
				0,
				0, 
				INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_NO_CACHE_WRITE
					| INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI
					| INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD,
				0);
			BOOST_ASSERT(handles.request);
			if (!handles.request) {
				return string();
			}

			ostringstream postData;
			postData << "keyRequest=";
			Helpers::StringUtil::EncodeUrl(request, postData);

			{
				const char *const headers
					=	"Content-Type: application/x-www-form-urlencoded";
				const BOOL sendResult = HttpSendRequestA(
					handles.request,
					headers,
					strlen(headers),
					const_cast<char *>(postData.str().c_str()),
					postData.str().size());
				if (!sendResult) {
					return string();
				}
			}
		
			vector<char> answer;
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
						answer.size() - realAnswerSize,
						&bytesRead);
					BOOST_ASSERT(readResult);
					if (!readResult) {
						return string();
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
				BOOST_ASSERT(answer.size() >= realAnswerSize);
				answer.resize(realAnswerSize + 1);
				answer[realAnswerSize] = 0;
			}

			return &answer[0];

		}
		
	};
	
} }

#endif // INCLUDED_FILE__TUNNELEX__OfflineCommPolicy_hpp__1003162252
