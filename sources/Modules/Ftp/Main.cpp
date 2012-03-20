/**************************************************************************
 *   Created: 2012/2/29/ 23:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "FtpListener.hpp"


namespace TunnelEx { namespace Mods { namespace Ftp {

	const char * GetModuleName() {
		return TUNNELEX_MODULE_FTP_NAME_FULL;
	}

	const char * GetModuleVersion() {
		return TUNNELEX_VERSION_FULL TUNNELEX_BUILD_IDENTITY_ADD;
	}

	SharedPtr<PreListener> CreateActiveFtpListener(
				Server::Ref server,
				const RuleEndpoint::ListenerInfo &,
				const TunnelRule &,
				const Connection &currentConnection,
				const Connection &oppositeConnection) {
		return SharedPtr<PreListener>(
			new FtpListenerForActiveMode(server, currentConnection, oppositeConnection));
	}

	SharedPtr<PreListener> CreatePassiveFtpListener(
				Server::Ref server,
				const RuleEndpoint::ListenerInfo &,
				const TunnelRule &,
				const Connection &currentConnection,
				const Connection &oppositeConnection) {
		return SharedPtr<PreListener>(
			new FtpListenerForPassiveMode(server, currentConnection, oppositeConnection));
	}

} } }
