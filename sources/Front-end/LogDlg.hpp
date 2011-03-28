/**************************************************************************
 *   Created: 2008/01/08 3:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: LogDlg.hpp 1119 2010-12-30 23:18:09Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LogDlg_h__0801080332
#define INCLUDED_FILE__TUNNELEX__LogDlg_h__0801080332

#include "ServiceAdapter.hpp"

class texs__LogRecord;

class LogDlg : public wxDialog {

public:

	class Event : public wxNotifyEvent {
	public:
		enum Id {
			ID_CLOSE
		};
		typedef void (wxEvtHandler::*Handler)(LogDlg::Event &);
	public:
		explicit Event(wxEventType, Id);
		virtual ~Event();
	public:
		virtual wxEvent * Clone() const;
	};

public:
	
	LogDlg(wxWindow *parent, wxWindowID);
	~LogDlg();

	DECLARE_NO_COPY_CLASS(LogDlg);

	DECLARE_EVENT_TABLE();

public:

	void RefreshData();
	
	void AddData(unsigned int recordsCount);

	void OnCopyButton(wxCommandEvent &);
	void OnCopySelected(wxCommandEvent &);
	void OnCopyAll(wxCommandEvent &);

	void OnSaveButton(wxCommandEvent &);
	void OnSaveAll(wxCommandEvent &);
	void OnSaveSelected(wxCommandEvent &);

	void OnItemContextMenu(wxContextMenuEvent &);

	void OnKeyDown(wxKeyEvent &);

	void OnClose(wxCloseEvent &);
	void OnCancel(wxCommandEvent &);

	void OnSize(wxSizeEvent &);
	
	void OnServiceNewLogRecord(ServiceAdapter::Event &);
 
	wxString GetListContentAsText(bool onlySelected) const;

	void ShowLastRecord();

	void OnLogLevelChange(wxCommandEvent &);
	
	void OnRecordActivated(wxListEvent &);

protected:

	void AppendRecord(const texs__LogRecord &);

private:

	wxSize GetCloseButtonSize() const {
		return m_buttonSize;
	}
	wxPoint GetCloseButtonPosition() const;
	wxSize GetListSize() const;
	wxPoint GetListPosition() const {
		return wxPoint(m_borderWidth, m_borderWidth);
	}
	wxSize GetCopyButtonSize() const {
		return m_buttonSize;
	}
	wxPoint GetCopyButtonPosition() const;
	wxSize GetSaveButtonSize() const {
		return m_buttonSize;
	}
	wxPoint GetSaveButtonPosition() const;
	wxPoint GetLogLevelLabelPosition() const;
	wxPoint GetLogLevelPosition() const;

	void OnClose();

	void DoContextMenu(bool save, bool copy);

	void Copy(bool onlySelected);
	void Save(bool onlySelected);

	void SetLogLevel(texs__LogLevel, bool = false);

private:

	enum Control;
	enum Column;
	enum Command;

	const wxSize m_buttonSize;
	const unsigned char m_borderWidth;
	
	unsigned long long m_logSize;

	texs__LogLevel m_logLevel;

};

DECLARE_EVENT_TYPE(EVT_LOG_DLG_ACTION, -1);

#define EVT_LOG_DLG(id, fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
		EVT_LOG_DLG_ACTION, \
		id, \
		-1, \
		(wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(LogDlg::Event::Handler, &fn), \
		(wxObject *)NULL),

#endif // INCLUDED_FILE__TUNNELEX__LogDlg_h__0801080332
