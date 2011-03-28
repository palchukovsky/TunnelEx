/**************************************************************************
 *   Created: 2010/03/14 17:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Main.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include <TunnelEx/Log.hpp>
#include <TunnelEx/Error.hpp>

using namespace TunnelEx;

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	if (fdwReason  == DLL_PROCESS_ATTACH) {
		if (ACE::init() == -1) {
			const Error error(errno);
			Format message("Framework initialization failed with the system error \"%1%\".");
			message % error.GetErrorNo();
			Log::GetInstance().AppendSystemError(message.str());
			return FALSE;
		}
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		const int finiResult = ACE::fini();
		finiResult;
		BOOST_ASSERT(finiResult != -1);
	}
	return TRUE;
}

