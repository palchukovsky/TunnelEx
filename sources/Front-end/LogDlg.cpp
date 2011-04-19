/**************************************************************************
 *   Created: 2008/01/08 3:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "LogDlg.hpp"
#include "MainFrame.hpp"
#include "ServiceWindow.hpp"
#include "Application.hpp"

#include "Core/Log.hpp"

using namespace TunnelEx;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

enum LogDlg::Column {
	COLUMN_LEVEL,
	COLUMN_TIME,
	COLUMN_MESSAGE,
	COLUMNS_NUMBER
};

enum LogDlg::Command {
	COMMAND_COPY_SELECTED = wxID_HIGHEST,
	COMMAND_COPY_ALL,
	COMMAND_SAVE_SELECTED,
	COMMAND_SAVE_ALL
};

enum LogDlg::Control {
	CONTROL_LIST = wxID_HIGHEST,
	CONTROL_COPY,
	CONTROL_SAVE,
	CONTROL_LOGLEVEL_LABEL,
	CONTROL_LOGLEVEL,
	CONTROLS_NUMBER
};

BEGIN_EVENT_TABLE(LogDlg, wxDialog)
	EVT_BUTTON(LogDlg::CONTROL_COPY, LogDlg::OnCopyButton)
	EVT_BUTTON(LogDlg::CONTROL_SAVE, LogDlg::OnSaveButton)
	EVT_MENU(LogDlg::COMMAND_COPY_ALL, LogDlg::OnCopyAll)
	EVT_MENU(LogDlg::COMMAND_COPY_SELECTED, LogDlg::OnCopySelected)
	EVT_MENU(LogDlg::COMMAND_SAVE_ALL, LogDlg::OnSaveAll)
	EVT_MENU(LogDlg::COMMAND_SAVE_SELECTED, LogDlg::OnSaveSelected)
	EVT_BUTTON(wxID_CANCEL, LogDlg::OnCancel)
	EVT_CLOSE(LogDlg::OnClose)
	EVT_SIZE(LogDlg::OnSize)
	EVT_CHAR_HOOK(LogDlg::OnKeyDown)
	EVT_SERVICE_ADAPTER(
		ServiceAdapter::Event::ID_NEW_LOG_RECORD,
		LogDlg::OnServiceNewLogRecord)
	EVT_CONTEXT_MENU(LogDlg::OnItemContextMenu)
	EVT_CHOICE(LogDlg::CONTROL_LOGLEVEL, LogDlg::OnLogLevelChange)
	EVT_LIST_ITEM_ACTIVATED(wxID_ANY, LogDlg::OnRecordActivated)
END_EVENT_TABLE()

LogDlg::LogDlg(wxWindow *parent, wxWindowID id)
		: wxDialog(
			parent,
			id,
			TUNNELEX_NAME_W wxT(" service log"),
			wxDefaultPosition,
			wxSize(650, 400),
			wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCLOSE_BOX | wxMAXIMIZE_BOX),
		m_buttonSize(75, 25),
		m_borderWidth(10),
		m_logSize(
			boost::polymorphic_downcast<ServiceWindow *>(GetParent())
				->GetService()
				.GetLogSize()),
		m_logLevel(::LOG_LEVEL_INFO) {

	SetMinSize(wxSize(500, 300));

	{
		wxListCtrl *listCtrl = new wxListCtrl(
			this,
			CONTROL_LIST,
			GetListPosition(), GetListSize(),
			wxLC_REPORT | wxLC_NO_HEADER
				| wxLC_NO_SORT_HEADER | wxLC_HRULES | wxLC_VRULES);
		wxListItem itemCol;
		itemCol.SetImage(-1);
		itemCol.SetAlign(wxLIST_FORMAT_RIGHT);
		listCtrl->InsertColumn(COLUMN_LEVEL, itemCol);
		listCtrl->InsertColumn(COLUMN_TIME, itemCol);
		itemCol.SetAlign(wxLIST_FORMAT_LEFT);
		itemCol.SetWidth(wxLIST_AUTOSIZE);
		listCtrl->InsertColumn(COLUMN_MESSAGE, itemCol);
	}

	new wxButton(
		this, CONTROL_COPY, wxT("Copy"),
		GetCopyButtonPosition(), GetCopyButtonSize());

	new wxButton(
		this, CONTROL_SAVE, wxT("Save"),
		GetSaveButtonPosition(), GetSaveButtonSize());

	{
		wxStaticText &label = *new wxStaticText(
			this,
			CONTROL_LOGLEVEL_LABEL,
			wxT("Loggin level:"));
		label.Fit();
		label.SetPosition(GetLogLevelLabelPosition());
		wxChoice &logLevel = *new wxChoice(this, CONTROL_LOGLEVEL);
		logLevel.Append(wxT("debugging"));
		logLevel.Append(wxT("full"));
		logLevel.Append(wxT("warnings and errors"));
		logLevel.Append(wxT("only errors"));
		logLevel.Fit();
		logLevel.SetPosition(GetLogLevelPosition());
		try {
			SetLogLevel(
				boost::polymorphic_downcast<ServiceWindow *>(GetParent())
					->GetService()
					.GetLogLevel(),
				true);
		} catch (const TunnelEx::LocalException &ex) {
			::wxLogError(ex.GetWhat());
		}
	}

	new wxButton(
		this, wxID_CANCEL, wxT("Close"),
		GetCloseButtonPosition(), GetCloseButtonSize());

	{
		Config &config = wxGetApp().GetConfig();
		wxPoint pos;
		config.Read(wxT("/Log/Window/Position/X"), &pos.x, 0);
		config.Read(wxT("/Log/Window/Position/Y"), &pos.y, 0);
		if (pos.x && pos.y) {
			SetPosition(pos);
		}
		wxSize size;
		config.Read(wxT("/Log/Window/Size/Width"), &size.x, 0);
		config.Read(wxT("/Log/Window/Size/Height"), &size.y, 0);
		if (size.x && size.y) {
			SetSize(size);
		}

	}

	SetLogLevel(::LOG_LEVEL_INFO);
	RefreshData();

}

LogDlg::~LogDlg() {
	if (!IsMaximized() && !IsIconized()) {
		Config& config = wxGetApp().GetConfig();
		const wxPoint pos(GetPosition());
		config.Write(wxT("/Log/Window/Position/X"), pos.x);
		config.Write(wxT("/Log/Window/Position/Y"), pos.y);
		const wxSize size(GetSize());
		config.Write(wxT("/Log/Window/Size/Width"), size.x);
		config.Write(wxT("/Log/Window/Size/Height"), size.y);
	}
}

void LogDlg::RefreshData() {

	wxListCtrl &listCtrl
		= *boost::polymorphic_downcast<wxListCtrl *>(FindWindow(CONTROL_LIST));

	listCtrl.DeleteAllItems();

	std::list<texs__LogRecord> log;
	texs__LogLevel logLevel = m_logLevel;
	boost::polymorphic_downcast<ServiceWindow *>(GetParent())
		->GetService()
		.GetLogRecords(100, log);
	//! @todo: reimplement, by service configure update event [2010/04/03 23:06]
	logLevel = boost::polymorphic_downcast<ServiceWindow *>(GetParent())
		->GetService()
		.GetLogLevel();
	SetLogLevel(logLevel);

	const std::list<texs__LogRecord>::const_iterator logEnd = log.end();
	for (std::list<texs__LogRecord>::const_iterator i = log.begin(); i != logEnd; ++i) {
		AppendRecord(*i);
	}

	ShowLastRecord();

}

void LogDlg::AddData(unsigned int recordsCount) {
	std::list<texs__LogRecord> log;
	boost::polymorphic_downcast<ServiceWindow *>(GetParent())
		->GetService()
		.GetLogRecords(recordsCount, log);
	const std::list<texs__LogRecord>::const_iterator logEnd = log.end();
	for (std::list<texs__LogRecord>::const_iterator i = log.begin(); i != logEnd; ++i) {
		AppendRecord(*i);
	}
	ShowLastRecord();
}

void LogDlg::AppendRecord(const texs__LogRecord &record) {

	switch (record.level) {
		case 0: // LOG_LEVEL_TRACK in release
		case TunnelEx::LOG_LEVEL_TRACK:
		case TunnelEx::LOG_LEVEL_DEBUG:
			if (m_logLevel > ::LOG_LEVEL_DEBUG) {
				return;
			}
			break;
		case TunnelEx::LOG_LEVEL_INFO:
			if (m_logLevel > ::LOG_LEVEL_INFO) {
				return;
			}
			break;
		case TunnelEx::LOG_LEVEL_WARN:
			if (m_logLevel > ::LOG_LEVEL_WARN) {
				return;
			}
			break;
		case TunnelEx::LOG_LEVEL_ERROR:
		case TunnelEx::LOG_LEVEL_SYSTEM_ERROR:
		case TunnelEx::LOG_LEVEL_FATAL_ERROR:
			break;
		default:
			BOOST_ASSERT(false);
	}

	wxListCtrl &listCtrl
		= *boost::polymorphic_downcast<wxListCtrl *>(FindWindow(CONTROL_LIST));
	wxListItem item;
	item.SetId(listCtrl.GetItemCount());
	wxFont itemFont(item.GetFont());
	itemFont.SetFamily(wxFONTFAMILY_SWISS);
	switch (record.level) {
		case 0: // LOG_LEVEL_TRACK in release
		case TunnelEx::LOG_LEVEL_TRACK:
		case TunnelEx::LOG_LEVEL_DEBUG:
			item.SetTextColour(wxColor(wxT("RGB(150,150,150)")));
			item.SetText(wxT("Debugging"));
			break;
		case TunnelEx::LOG_LEVEL_INFO:
			item.SetText(wxT("Information"));
			break;
		case TunnelEx::LOG_LEVEL_WARN:
			itemFont.SetWeight(wxFONTWEIGHT_BOLD);
			item.SetBackgroundColour(wxColor(wxT("RGB(255,255,153)")));
			item.SetTextColour(wxColor(wxT("RGB(255,120,0)")));
			item.SetText(wxT("Warning"));
			break;
		case TunnelEx::LOG_LEVEL_ERROR:
		case TunnelEx::LOG_LEVEL_SYSTEM_ERROR:
		case TunnelEx::LOG_LEVEL_FATAL_ERROR:
			itemFont.SetWeight(wxFONTWEIGHT_BOLD);
			item.SetBackgroundColour(wxColor(wxT("RGB(255,200,200)")));
			item.SetTextColour(wxColor(wxT("RGB(204,0,0)")));
			item.SetText(wxT("Error"));
			break;
		default:
			BOOST_ASSERT(false);
			break;
	}
	item.SetFont(itemFont);
	item.SetData(record.level);
	const long index = listCtrl.InsertItem(item);
	tm tm(pt::to_tm(pt::time_from_string(record.time)));
	listCtrl.SetItem(
		index,
		COLUMN_TIME,
		(WFormat(L"%|02|.%|02|.%|4| %|02|:%|02|:%|02|")
			% tm.tm_mday % (tm.tm_mon + 1) % (tm.tm_year + 1900)
			% tm.tm_hour % tm.tm_min % tm.tm_sec).str().c_str());
	listCtrl.SetItem(
		index,
		COLUMN_MESSAGE,
		ConvertString<WString>(record.message.c_str()).GetCStr());
	
	listCtrl.SetColumnWidth(COLUMN_LEVEL, wxLIST_AUTOSIZE);
	listCtrl.SetColumnWidth(COLUMN_TIME, wxLIST_AUTOSIZE);
	listCtrl.SetColumnWidth(COLUMN_MESSAGE, wxLIST_AUTOSIZE);

}

void LogDlg::OnClose(wxCloseEvent &) {
	OnClose();
}

void LogDlg::OnCancel(wxCommandEvent &event) {
	OnClose();
	event.Skip();
}

void LogDlg::OnClose() {
	Event event(EVT_LOG_DLG_ACTION, LogDlg::Event::ID_CLOSE);
	wxPostEvent(GetParent(), event);
}

void LogDlg::OnSize(wxSizeEvent &event) {
	wxListCtrl &list = *boost::polymorphic_downcast<wxListCtrl *>(FindWindow(CONTROL_LIST));
	list.SetPosition(GetListPosition());
	list.SetSize(GetListSize());
	wxButton &copyToClipboardButton = *boost::polymorphic_downcast<wxButton *>(FindWindow(CONTROL_COPY));
	copyToClipboardButton.SetPosition(GetCopyButtonPosition());
	wxButton &saveToFileButton = *boost::polymorphic_downcast<wxButton *>(FindWindow(CONTROL_SAVE));
	saveToFileButton.SetPosition(GetSaveButtonPosition());
	boost::polymorphic_downcast<wxStaticText *>(FindWindow(CONTROL_LOGLEVEL_LABEL))
		->SetPosition(GetLogLevelLabelPosition());
	boost::polymorphic_downcast<wxChoice *>(FindWindow(CONTROL_LOGLEVEL))
		->SetPosition(GetLogLevelPosition());
	wxButton &closeButton = *boost::polymorphic_downcast<wxButton *>(FindWindow(wxID_CANCEL));
	closeButton.SetPosition(GetCloseButtonPosition());
	event.Skip();
}

void LogDlg::DoContextMenu(bool save, bool copy) {
	
	BOOST_ASSERT(save || copy);
	
	wxListCtrl &list = *boost::polymorphic_downcast<wxListCtrl *>(FindWindow(CONTROL_LIST));
	wxMenu menu;
	
	if (copy) {
		
		wxMenuItem *item = new wxMenuItem(&menu, COMMAND_COPY_SELECTED, wxT("Copy Selected"));
		item->Enable(list.GetSelectedItemCount() > 0);
		menu.Append(item);
		
		menu.Append(new wxMenuItem(&menu, COMMAND_COPY_ALL, wxT("Copy All")));
	
	}
	
	if (save) {
	
		wxMenuItem *item = new wxMenuItem(&menu, COMMAND_SAVE_SELECTED, wxT("Save Selected"));
		item->Enable(list.GetSelectedItemCount() > 0);
		menu.Append(item);

		menu.Append(new wxMenuItem(&menu, COMMAND_SAVE_ALL, wxT("Save All")));

	}
	
	PopupMenu(&menu,wxDefaultPosition);
	
}

void LogDlg::OnCopyButton(wxCommandEvent &) {
	DoContextMenu(false, true);
}

void LogDlg::Copy(bool onlySelected) {

	if (!wxTheClipboard->Open()) {
		BOOST_ASSERT(false);
		return;
	};

	struct AutoCloseClipboard {
		~AutoCloseClipboard() {
			wxTheClipboard->Close();
		}
	} autoCloseClipboard;
	wxTheClipboard->SetData(new wxTextDataObject(GetListContentAsText(onlySelected)));

}

wxString LogDlg::GetListContentAsText(bool onlySelected) const {
	
	const wxListCtrl &list = *boost::polymorphic_downcast<wxListCtrl *>(FindWindow(CONTROL_LIST));

	std::wostringstream textDataStream;
	wxListItem item;
	item.SetMask(wxLIST_MASK_TEXT);
	for (long itemId = -1; ; ) {
		
		itemId = list.GetNextItem(
			itemId,
			wxLIST_NEXT_ALL,
			onlySelected ? wxLIST_STATE_SELECTED : wxLIST_STATE_DONTCARE);
		if (itemId == -1) {
			break;
		}
		
		item.SetId(itemId);

		item.SetColumn(COLUMN_TIME);
		list.GetItem(item);
		textDataStream << item.GetText().c_str() << L' ';

		item.SetColumn(COLUMN_LEVEL);
		list.GetItem(item);
		textDataStream << std::setw(12) << item.GetText().c_str() << L": ";

		item.SetColumn(COLUMN_MESSAGE);
		list.GetItem(item);
		textDataStream << item.GetText().c_str();
		
		textDataStream << L"\r\n";

	}

	return textDataStream.str();

}

void LogDlg::OnSaveButton(wxCommandEvent &) {
	DoContextMenu(true, false);
}

void LogDlg::OnCopySelected(wxCommandEvent &) {
	Copy(true);
}

void LogDlg::OnCopyAll(wxCommandEvent &) {
	Copy(false);
}

void LogDlg::OnSaveSelected(wxCommandEvent &) {
	Save(true);
}

void LogDlg::OnSaveAll(wxCommandEvent &) {
	Save(false);
}

void LogDlg::Save(bool onlySelected) {
	wxFileDialog saveToDlg(
		this, wxT("Choose a file"),
		wxEmptyString, wxEmptyString,
		wxT("All files (*.*)|*.*|Text files (*.txt)|*.txt|Log files (*.log)|*.log;"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveToDlg.ShowModal() == wxID_OK) {
		wxFile file;
		if (file.Create(saveToDlg.GetPath(), true)) {
			file.Write(GetListContentAsText(onlySelected));
		} else {
			wxLogError(wxT("Could not create file \"") + saveToDlg.GetPath() + wxT("\"."));
		}
	}
}

wxPoint LogDlg::GetCopyButtonPosition() const {
	const wxSize clientSize(GetClientSize());
	return wxPoint(
		m_borderWidth,
		clientSize.y - m_borderWidth - GetCopyButtonSize().y);
}

wxPoint LogDlg::GetSaveButtonPosition() const {
	const wxPoint copyButtonPos(GetCopyButtonPosition());
	return wxPoint(
		copyButtonPos.x + GetSaveButtonSize().x + m_borderWidth,
		copyButtonPos.y);
}

wxPoint LogDlg::GetLogLevelPosition() const {
	wxStaticText &logLevelLabel
		= *boost::polymorphic_downcast<wxStaticText *>(FindWindow(CONTROL_LOGLEVEL_LABEL));
	wxChoice &logLevel
		= *boost::polymorphic_downcast<wxChoice *>(FindWindow(CONTROL_LOGLEVEL));
	return wxPoint(
		logLevelLabel.GetSize().GetWidth() + logLevelLabel.GetPosition().x,
		GetSaveButtonPosition().y
			+ ((GetSaveButtonSize().GetHeight() - logLevel.GetSize().GetHeight()) / 2));
}

wxPoint LogDlg::GetLogLevelLabelPosition() const {
	wxStaticText &logLevelLabel
		= *boost::polymorphic_downcast<wxStaticText *>(FindWindow(CONTROL_LOGLEVEL_LABEL));
	const wxPoint saveButtonPosition = GetSaveButtonPosition();
	return wxPoint(
		saveButtonPosition.x + m_borderWidth + GetSaveButtonSize().GetWidth(),
		saveButtonPosition.y
			+ ((GetSaveButtonSize().GetHeight() - logLevelLabel.GetSize().GetHeight()) / 2));
}

wxPoint LogDlg::GetCloseButtonPosition() const {
	const wxSize clientSize(GetClientSize());
	return wxPoint(
		clientSize.x - m_borderWidth - m_buttonSize.x,
		clientSize.y - m_borderWidth - m_buttonSize.y);
}

wxSize LogDlg::GetListSize() const {
	wxSize result(GetClientSize());
	result.x -= m_borderWidth * 2;
	result.y -= m_borderWidth * 3;
	result.y -= m_buttonSize.y;
	return result;
}

void LogDlg::ShowLastRecord() {
	wxListCtrl &list
		= *boost::polymorphic_downcast<wxListCtrl *>(FindWindow(CONTROL_LIST));
	int pxOffset = 0;
	const int itemCount = list.GetItemCount();
	BOOST_ASSERT(itemCount >= list.GetTopItem());
	for (int i = list.GetTopItem(); i < itemCount; ++i) {
		wxRect rect;
		if (list.GetItemRect(i, rect)) {
			pxOffset += rect.GetHeight();
		}
	}
	list.ScrollList(0, pxOffset);
}

void LogDlg::OnKeyDown(wxKeyEvent &event) {
	switch (event.GetKeyCode()) {
		case WXK_ESCAPE:
		case WXK_F2:
			Close();
			break;
		default:
			event.Skip();
	}
}

void LogDlg::OnServiceNewLogRecord(ServiceAdapter::Event &) {
	const unsigned long long prevLogSize = m_logSize;
	m_logSize
		= boost::polymorphic_downcast<ServiceWindow *>(GetParent())->GetService().GetLogSize();
	if (prevLogSize >= m_logSize) {
		RefreshData();
	} else {
		AddData(m_logSize - prevLogSize);
	}
}

void LogDlg::OnItemContextMenu(wxContextMenuEvent &) {
	DoContextMenu(true, true);
}

void LogDlg::SetLogLevel(texs__LogLevel logLevel, bool force /*= false*/) {
	if (m_logLevel == logLevel && !force) {
		return;
	}
	wxString str;
	switch (logLevel) {
		case ::LOG_LEVEL_DEBUG:
			str = wxT("debugging");
			break;
		case ::LOG_LEVEL_WARN:
			str = wxT("warnings and errors");
			break;
		case ::LOG_LEVEL_ERROR:
			str = wxT("only errors");
			break;
		default:
			BOOST_ASSERT(false);
		case ::LOG_LEVEL_INFO:
			str = wxT("full");
	}
	wxChoice &choise
		= *boost::polymorphic_downcast<wxChoice *>(FindWindow(CONTROL_LOGLEVEL));
	choise.Select(choise.FindString(str));
	m_logLevel = logLevel;
}

void LogDlg::OnLogLevelChange(wxCommandEvent &) {
	const wxString str
		= boost::polymorphic_downcast<wxChoice *>(FindWindow(CONTROL_LOGLEVEL))
			->GetStringSelection();
	texs__LogLevel level = ::LOG_LEVEL_INFO;
	if (str == wxT("debugging")) {
		wxMessageBox(
			wxT("Debugging information can increase the use of system resources.")
				wxT(" Use it if you really need this information.")
				wxT(" The logging level will be set to \"full\"")
				wxT(" at the next system startup."));
		level = ::LOG_LEVEL_DEBUG;
	} else if (str == wxT("warnings and errors")) {
		level = ::LOG_LEVEL_WARN;
	} else if (str == wxT("only errors")) {
		level = ::LOG_LEVEL_ERROR;
	} else if (str == wxT("full")) {
		level = ::LOG_LEVEL_INFO;
	} else {
		BOOST_ASSERT(false);
	}
	m_logLevel = level;
	boost::polymorphic_downcast<ServiceWindow *>(GetParent())
		->GetService()
		.SetLogLevel(level);
	RefreshData();
}

void LogDlg::OnRecordActivated(wxListEvent &) {

	wxString time;
	wxString level;
	wxString text;
	unsigned int levelCode = 0;
	{
		const wxListCtrl &list
			= *boost::polymorphic_downcast<wxListCtrl *>(FindWindow(CONTROL_LIST));
		wxListItem item;
		item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA);
		const long itemId = list.GetNextItem(
			-1,
			wxLIST_NEXT_ALL,
			wxLIST_STATE_SELECTED);
		BOOST_ASSERT(itemId != -1);
		if (itemId == -1) {
			return;
		}
		item.SetId(itemId);
		item.SetColumn(COLUMN_TIME);
		list.GetItem(item);
		time = item.GetText();
		item.SetColumn(COLUMN_LEVEL);
		list.GetItem(item);
		level = item.GetText();
		item.SetColumn(COLUMN_MESSAGE);
		list.GetItem(item);
		text = item.GetText();
		levelCode = item.GetData();
	}

	const Theme &theme = wxGetApp().GetTheme();

	wxDialog dlg(
		this,
		wxID_ANY,
		level,
		wxDefaultPosition,
		wxDefaultSize,
		wxCAPTION);
	
	const wxSizerFlags flags
		= wxSizerFlags(0)
			.Expand()
			.Border(wxLEFT | wxRIGHT, theme.GetDlgBorder() / 2);

	std::auto_ptr<wxBoxSizer> box(new wxBoxSizer(wxVERTICAL));

	{
		std::auto_ptr<wxBoxSizer> timeBox(new wxBoxSizer(wxHORIZONTAL));
		timeBox->Add(
			new wxStaticText(&dlg, wxID_ANY, wxT("Time:")),
			wxSizerFlags(0).Center());
		wxTextCtrl &ctrl = *new wxTextCtrl(&dlg, wxID_ANY, time);
		ctrl.SetEditable(false);
		timeBox->Add(&ctrl, wxSizerFlags(1).Center());
		box->Add(timeBox.get(), flags);
		timeBox.release();
		box->Add(0, theme.GetDlgBorder() / 2);
	}
	
	{
		wxTextCtrl &ctrl = *new wxTextCtrl(
			&dlg,
			wxID_ANY,
			text,
			wxDefaultPosition,
			wxDefaultSize,
			wxTE_MULTILINE);
		ctrl.SetEditable(false);
		ctrl.SetMinSize(wxSize(350, 150));
		wxFont font(ctrl.GetFont());
		switch (levelCode) {
			case 0: // LOG_LEVEL_TRACK in release
			case TunnelEx::LOG_LEVEL_TRACK:
			case TunnelEx::LOG_LEVEL_DEBUG:
				ctrl.SetForegroundColour(wxColor(wxT("RGB(150,150,150)")));
				break;
			case TunnelEx::LOG_LEVEL_INFO:
				break;
			case TunnelEx::LOG_LEVEL_WARN:
				font.SetWeight(wxFONTWEIGHT_BOLD);
				ctrl.SetBackgroundColour(wxColor(wxT("RGB(255,255,153)")));
				ctrl.SetForegroundColour(wxColor(wxT("RGB(255,120,0)")));
				break;
			case TunnelEx::LOG_LEVEL_ERROR:
			case TunnelEx::LOG_LEVEL_SYSTEM_ERROR:
			case TunnelEx::LOG_LEVEL_FATAL_ERROR:
				font.SetWeight(wxFONTWEIGHT_BOLD);
				ctrl.SetBackgroundColour(wxColor(wxT("RGB(255,200,200)")));
				ctrl.SetForegroundColour(wxColor(wxT("RGB(204,0,0)")));
				break;
			default:
				BOOST_ASSERT(false);
				break;
		}
		ctrl.SetFont(font);
		box->Add(&ctrl, wxSizerFlags(flags).Proportion(1));
		box->Add(0, theme.GetDlgBorder() / 2);
	}

	box->Add(dlg.CreateButtonSizer(wxOK), flags);
	box->Add(0, theme.GetDlgBorder() / 2);

	dlg.SetSizer(box.get());
	box.release()->SetSizeHints(&dlg);

	dlg.ShowModal();

}

//////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(EVT_LOG_DLG_ACTION);

LogDlg::Event::Event(wxEventType type, Id id)
		: wxNotifyEvent(type, id) {
	//...//
}

LogDlg::Event::~Event() {
	//...//
}

wxEvent * LogDlg::Event::Clone() const {
	return new Event(*this);
}

//////////////////////////////////////////////////////////////////////////
