/**************************************************************************
 *   Created: 2010/06/27 13:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: NewVersionDlg.hpp 1034 2010-10-15 21:38:05Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__NewVersionDlg_h__1006271336
#define INCLUDED_FILE__TUNNELEX__NewVersionDlg_h__1006271336

#include "UpdateChecker.hpp"


class NewVersionDlg : public wxDialog {

public:

	explicit NewVersionDlg(const UpdateChecker::Version &, wxWindow *parent);
	~NewVersionDlg();

	DECLARE_NO_COPY_CLASS(NewVersionDlg);

public:

	bool IsNewVersionCheckingOn() const;

private:

	wxCheckBox *m_doNotCheckCtrl;

}; 

#endif
