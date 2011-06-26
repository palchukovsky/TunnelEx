/**************************************************************************
 *   Created: 2011/06/01 21:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "ClientServer.hpp"

using namespace TestUtil;
namespace pt = boost::posix_time;

namespace {

	struct Util {

		static void GetMinMaxSize(
					const std::list<const std::string *> &list,
					size_t &minSize,
					size_t &maxSize) {
			size_t minTmp = std::numeric_limits<size_t>::max();
			size_t maxTmp = std::numeric_limits<size_t>::min();
			foreach (const std::string *const i, list) {
				if (i->size() > maxTmp) {
					maxTmp = i->size();
				}
				if (i->size() < minTmp) {
					minTmp = i->size();
				}
			}
			minSize = minTmp;
			maxSize = maxTmp;
		}
	
		static size_t CheckReceivedData(
					const std::list<const std::string *> &waitDataList,
					const std::string &receivedData) {
			size_t i = 0;
			foreach (const std::string *const waitData, waitDataList) {
				++i;
				if (*waitData == receivedData) {
					return i;
				}
			}
			return 0;
		}
	
	};


}

namespace {

	const pt::milliseconds iterationSleepTime(1);

}

Server::Server() {
	//...//
}

Server::~Server() {
	//...//
}

bool Server::WaitConnect(size_t connectionsNumber, bool infiniteTimeout) const {
	const pt::ptime toTime = boost::get_system_time() + GetWaitTime();
	while (
			GetNumberOfAcceptedConnections(true) < connectionsNumber
			&& (infiniteTimeout || boost::get_system_time() <= toTime)) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return GetNumberOfAcceptedConnections(true) >= connectionsNumber;
}

bool Server::WaitDisconnect(size_t connectionIndex) const {
	const pt::ptime toTime = boost::get_system_time() + GetWaitTime();
	while (	IsConnected(connectionIndex, true)
			&& boost::get_system_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return !IsConnected(connectionIndex, true);
}

void Server::WaitAnyData(
			size_t connectionIndex,
			Buffer::size_type size,
			bool isExactly,
			Buffer &result)
		const {

	if (	!WaitDataReceiveEvent(
				connectionIndex,
				boost::get_system_time() + GetWaitTime(),
				size)) {
		throw Timeout();
	}

	Buffer resultTmp;
	GetReceived(connectionIndex, size + 1, resultTmp);
	if (isExactly && size != resultTmp.size()) {
		assert(size < resultTmp.size());
		throw TooMuchDataReceived();
	}
	resultTmp.resize(size);
	resultTmp.swap(result);

}

void Server::WaitAndTakeAnyData(
			size_t connectionIndex,
			Buffer::size_type size,
			bool isExactly,
			Buffer &result) {
	Buffer resultTmp;
	WaitAnyData(connectionIndex, size, isExactly, resultTmp);
	ClearReceived(connectionIndex, size);
	resultTmp.swap(result);
}

bool Server::WaitData(
			size_t connectionIndex,
			const Buffer &waitData,
			bool isExactly)
		const {
	const pt::ptime toTime = boost::get_system_time() + GetWaitTime();
	Buffer data;
	for ( ; ; ) {
		GetReceived(
			connectionIndex,
			isExactly ? waitData.size() + 1 : waitData.size(),
			data);
		if (waitData == data) {
			return true;
		} else if (boost::get_system_time() > toTime) {
			return false;
		}
		boost::this_thread::sleep(iterationSleepTime);
	}
}

bool Server::WaitData(
			size_t connectionIndex,
			const std::string &waitData,
			bool isExactly)
		const {
	if (	!WaitDataReceiveEvent(
				connectionIndex,
				boost::get_system_time() + GetWaitTime(),
				waitData.size())) {
		return false;
	}
	std::string data;
	GetReceived(
		connectionIndex,
		isExactly ? waitData.size() + 1 : waitData.size(),
		data);
	return waitData == data;
}

size_t Server::WaitData(
			size_t connectionIndex,
			const std::list<const std::string *> &waitData,
			bool isExactly)
		const {
	const pt::ptime waitUntil = boost::get_system_time() + GetWaitTime();
	std::string data;
	size_t minSize = 0;
	size_t maxSize = 0;
	Util::GetMinMaxSize(waitData, minSize, maxSize);
	if (!WaitDataReceiveEvent(connectionIndex, waitUntil, minSize)) {
		return 0;
	}
	for ( ; ; ) {
		GetReceived(connectionIndex, isExactly ? maxSize + 1 : maxSize, data);
		const auto result = Util::CheckReceivedData(waitData, data);
		if (result != 0) {
			return result;
		} else if (boost::get_system_time() > waitUntil) {
			return 0;
		}
		boost::this_thread::sleep(iterationSleepTime);
	}
}

size_t Server::WaitAndTakeData(
			size_t connectionIndex,
			const std::list<const std::string *> &data,
			bool isExactly) {
	const size_t result = WaitData(connectionIndex, data, isExactly);
	if (result != 0) {
		std::list<const std::string *>::const_iterator pos = data.begin();
		std::advance(pos, result - 1);
		ClearReceived(connectionIndex, (**pos).size());
	}
	return result;
}

void Server::GetReceived(
			size_t connectionIndex,
			size_t maxSize,
			std::string &result)
		const {
	Buffer data;
	GetReceived(connectionIndex, maxSize, data);
	std::string resultTmp(data.begin(), data.end());
	if (resultTmp.size() && resultTmp[resultTmp.size() - 1] == 0) {
		resultTmp.resize(resultTmp.size() - 1);
	}
	resultTmp.swap(result);
}

////////////////////////////////////////////////////////////////////////////////

Client::Client() {
	//...//
}

Client::~Client() {
	//...//
}

void Client::GetReceived(size_t maxSize, std::string &result) const {
	Buffer data;
	GetReceived(maxSize, data);
	std::string resultTmp(data.begin(), data.end());
	if (resultTmp.size() && resultTmp[resultTmp.size() - 1] == 0) {
		resultTmp.resize(resultTmp.size() - 1);
	}
	resultTmp.swap(result);
}

bool Client::WaitConnect(bool infiniteTimeout) const {
	const pt::ptime toTime = boost::get_system_time() + GetWaitTime();
	while (		!IsConnected()
				&& (infiniteTimeout || boost::get_system_time() <= toTime)) {
			boost::this_thread::sleep(iterationSleepTime);
	}
	return IsConnected();
}

bool Client::WaitDisconnect() const {
	const pt::ptime toTime = boost::get_system_time() + GetWaitTime();
	while (IsConnected() && boost::get_system_time() <= toTime) {
			boost::this_thread::sleep(iterationSleepTime);
	}
	return !IsConnected();
}

void Client::WaitAnyData(
			Buffer::size_type size,
			bool isExactly,
			Buffer &result)
		const {
	if (!WaitDataReceiveEvent(boost::get_system_time() + GetWaitTime(), size)) {
		throw Timeout();
	}
	Buffer resultTmp;
	GetReceived(isExactly ? size + 1 : size, resultTmp);
	if (isExactly && size != resultTmp.size()) {
		assert(size > resultTmp.size());
		throw TooMuchDataReceived();
	}
	resultTmp.resize(size);
	resultTmp.swap(result);
}

void Client::WaitAndTakeAnyData(
			Buffer::size_type size,
			bool isExactly,
			Buffer &result) {
	Buffer resultTmp;
	WaitAnyData(size, isExactly, resultTmp);
	ClearReceived(size);
	resultTmp.swap(result);
}

bool Client::WaitData(const Buffer &waitData, bool isExactly) const {
	const pt::ptime toTime = boost::get_system_time() + GetWaitTime();
	Buffer data;
	for ( ; ; ) {
		GetReceived(
			isExactly ? waitData.size() + 1 : waitData.size(),
			data);
		if (waitData == data) {
			return true;
		} else if (boost::get_system_time() > toTime) {
			return false;
		}
		boost::this_thread::sleep(iterationSleepTime);
	}
}

bool Client::WaitData(const std::string &waitData, bool isExactly) const {
	const pt::ptime toTime = boost::get_system_time() + GetWaitTime();
	std::string data;
	for ( ; ; ) {
		GetReceived(
			isExactly ? waitData.size() + 1 : waitData.size(),
			data);
		if (waitData == data) {
			return true;
		} else if (boost::get_system_time() > toTime) {
			return false;
		}
		boost::this_thread::sleep(iterationSleepTime);
	}
}

size_t Client::WaitData(
			const std::list<const std::string *> &waitData,
			bool isExactly)
		const {
	const pt::ptime waitUntil = boost::get_system_time() + GetWaitTime();
	std::string data;
	size_t minSize = 0;
	size_t maxSize = 0;
	Util::GetMinMaxSize(waitData, minSize, maxSize);
	if (!WaitDataReceiveEvent(waitUntil, minSize)) {
		return 0;
	}
	for ( ; ; ) {
		GetReceived(isExactly ? maxSize + 1 : maxSize, data);
		const auto result = Util::CheckReceivedData(waitData, data);
		if (result != 0) {
			return result;
		} else if (boost::get_system_time() > waitUntil) {
			return 0;
		}
		boost::this_thread::sleep(iterationSleepTime);
	}
}

size_t Client::WaitAndTakeData(
			const std::list<const std::string *> &data,
			bool isExactly) {
	const size_t result = WaitData(data, isExactly);
	if (result != 0) {
		std::list<const std::string *>::const_iterator pos = data.begin();
		std::advance(pos, result - 1);
		ClearReceived((**pos).size());
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////
