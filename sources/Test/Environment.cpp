/**************************************************************************
 *   Created: 2008/10/21 14:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Environment.hpp"

#include "Core/Server.hpp"
#include "Core/Log.hpp"

using namespace Test;
namespace tex = TunnelEx;

////////////////////////////////////////////////////////////////////////////////

namespace {

	void XmlErrosNull(void *, const char *, ...) {
		//...//
	}

}

////////////////////////////////////////////////////////////////////////////////

Environment::~Environment() {
	//...//
}

void Environment::SetUp() {
	SetLogParams();
	xmlInitParser();
	OpenSSL_add_all_algorithms();
	tex::Helpers::Xml::SetErrorsHandler(&XmlErrosNull);
}

void Environment::TearDown() {
	if (tex::Server::GetInstance().IsStarted()) {
		while (tex::Server::GetInstance().GetTunnelsNumber()) {
			Sleep(500);
		}
		assert(!tex::Server::GetInstance().IsStarted());
		tex::Server::GetInstance().Stop();
	}
	EVP_cleanup();
}

void Environment::SetLogParams() {
	for (int level = tex::LOG_LEVEL_UNKNOWN + 1; level < tex::LOG_LEVEL_LEVELS_COUNT; ++level) {
		tex::Log::GetInstance().SetLevelRegistrationState(
			static_cast<tex::LogLevel>(level),
			true);
	}
	// streams and files not attached: so it turn on all log outputs, but it still be invisible
	// Log::GetInstance().AttachStdoutStream();
}

////////////////////////////////////////////////////////////////////////////////

LocalEnvironment::~LocalEnvironment() {
	//...//
}

void LocalEnvironment::SetUp() {
	//...//
}

void LocalEnvironment::TearDown() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

ServerEnvironment::~ServerEnvironment() {
	//...//
}

void ServerEnvironment::SetUp() {
	//...//
}

void ServerEnvironment::TearDown() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

ClientEnvironment::~ClientEnvironment() {
	//...//
}

void ClientEnvironment::SetUp() {
	//...//
}

void ClientEnvironment::TearDown() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////
