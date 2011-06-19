/**************************************************************************
 *   Created: 2010/03/22 0:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Service.hpp"
#include "Licensing.hpp"

#include "Core/Log.hpp"
#include "Core/Error.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Helpers;
using namespace TunnelEx::Mods::Pathfinder;
using Mods::Inet::Proxy;
using Mods::Inet::ProxyList;
using Mods::Inet::TcpEndpointAddress;

//////////////////////////////////////////////////////////////////////////

namespace {

	Proxy ExtractProxy(const std::string proxyStr) {
		typedef boost::split_iterator<std::string::const_iterator> HostSplitInterator;
		HostSplitInterator hostIt
			= boost::make_split_iterator(proxyStr, boost::first_finder(L":", boost::is_equal()));
		Proxy result;
		const std::string proxyHost = boost::copy_range<std::string>(*hostIt);
		result.host = ConvertString<WString>(proxyHost.c_str()).GetCStr();
		assert(!result.host.empty());
		result.port = 0;
		if (++hostIt != HostSplitInterator()) {
			try {
				result.port = boost::lexical_cast<unsigned short>(*hostIt);
				assert(result.port > 0);
			} catch (const boost::bad_lexical_cast &) {
				assert(false);
			}
		}
		if (result.port > 0 && !result.host.empty()) {
			Log::GetInstance().AppendDebug(
				"Pathfinder found proxy %1%:%2%.",
				proxyHost,
				result.port);
		} else {
			Log::GetInstance().AppendDebug(
				"Pathfinder could not extract proxy from \"%1%\".",
				proxyStr);
		}
		return result;
	} 

	/** @throw ServiceException
	  */
	void SetupProxyAuth(HINTERNET handle, const TcpEndpointAddress &target) {
		if (	target.GetProxyList().size() == 0
				|| target.GetProxyList().begin()->user.empty()) {
			return;
		}
		int setOptResult = InternetSetOptionW(
			handle,
			INTERNET_OPTION_PROXY_USERNAME,
			const_cast<void *>(
				static_cast<const void *>(
					target.GetProxyList().begin()->user.c_str())),
			DWORD(target.GetProxyList().begin()->user.size()));
		if (setOptResult && !target.GetProxyList().begin()->password.empty()) {
			setOptResult = InternetSetOptionW(
				handle,
				INTERNET_OPTION_PROXY_PASSWORD,
				const_cast<void *>(
					static_cast<const void *>(
						target.GetProxyList().begin()->password.c_str())),
				DWORD(target.GetProxyList().begin()->password.size()));
		}
		if (!setOptResult) {
			const Error error(GetLastError());
			WFormat message(
				L"Unable to setup connection to the Pathfinder service: \"%1% (%2%)\","
					L" please check Internet connection and endpoint proxy server settings");
			message % error.GetString().GetCStr() % error.GetErrorNo();
			Log::GetInstance().AppendWarn(
				ConvertString<String>(message.str().c_str()).GetCStr());
			throw ServiceException(message.str().c_str());
		}
	}

}

//////////////////////////////////////////////////////////////////////////

namespace {

	struct AutoInetHandle {
		AutoInetHandle()
				: handle(0) {
			//...//
		}

		~AutoInetHandle() {
			Reset();
		}

		void Reset() throw() {
			if (handle) {
				InternetCloseHandle(handle);
				handle = 0;
			}
		}

		void Swap(AutoInetHandle &rhs) throw() {
			HINTERNET tmp = handle;
			handle = rhs.handle;
			rhs.handle = tmp;
		}

		HINTERNET handle;

	};

}

//////////////////////////////////////////////////////////////////////////

struct ServiceImpl::Handles {

	typedef ACE_Thread_Mutex Mutex;
	typedef ACE_Guard<Mutex> Lock;

	void Reset() throw() {
		AutoInetHandle().Swap(inet);
		AutoInetHandle().Swap(connect);
	}

	Mutex mutex;

	AutoInetHandle inet;
	AutoInetHandle connect;

};


//////////////////////////////////////////////////////////////////////////

struct ServiceImpl::Licensing {

	Licensing()
			: license(licenseState) {
		//...//
	}

	::TunnelEx::Licensing::FsLocalStorageState licenseState;
	::TunnelEx::Licensing::PathfinderLicense license;

};

//////////////////////////////////////////////////////////////////////////

ServiceImpl::ServiceImpl()
		: m_licensing(new Licensing),
		m_handles(new Handles) {
	//...//
}

ServiceImpl::~ServiceImpl() throw() {
	delete m_handles;
	delete m_licensing;
}

void ServiceImpl::InitConnection(const TcpEndpointAddress &target) {

	if (m_handles->connect.handle) {
		return;
	}
	
	std::wstring proxy;
	if (target.GetProxyList().size() > 0) {
		WFormat proxyFormat(L"%1%:%2%");
		proxyFormat
			% target.GetProxyList().begin()->host
			% target.GetProxyList().begin()->port;
		proxyFormat.str().swap(proxy);
	}

	if (Log::GetInstance().IsDebugRegistrationOn()) {
		Log::GetInstance().AppendDebug(
			"Requesting Pathfinder service for endpoint %1%:%2% (using proxy: %3%).",
			 ConvertString<String>(target.GetHostName().c_str()).GetCStr(),
			 target.GetPort(),
			 proxy.empty()
				?	"no"
				:	ConvertString<String>(proxy.c_str()).GetCStr());
	}

	if (proxy.empty()) {
		m_handles->inet.handle
			= InternetOpenW(TUNNELEX_FAKE_HTTP_CLIENT_W, INTERNET_OPEN_TYPE_DIRECT, 0, 0, 0);
	} else {
		m_handles->inet.handle = InternetOpenW(
			TUNNELEX_FAKE_HTTP_CLIENT_W,
			INTERNET_OPEN_TYPE_PROXY,
			proxy.c_str(),
			0,
			0);
	}
	
	if (!m_handles->inet.handle) {
		const Error error(GetLastError());
		m_handles->Reset();
		WFormat message(
			L"Unable open connection to the Pathfinder service: \"%1% (%2%)\","
				L" please check Internet connection and endpoint proxy server settings");
		message % error.GetString().GetCStr() % error.GetErrorNo();
		throw ServiceException(message.str().c_str());
	}

	m_handles->connect.handle = InternetConnectW(
		m_handles->inet.handle,
		L"pathfinder." TUNNELEX_DOMAIN_W,
		80,
		0,
		0,
		INTERNET_SERVICE_HTTP,
		0,
		0);
	if (!m_handles->connect.handle) {
		const Error error(GetLastError());
		m_handles->Reset();
		WFormat message(
			L"Unable to connect to the Pathfinder service: \"%1% (%2%)\","
				L" please check Internet connection and endpoint proxy server settings");
		message % error.GetString().GetCStr() % error.GetErrorNo();
		throw ServiceException(message.str().c_str());
	}

}

bool ServiceImpl::RequestProxy(
			const TcpEndpointAddress &target,
			ProxyList &result)
		const {

	Handles::Lock lock(m_handles->mutex);

	if (m_goodProxy) {
		ProxyList proxy;
		proxy.push_back(*m_goodProxy);
		if (Log::GetInstance().IsDebugRegistrationOn()) {
			Log::GetInstance().AppendDebug(
				"Pathfinder restored proxy %1%:%2%.",
				ConvertString<String>(m_goodProxy->host.c_str()).GetCStr(),
				m_goodProxy->port);
		}
		proxy.swap(result);
		return false;
	}

	std::wostringstream requestStr;
	requestStr << L"proxy/list?";
	{
		std::wstring key
			= ConvertString<WString>(m_licensing->license.GetKey().c_str()).GetCStr();
		if (key.empty()) {
			key = (WFormat(L"%1%%1%%2%%1%%2%%1%%2%%1%%2%%1%%1%%1%") % L"0000" % L"-").str();
		}
		requestStr << L"lk=" << StringUtil::EncodeUrl(key);
	}
	requestStr
		<< "&e="
		<< StringUtil::EncodeUrl(
			(WFormat(L"%1%:%2%") % target.GetHostName() % target.GetPort()).str());
	requestStr << "&v=" << StringUtil::EncodeUrl<wchar_t>(TUNNELEX_VERSION_FULL_W);
	requestStr << "&p=" << target.GetProxyList().size();

	try {
		const_cast<ServiceImpl *>(this)->InitConnection(target);
	} catch (const ServiceException &ex) {
		Log::GetInstance().AppendWarn(
			ConvertString<String>(ex.GetWhat()).GetCStr());
		throw;
	}

	AutoInetHandle request;
	request.handle = HttpOpenRequestW(
		m_handles->connect.handle,
		L"GET",
		requestStr.str().c_str(),
		0,
		0,
		0, 
		INTERNET_FLAG_NO_CACHE_WRITE
			| INTERNET_FLAG_KEEP_CONNECTION
			| INTERNET_FLAG_NO_UI
			| INTERNET_FLAG_PRAGMA_NOCACHE
			| INTERNET_FLAG_RELOAD,
		0);
	if (!request.handle) {
		const Error error(GetLastError());
		m_handles->Reset();
		WFormat message(
			L"Unable to send request to the Pathfinder service: \"%1% (%2%)\","
				L" please check Internet connection and endpoint proxy server settings");
		message % error.GetString().GetCStr() % error.GetErrorNo();
		Log::GetInstance().AppendWarn(
			ConvertString<String>(message.str().c_str()).GetCStr());
		throw ServiceException(message.str().c_str());
	}

	SetupProxyAuth(request.handle, target);

	if (!HttpSendRequestW(request.handle, NULL, 0, NULL, 0)) {
		const Error error(GetLastError());
		m_handles->Reset();
		WFormat message(
			L"Unable to communicate with the Pathfinder service: \"%1% (%2%)\","
				L" please check Internet connection and endpoint proxy server settings");
		message % error.GetString().GetCStr() % error.GetErrorNo();
		Log::GetInstance().AppendWarn(
			ConvertString<String>(message.str().c_str()).GetCStr());
		throw ServiceException(message.str().c_str());
	}

	std::vector<char> answer;
	{
#		ifdef _DEBUG
			answer.resize(10);
#		else
			answer.resize(1024 * 1);
#		endif
		size_t realAnswerSize = 0;
		for ( ; ; ) {
			DWORD bytesRead = 0;
			const BOOL readResult = InternetReadFile(
				request.handle,
				&answer[realAnswerSize],
				DWORD(answer.size()) - realAnswerSize,
				&bytesRead);
			assert(readResult);
			if (!readResult) {
				const Error error(GetLastError());
				WFormat message(
					L"Could not read Pathfinder service response: \"%1% (%2%)\","
						L" please check Internet connection and endpoint proxy server settings");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				Log::GetInstance().AppendWarn(
					ConvertString<String>(message.str().c_str()).GetCStr());
				throw ServiceException(message.str().c_str());
			} else if (bytesRead == 0) {
				break;
			}
			realAnswerSize += bytesRead;
			if (realAnswerSize >= answer.size()) {
#				ifdef _DEBUG
					answer.resize(answer.size() + 10);
#				else
					answer.resize(answer.size() + 1024);
#				endif
			}
		}
		assert(answer.size() >= realAnswerSize);
		answer.resize(realAnswerSize + 1);
		answer[realAnswerSize] = 0;
	}

	request.Reset();
	lock.release();

#	ifdef DEV_VER
	{
		boost::filesystem::path dumpPath = GetModuleFilePathA().branch_path();
		dumpPath /= "PathfindServiceAnswer.html";
		std::ofstream f(dumpPath.string().c_str(), std::ios::trunc | std::ios::binary);
		f.write(&answer[0], std::streamsize(answer.size()));
	}
#	endif

	{
		typedef boost::split_iterator<std::vector<char>::iterator> AnswerSplitIterator;
		AnswerSplitIterator lineIt
			= boost::make_split_iterator(answer, boost::first_finder("\n", boost::is_equal()));
		AnswerSplitIterator resultCodeIt
			= boost::make_split_iterator(answer, boost::token_finder(!boost::is_digit()));
		int resultCode;
		try {
			resultCode = begin(*resultCodeIt) != end(*resultCodeIt)
				?	boost::lexical_cast<int>(*resultCodeIt)
				:	(target.GetProxyList().size() > 0 ? 201 : 200);
		} catch (const boost::bad_lexical_cast &) {
			assert(false);
			resultCode = 200;
		}
		Log::GetInstance().AppendDebug("Pathfinder service status code: %1%", resultCode);
		const wchar_t *const errorTemplate = L"Pathfinder service could not find network path: %s";
		switch (resultCode) {
			case 1:
				// custom
				{
					std::string error(begin(*++resultCodeIt), end(*lineIt));
					boost::trim_if(error, boost::is_space() || boost::is_cntrl());
					WFormat message(errorTemplate);
					message % 
						(!error.empty()
							?	ConvertString<WString>(error.c_str()).GetCStr()
							:	L"unknown error");
					throw ServiceException(
						message.str().c_str());
				}
			case 100:
				// access denied
				{
					Log::GetInstance().AppendWarn(
						"Pathfinder Online Service using is not permitted."
							" The functionality you have requested requires"
							" a License Upgrade. Please purchase a License that"
							" will enable this feature at http://" TUNNELEX_DOMAIN "/order"
							" or get free trial at http://" TUNNELEX_DOMAIN "/order/trial.");
					WFormat message(errorTemplate);
					message % L"Service using is not permitted, License Upgrade required";
					throw LicensingException(message.str().c_str());
				}
			case 0:
				{
					ProxyList proxyList;
					while (++lineIt != AnswerSplitIterator()) {
						std::string proxyStr = boost::copy_range<std::string>(*lineIt);
						boost::trim_if(proxyStr, boost::is_space() || boost::is_cntrl());
						assert(!proxyStr.empty());
						if (!proxyStr.empty()) {
							const Proxy proxy = ExtractProxy(proxyStr);
							if (!proxy.host.empty() && proxy.port > 0) {
								proxyList.push_back(proxy);
							}
						}
					}
					assert(proxyList.size() > 0);
					if (proxyList.size() > 0) {
						proxyList.swap(result);
						break;
					}
				}
				assert(false);
			default:
				assert(false);
			case 200:
				{
					WFormat message(errorTemplate);
					message
						%	L"service temporarily unavailable,"
								L" please try to use it later";
					throw ServiceException(message.str().c_str());
				}
			case 201:
				{
					WFormat message(errorTemplate);
					message
						%	L"could not connect to the online service,"
								L" please check Internet connection"
								L" and endpoint proxy server settings";
					Log::GetInstance().AppendWarn(
						ConvertString<String>(message.str().c_str()).GetCStr());
					throw ServiceException(message.str().c_str());
				}
		}
	
	}

	return true;

}

void ServiceImpl::Report(
			const wchar_t *const result,
			const TcpEndpointAddress &target,
			const Proxy &proxy,
			bool isSuccess)
		const
		throw() {
	
	Handles::Lock lock(m_handles->mutex);

	if (isSuccess) {
		if (m_goodProxy && proxy == *m_goodProxy) {
			return;
		}
		m_goodProxy = proxy;
		if (Log::GetInstance().IsDebugRegistrationOn()) {
			Log::GetInstance().AppendDebug(
				"Pathfinder cached proxy %1%:%2%.",
				ConvertString<String>(m_goodProxy->host.c_str()).GetCStr(),
				m_goodProxy->port);
		}
	} else {
		m_goodProxy.reset();
	}

	std::wostringstream requestStr;
	requestStr << L"report/" << result;

	try {
		const_cast<ServiceImpl *>(this)->InitConnection(target);
	} catch (const ServiceException &ex) {
		Log::GetInstance().AppendDebug(
			ConvertString<String>(ex.GetWhat()).GetCStr());
		return;
	}

	AutoInetHandle request;
	request.handle = HttpOpenRequestW(
		m_handles->connect.handle,
		L"POST",
		requestStr.str().c_str(),
		0,
		0,
		0, 
		INTERNET_FLAG_NO_CACHE_WRITE
			| INTERNET_FLAG_KEEP_CONNECTION
			| INTERNET_FLAG_NO_UI
			| INTERNET_FLAG_PRAGMA_NOCACHE
			| INTERNET_FLAG_RELOAD,
		0);
	if (!request.handle) {
		const Error error(GetLastError());
		m_handles->Reset();
		WFormat message(
			L"Unable to send request to the Pathfinder service: \"%1% (%2%)\","
				L" please check Internet connection and endpoint proxy server settings");
		message % error.GetString().GetCStr() % error.GetErrorNo();
		Log::GetInstance().AppendDebug(
			ConvertString<String>(message.str().c_str()).GetCStr());
		return;
	}

	try {
		SetupProxyAuth(request.handle, target);
	} catch (const ServiceException &ex) {
		Log::GetInstance().AppendDebug(
			ConvertString<String>(ex.GetWhat()).GetCStr());
		return;
	}

	const char *const headers
		= "Content-Type: application/x-www-form-urlencoded";

	std::ostringstream postDataStr;
	{
		std::string key = m_licensing->license.GetKey();
		if (key.empty()) {
			key = (Format("%1%%1%%2%%1%%2%%1%%2%%1%%2%%1%%1%%1%") % "0000" % "-").str();
		}
		postDataStr << "lk=" << StringUtil::EncodeUrl(key);
	}
	postDataStr << "&e=";
	postDataStr << StringUtil::EncodeUrl(
		(Format("%1%:%2%")
				% ConvertString<String>(target.GetHostName().c_str()).GetCStr()
				% target.GetPort())
			.str());
	postDataStr
		<< "&v="
		<< StringUtil::EncodeUrl<char>(ConvertString<String>(TUNNELEX_VERSION_FULL_W).GetCStr());
	assert(target.GetProxyList().size() >= 1);
	postDataStr << "&p=" << (target.GetProxyList().size() - 1);
	postDataStr
		<< "&h="
		<< StringUtil::EncodeUrl(
			(Format("%1%:%2%")
					% ConvertString<String>(proxy.host.c_str()).GetCStr()
					% proxy.port)
				.str());

	if (	!HttpSendRequestA(
				request.handle,
				headers,
				DWORD(strlen(headers)),
				const_cast<char *>(postDataStr.str().c_str()),
				DWORD(postDataStr.str().size()))) {
		const Error error(GetLastError());
		m_handles->Reset();
		WFormat message(
			L"Unable to communicate with the Pathfinder service: \"%1% (%2%)\","
				L" please check Internet connection and endpoint proxy server settings");
		message % error.GetString().GetCStr() % error.GetErrorNo();
		Log::GetInstance().AppendDebug(
			ConvertString<String>(message.str().c_str()).GetCStr());
		return;
	}

}
