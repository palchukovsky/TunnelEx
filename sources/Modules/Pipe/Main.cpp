/**************************************************************************
 *   Created: 2008/11/26 14:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.h"

#include "Core/Log.hpp"
#include "Core/Error.hpp"

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
		assert(finiResult != -1);
	}
	return TRUE;
}
