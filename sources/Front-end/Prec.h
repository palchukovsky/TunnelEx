/**************************************************************************
 *   Created: 2010/06/21 21:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Prec.h 1072 2010-11-25 20:02:26Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Prec_h__1006212136
#define INCLUDED_FILE__TUNNELEX__Prec_h__1006212136

#include "CompileConfig.h"
#include "Constants.h"

#include "Licensing/Prec.h"
#include "Licensing/License.hpp"
#include "Licensing/LocalComunicationPolicy.hpp"

#include "Format.hpp"
#include "ModulePath.hpp"
#include "Foreach.h"
#include "StringUtil.hpp"

#include "SoapServiceInterface/soapStub.h"
#include "SoapServiceInterface/soapTunnelExServiceProxy.h"
#include "SoapServiceInterface/TunnelExService.nsmap"

#include <Windows.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <atlbase.h>
#include <comutil.h>

#include <wx/msw/winundef.h>
#include <wx/wxprec.h>
#include <wx/fileconf.h>
#include <wx/msw/registry.h>
#include <wx/listctrl.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/aboutdlg.h>
#include <wx/statline.h>
#include <wx/hyperlink.h>
#include <wx/clipbrd.h>
#include <wx/sstream.h>
#include <wx/html/m_templ.h>
#include <wx/wupdlock.h>
#include <wx/fontenum.h>
#include <wx/dialog.h>
#include <wx/html/htmlwin.h>
#include <wx/utils.h>
#include <wx/progdlg.h>

#include "CompileWarningsBoost.h"
#	include <boost/array.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/algorithm/string/regex.hpp>
#	include <boost/bind.hpp>
#	include <boost/cast.hpp>
#	include <boost/function.hpp>
#	include <boost/functional.hpp>
#	include <boost/noncopyable.hpp>
#	include <boost/optional.hpp>
#	include <boost/ptr_container/ptr_map.hpp>
#	include <boost/lexical_cast.hpp>
#	include <boost/regex.hpp>
#	include <boost/ref.hpp>
#	include <boost/shared_ptr.hpp>
#	include <boost/filesystem.hpp>
#	include <boost/cstdint.hpp>
#include "CompileWarningsBoost.h"

#include <numeric>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <sstream>
#include <bitset>

inline int CorrectBorderForButtonSizer(const int borderWidth) {
	return borderWidth - 4;
}

#endif
