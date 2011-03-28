/**************************************************************************
 *   Created: 2008/02/03 23:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: UpdateChecker.hpp 970 2010-06-27 13:47:52Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UpdateCheckThread_h__0802032329
#define INCLUDED_FILE__TUNNELEX__UpdateCheckThread_h__0802032329

class MainFrame;

class UpdateChecker : private boost::noncopyable {

public:

	struct Version {
		unsigned long majorHigh;
		unsigned long majorLow;
		unsigned long minorHigh;
		unsigned long minorLow;
	};
	
	struct Stat {
		std::wstring installationUuid;
		size_t rulesCount;
		std::wstring licenseKey;
		bool isTrialMode;
	};

	class Event : public wxNotifyEvent {
	public:
		enum Id {
			ID_NEW_RELEASE_CHECKED
		};
		typedef void (wxEvtHandler::*Handler)(UpdateChecker::Event &);
	public:
		explicit Event(wxEventType, Id);
		virtual ~Event();
	public:
		virtual wxEvent * Clone() const;
	};

public:

	explicit UpdateChecker(wxWindow *parent, const Stat &);
	~UpdateChecker() throw();

public:

	void RunCheck();

	void WaitCheck() const;

	bool IsReleaseNew() const;
	const Version & GetCurrentVersion() const;

private:

	class Thread;
	Thread *m_thread;
	bool m_isActive;

};

DECLARE_EVENT_TYPE(EVT_UPDATE_CHECK_ACTION, -1);

#define EVT_UPDATE_CHECK(id, fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
		EVT_UPDATE_CHECK_ACTION, \
		id, \
		-1, \
		(wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(UpdateChecker::Event::Handler, &fn), \
		(wxObject *)NULL),

#endif // INCLUDED_FILE__TUNNELEX__UpdateCheckThread_h__0802032329
