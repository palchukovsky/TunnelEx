/**************************************************************************
 *   Created: 2007/11/10 1:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.h"

#include "Log.hpp"
#include "Error.hpp"

using namespace TunnelEx;

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	if (fdwReason  == DLL_PROCESS_ATTACH) {
		if (ACE::init() == -1) {
			const Error error(errno);
			Format message(
				"Framework initialization failed with the system error \"%1%\".");
			message % error.GetErrorNo();
			Log::GetInstance().AppendSystemError(message.str());
			return FALSE;
		}
		  ACE::set_handle_limit();
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		const int finiResult = ACE::fini();
		ACE_UNUSED_ARG(finiResult);
		assert(finiResult != -1);
	}
	return TRUE;
}

