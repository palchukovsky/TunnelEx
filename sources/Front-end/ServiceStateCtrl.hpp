/**************************************************************************
 *   Created: 2008/01/04 3:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServiceStateCtrl_h__0801040300
#define INCLUDED_FILE__TUNNELEX__ServiceStateCtrl_h__0801040300

enum ServiceState;

class ServiceStateCtrl : public wxHtmlWindow  {

public:
	
	explicit ServiceStateCtrl(
			wxWindow *parent,
			wxWindowID id = wxID_ANY,
			const wxPoint &pos = wxDefaultPosition,
			const wxSize &size = wxDefaultSize,
			const wxString &name = wxT("ServiceState"));
	~ServiceStateCtrl();

	void SetState(ServiceState code, const wxString &description);
	ServiceState GetStateCode() const {
		return m_state;
	}
	const wxString & GetStateDecription() const {
		return m_description;
	}

	void OnLinkClicked(wxHtmlLinkEvent &);

	void StartTexService();
	void StopTexService();

private:

	typedef boost::function<void(void)> LinkClickHandlerFunc;
	typedef std::map<wxString, LinkClickHandlerFunc> LinkClickHandlersMap;
	void FillLinkClickHandlersMap(LinkClickHandlersMap&);

private:


	ServiceState m_state;
	wxString m_description;
	LinkClickHandlersMap m_linkClickHandlers;

	DECLARE_NO_COPY_CLASS(ServiceStateCtrl)
	DECLARE_EVENT_TABLE()

};

#endif // INCLUDED_FILE__TUNNELEX__ServiceStateCtrl_h__0801040300