/**************************************************************************
 *   Created: 2008/01/04 3:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: ServiceStateCtrl.cpp 1003 2010-09-24 11:43:11Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "ServiceStateCtrl.hpp"
#include "ServiceState.hpp"
#include "ServiceWindow.hpp"
#include "ServiceAdapter.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

TAG_HANDLER_BEGIN(ServiceStateDescription, "DESCRIPTION")
TAG_HANDLER_PROC(tag) {

	tag;

	ServiceStateCtrl &parentCtrl = *polymorphic_downcast<ServiceStateCtrl *>(
		m_WParser->GetWindowInterface()->GetHTMLWindow());

	wxStaticText *const descriptionCtrl
		= new wxStaticText(&parentCtrl, wxID_ANY, parentCtrl.GetStateDecription());
	
	descriptionCtrl->Wrap(-1);

	if (!descriptionCtrl->SetForegroundColour(m_WParser->GetActualColor())) {
		BOOST_ASSERT(false);
	}
	if (!m_WParser->GetFontFace().IsEmpty()) {
		wxFont font;
		if (	!font.SetFaceName(m_WParser->GetFontFace())
			||	!descriptionCtrl->SetFont(font)) {
			BOOST_ASSERT(false);
		}
	}
	for (	wxHtmlContainerCell* container = m_WParser->GetContainer()
		;	container
		;	container = container->GetParent()) {
		const wxColor color = container->GetBackgroundColour();
		if (color != wxNullColour) {
			if (!descriptionCtrl->SetBackgroundColour(color)) {
				BOOST_ASSERT(false);
			}
			break;
		}
	}

	m_WParser->GetContainer()->InsertCell(new wxHtmlWidgetCell(descriptionCtrl, 100));

	return false;

}
TAG_HANDLER_END(ServiceStateDescription)

TAG_HANDLER_BEGIN(ServiceStateChoosing, "STATE")
TAG_HANDLER_PROC(tag) {

	BOOST_ASSERT(tag.HasParam(wxT("NAME")));
	if (!tag.HasParam(wxT("NAME"))) {
		return false;
	}
	const wxString passedStateName(tag.GetParam(wxT("NAME")));

	ServiceStateCtrl &ctrl = *polymorphic_downcast<ServiceStateCtrl *>(
		m_WParser->GetWindowInterface()->GetHTMLWindow());
	wxString currentStateName;
	switch (ctrl.GetStateCode()) {
		case TEX_SERVICE_STATE_CONNECTING:
		case TEX_SERVICE_STATE_UNKNOWN:
			currentStateName = wxT("unknown");
			break;
		case TEX_SERVICE_STATE_STARTED:
			currentStateName = wxT("started");
			break;
		case TEX_SERVICE_STATE_STOPPED:
			currentStateName = wxT("stopped");
			break;
		case TEX_SERVICE_STATE_ERROR:
			currentStateName = wxT("error");
			break;
		case TEX_SERVICE_STATE_WARNING:
			currentStateName = wxT("warning");
			break;
		case TEX_SERVICE_STATE_CHANGED:
			currentStateName = wxT("changed");
			break;
		default:
			BOOST_ASSERT(false);
	}
	if (!currentStateName.IsEmpty() && currentStateName == passedStateName) {
		ParseInner(tag);
	}

	return true;

}
TAG_HANDLER_END(ServiceStateChoosing)

TAGS_MODULE_BEGIN(ServiceState)
    TAGS_MODULE_ADD(ServiceStateChoosing)
	TAGS_MODULE_ADD(ServiceStateDescription)
TAGS_MODULE_END(ServiceState)

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ServiceStateCtrl, wxHtmlWindow)
	EVT_HTML_LINK_CLICKED(wxID_ANY,  ServiceStateCtrl::OnLinkClicked)
END_EVENT_TABLE()

ServiceStateCtrl::ServiceStateCtrl(
			wxWindow *parent,
			wxWindowID id,
			const wxPoint &pos,
			const wxSize &size,
			const wxString &name)
		: wxHtmlWindow(parent, id, pos, size, wxHW_NO_SELECTION | wxHW_SCROLLBAR_NEVER | wxBORDER_NONE, name),
		m_state(TEX_SERVICE_STATE_UNKNOWN),
		m_description(),
		m_linkClickHandlers() {
	SetBorders(0);
	SetState(TEX_SERVICE_STATE_UNKNOWN, wxEmptyString);
}

ServiceStateCtrl::~ServiceStateCtrl() {
	//...//
}

void ServiceStateCtrl::SetState(ServiceState state, const wxString& description) {
	m_state = state;
	m_description = description;
	wpath pageFile = Helpers::GetModuleFilePath().branch_path();
	pageFile /= L"Templates";
	pageFile /= L"ServiceState.html";
	wxWindowUpdateLocker noUpdates(this);
	LoadPage(pageFile.string().c_str());
}

void ServiceStateCtrl::StartTexService() {
	polymorphic_downcast<ServiceWindow *>(GetParent())->GetService().Start();
}

void ServiceStateCtrl::StopTexService() {
	polymorphic_downcast<ServiceWindow *>(GetParent())->GetService().Stop();
}

void ServiceStateCtrl::FillLinkClickHandlersMap(LinkClickHandlersMap &map) {
	ServiceWindow &wnd = *polymorphic_downcast<ServiceWindow *>(GetParent());
	map.insert(make_pair<wxString, LinkClickHandlerFunc>(wxT("serviceStart"), boost::bind(&ServiceStateCtrl::StartTexService, this)));
	map.insert(make_pair<wxString, LinkClickHandlerFunc>(wxT("serviceStop"), boost::bind(&ServiceStateCtrl::StopTexService, this)));
	map.insert(make_pair<wxString, LinkClickHandlerFunc>(wxT("serviceLogShow"), boost::bind(&ServiceWindow::OpenServiceLog, &wnd)));
	map.insert(make_pair<wxString, LinkClickHandlerFunc>(wxT("applyChanges"), boost::bind(&ServiceWindow::ApplyChanges, &wnd)));
	map.insert(make_pair<wxString, LinkClickHandlerFunc>(wxT("cancelChanges"), boost::bind(&ServiceWindow::CancelChanges, &wnd)));
	map.insert(make_pair<wxString, LinkClickHandlerFunc>(wxT("clearState"), boost::bind(&ServiceWindow::ClearState, &wnd)));
}

void ServiceStateCtrl::OnLinkClicked(wxHtmlLinkEvent &linkEvent) {
	if (!m_linkClickHandlers.size()) {
		FillLinkClickHandlersMap(m_linkClickHandlers);
	}
	const basic_string<wxChar> href = linkEvent.GetLinkInfo().GetHref();
	typedef vector<basic_string<wxChar> > Cmds;
	Cmds cmds;
	split(cmds, href, is_any_of(wxT("_")));
	const Cmds::const_iterator cmdEnd = cmds.end();
	const LinkClickHandlersMap::const_iterator handlersEnd = m_linkClickHandlers.end();
	for (Cmds::const_iterator i = cmds.begin(); i != cmdEnd; ++i) {
		const LinkClickHandlersMap::const_iterator pos = m_linkClickHandlers.find(*i);
		if (pos != handlersEnd) {
			pos->second();
		}
	}
}
