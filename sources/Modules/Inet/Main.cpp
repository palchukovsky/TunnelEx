/**************************************************************************
 *   Created: 2008/11/06 13:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "Api.h"

#include "Core/Log.hpp"
#include "Core/Error.hpp"

using namespace TunnelEx;

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	if (fdwReason  == DLL_PROCESS_ATTACH) {
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
		if (ACE::init() == -1) {
			const Error error(errno);
			Format message("Framework initialization failed with the system error \"%1%\".");
			message % error.GetErrorNo();
			Log::GetInstance().AppendSystemError(message.str());
			return FALSE;
		}
		ACE::set_handle_limit();
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		const int finiResult = ACE::fini();
		finiResult;
		assert(finiResult != -1);
	}
	return TRUE;
}

namespace TunnelEx { namespace Mods { namespace Inet {

	const char * GetModuleName() {
		return TUNNELEX_MODULE_INET_NAME_FULL;
	}

	const char * GetModuleVersion() {
		return TUNNELEX_VERSION_FULL TUNNELEX_BUILD_IDENTITY_ADD;
	}

} } }
