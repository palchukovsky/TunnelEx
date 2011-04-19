/**************************************************************************
 *   Created: 2008/02/03 22:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "UpdateChecker.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

class UpdateChecker::Thread : public wxThread {

public:

	explicit Thread(wxWindow *parent, const Stat &stat)
			: wxThread(wxTHREAD_JOINABLE),
			m_parent(parent),
			m_versionResolved(false),
			m_isShutdownMode(false),
			m_stat(stat) {
		//...//
	}

private:

	wxWindow *m_parent;
	UpdateChecker::Version m_currentVersion;
	bool m_versionResolved;
	bool m_isShutdownMode;
	const Stat m_stat;

public:

	virtual ExitCode Entry() {
#		if !defined(TEST) && !defined(_DEBUG)
			CheckNewRelease();
#		endif
		UpdateChecker::Event evt(
			EVT_UPDATE_CHECK_ACTION,
			UpdateChecker::Event::ID_NEW_RELEASE_CHECKED);
		wxPostEvent(m_parent, evt);
		return 0;
	}

public:

	bool IsReleaseNew() const {
		return m_versionResolved
			&&	(m_currentVersion.majorHigh > TUNNELEX_VERSION_MAJOR_HIGH
				||	(m_currentVersion.majorHigh == TUNNELEX_VERSION_MAJOR_HIGH
					&& m_currentVersion.majorLow > TUNNELEX_VERSION_MAJOR_LOW)
				||	(m_currentVersion.majorHigh == TUNNELEX_VERSION_MAJOR_HIGH
					&& m_currentVersion.majorLow == TUNNELEX_VERSION_MAJOR_LOW
					&& m_currentVersion.minorHigh > TUNNELEX_VERSION_MINOR_HIGH)
				||	(m_currentVersion.majorHigh == TUNNELEX_VERSION_MAJOR_HIGH
					&& m_currentVersion.majorLow == TUNNELEX_VERSION_MAJOR_LOW
					&& m_currentVersion.minorHigh == TUNNELEX_VERSION_MINOR_HIGH
					&& m_currentVersion.minorLow > TUNNELEX_VERSION_MINOR_LOW));
	}

	const UpdateChecker::Version & GetReleaseVersion() const {
		return m_currentVersion;
	}

	void PrepareShutdown() {
		m_isShutdownMode = true;
	}

protected:

	void CheckNewRelease() {

		BOOST_ASSERT(!m_versionResolved);

		WFormat url(
			L"http://" TUNNELEX_DOMAIN_W L"/version/xml/"
				L"?compareVer=" TUNNELEX_VERSION_FULL_W
				L"&uuid=%1%" // installation ID
				L"&s=%2%" // rule set size
				L"&lk=%3%" // license key or zero
				L"&t=%4%"); // is trial (1 or 0)
		url
			% m_stat.installationUuid
			% m_stat.rulesCount
			% m_stat.licenseKey
			% (m_stat.isTrialMode ? L'1' : L'0');

		std::wstring versionStr;
		if (GetLastReleaseInfo(url.str().c_str(), versionStr)) {
			UpdateChecker::Version version;
			if (ResolveVersion(versionStr, version)) {
				m_currentVersion = version;
				m_versionResolved = true;
			}
		}

	}

	bool ResolveVersion(const std::wstring &version, UpdateChecker::Version &result) const {
		std::vector<std::wstring> versionNumbers;
		versionNumbers.reserve(4);
		split(versionNumbers, version, boost::is_any_of(L"."));
		BOOST_ASSERT(versionNumbers.size() == 4);
		if (versionNumbers.size() != 4) {
			return false;
		}
		UpdateChecker::Version parsed;
		try {
			parsed.majorHigh = boost::lexical_cast<unsigned long>(versionNumbers[0]);
			parsed.majorLow = boost::lexical_cast<unsigned long>(versionNumbers[1]);
			parsed.minorHigh = boost::lexical_cast<unsigned long>(versionNumbers[2]);
			parsed.minorLow = boost::lexical_cast<unsigned long>(versionNumbers[3]);
		} catch (const boost::bad_lexical_cast &) {
			BOOST_ASSERT(false);
			return false;
		}
		result = parsed;
		return true;
	}

	bool GetLastReleaseInfo(const wchar_t *const url, std::wstring &version) const {
		
		struct CoInitializer : private boost::noncopyable {
			CoInitializer() {
				CoInitialize(NULL);	
			}
			~CoInitializer() {
				CoUninitialize();
			}
		} coInitializer;
		
		try {
	
			CComPtr<IXMLDOMDocument> doc;

			VARIANT_BOOL loadResult;
			if (doc.CoCreateInstance(_uuidof(DOMDocument)) != S_OK
					|| doc->put_async(VARIANT_TRUE) != S_OK
					|| doc->load(CComVariant(url), &loadResult) != S_OK
					|| loadResult != VARIANT_TRUE) {
				BOOST_ASSERT(false);
				return false;
			}

			for ( ; !m_isShutdownMode; ) {
				long state;
				if (doc->get_readyState(&state) != S_OK) {
					BOOST_ASSERT(false);
					return false;
				} else if (state != READYSTATE_COMPLETE) {
					MSG msg;
					if (PeekMessage(&msg, 0, 0 ,0, PM_REMOVE)) {
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					wxMilliSleep(500);
				} else {
					CComPtr<IXMLDOMParseError> parseError;
					long parseErrorCode;
					if (doc->get_parseError(&parseError) != S_OK) {
						BOOST_ASSERT(false);
						return false;
					} else if (parseError->get_errorCode(&parseErrorCode) != S_FALSE) {
						return false;
					}
					std::wstring resultVersion;
					const bool result = ParseVersion(doc, resultVersion);
					if (result) {
						swap(resultVersion, version);
					}
					return result;
				}
			}

		} catch (...) {
			BOOST_ASSERT(false);
		}
		
		return false;
	
	}

	bool ParseVersion(CComPtr<IXMLDOMDocument> doc, std::wstring &version) const {
		version.clear();
		CComPtr<IXMLDOMNode> node;
		if (doc->selectSingleNode(_bstr_t("VersionInfo/Release/@Version"), &node) == S_OK) {
			_variant_t str;
			if (node->get_nodeTypedValue(&str) >= 0) {
				version = str.bstrVal;
				return true;
			}
		}
		return false;
	}

};

//////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(EVT_UPDATE_CHECK_ACTION);

UpdateChecker::Event::Event(wxEventType type, Id id)
		: wxNotifyEvent(type, id) {
	//...//
}

UpdateChecker::Event::~Event() {
	//...//
}

wxEvent * UpdateChecker::Event::Clone() const {
	return new Event(*this);
}

//////////////////////////////////////////////////////////////////////////

UpdateChecker::UpdateChecker(wxWindow *parent, const Stat &stat)
		: m_thread(new Thread(parent, stat)),
		m_isActive(true) {
	m_thread->Create();
}

UpdateChecker::~UpdateChecker() {
	if (m_isActive) {
		m_isActive = false;
		m_thread->Delete();
	}
	delete m_thread;
}

void UpdateChecker::WaitCheck() const {
	if (!m_isActive) {
		return;
	}
	const_cast<UpdateChecker *>(this)->m_isActive = false;
	const_cast<UpdateChecker *>(this)->m_thread->Wait();
}

void UpdateChecker::RunCheck() {
	BOOST_ASSERT(!m_thread->IsRunning());
	BOOST_ASSERT(m_isActive);
	m_thread->Run();
}

bool UpdateChecker::IsReleaseNew() const {
	WaitCheck();
	return m_thread->IsReleaseNew();
}

const UpdateChecker::Version & UpdateChecker::GetCurrentVersion() const {
	WaitCheck();
	return m_thread->GetReleaseVersion();
}
