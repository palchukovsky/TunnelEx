/**************************************************************************
 *   Created: 2008/10/21 14:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Fixtures.hpp"

#include "Core/Server.hpp"
#include "Core/Log.hpp"

namespace ut = boost::unit_test;
using namespace Test;
using namespace TunnelEx;

void XmlErrosNull(void *, const char *, ...) {
	//...//
}

GlobalFixture::GlobalFixture() {
	SetLogParams();
	xmlInitParser();
	OpenSSL_add_all_algorithms();
	TunnelEx::Helpers::Xml::SetErrorsHandler(&XmlErrosNull);
}

GlobalFixture::~GlobalFixture() {
	if (Server::GetInstance().IsStarted()) {
		while (Server::GetInstance().GetTunnelsNumber()) {
			Sleep(500);
		}
		BOOST_ASSERT(!Server::GetInstance().IsStarted());
		Server::GetInstance().Stop();
	}
	EVP_cleanup();
}

void GlobalFixture::SetLogParams() {
	for (int level = LOG_LEVEL_UNKNOWN + 1; level < LOG_LEVEL_LEVELS_COUNT; ++level) {
		Log::GetInstance().SetLevelRegistrationState(
			static_cast<LogLevel>(level),
			true);
	}
	// streams and files not attached: so it turn on all log outputs, but it still be invisible
	// Log::GetInstance().AttachStdoutStream();
}
