/**************************************************************************
 *   Created: 2008/01/18 1:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: TrafficLogger.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "TrafficLogger.hpp"
#include "Tunnel.hpp"
#include "Connection.hpp"
#include "MessageBlock.hpp"
#include "Log.hpp"

using namespace std;
using namespace boost;
using namespace boost::posix_time;
using namespace boost::filesystem;
using namespace TunnelEx;

inline wpath GetFileNameForDump(	const WString &dir,
									const TunnelRule &rule,
									const Connection &connection) {
	LogTracking("", "GetFileNameForDump", __FILE__, __LINE__);
	tm tm(to_tm(microsec_clock::local_time()));
	wpath result(dir.GetCStr());
	WFormat format(L"%1%_%2%%3%%4%%5%%6%%7%_%8%.dump");
	format	//! \todo: add milliseconds here [2008/01/20 13:45]
			% rule.GetUuid().GetCStr()
			% (tm.tm_year + 1900) % (tm.tm_mon + 1) % tm.tm_mday % tm.tm_hour % tm.tm_mon % tm.tm_sec
			% connection.GetInstanceId();
	result /= format.str();
	return result;
}

TrafficLogger::TrafficLogger(	Server::Ref,
								const RuleEndpoint::ListenerInfo &info,
								const TunnelRule &rule,
								const Connection &currentConnection,
								const Connection &) {
	LogTracking("TrafficLogger::TrafficLogger", "TrafficLogger", __FILE__, __LINE__);
	const wpath filePath(GetFileNameForDump(info.param, rule, currentConnection));
	try {
		create_directories(filePath.branch_path());
		m_file.open(filePath.string().c_str(), ios::trunc | ios::binary);
		if (!m_file) {
			Log::GetInstance().AppendSystemError(
				(Format("Could not open file \"%1%\" for packets dumping.")
						% ConvertString<String>(filePath.string().c_str()).GetCStr()
					).str());
		} else {
			m_file	<< "Packets dump-file: rule \"" << ConvertString<String>(rule.GetUuid()).GetCStr()
					<< "\", connection \"" << currentConnection.GetInstanceId()
					<< "\", started at " << microsec_clock::local_time() << ".\r\n\r\n";
			m_file.flush();
		}
	} catch (const filesystem_error &ex) {
		LogTracking("TrafficLogger::TrafficLogger", "TrafficLogger", __FILE__, __LINE__);
		Log::GetInstance().AppendSystemError(
			(Format("Could not open file \"%2%\" for packets dumping: \"%1%\".")
					% ex.what()
					% ConvertString<String>(filePath.string().c_str()).GetCStr()
				).str());

	}
}

TrafficLogger::~TrafficLogger() {
	LogTracking("TrafficLogger::TrafficLogger", "~TrafficLogger", __FILE__, __LINE__);
}

DataTransferCommand TrafficLogger::OnNewMessageBlock(MessageBlock &messageBlock) {
	LogTracking("TrafficLogger", "OnNewMessageBlock", __FILE__, __LINE__);
	m_file	<< "-- Packet begin -- " << microsec_clock::local_time()
			<< " -- " << setw(16) << messageBlock.GetUnreadedDataSize() << " bytes --\r\n";
	m_file.write(messageBlock.GetData(), streamsize(messageBlock.GetUnreadedDataSize()));
	m_file	<< "\r\n-- Packet end -------------------------------------------------------------\r\n\r\n";
	m_file.flush();
	return DATA_TRANSFER_CMD_SEND_PACKET;
}

