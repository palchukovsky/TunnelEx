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
	
		static size_t CheckReceivedData(
					const std::list<const std::string *> &waitDataList,
					const std::string &receivedData,
					bool isExactly) {
			size_t i = 0;
			foreach (const std::string *const waitData, waitDataList) {
				++i;
				if (isExactly
						?	*waitData == receivedData
						:	boost::starts_with(receivedData, *waitData)) {
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
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (
			GetNumberOfAcceptedConnections(true) < connectionsNumber
			&& (infiniteTimeout || pt::second_clock::local_time() <= toTime)) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return GetNumberOfAcceptedConnections(true) >= connectionsNumber;
}

bool Server::WaitDisconnect(size_t connectionIndex) const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (IsConnected(connectionIndex, true) && pt::second_clock::local_time() <= toTime) {
			boost::this_thread::sleep(iterationSleepTime);
	}
	return !IsConnected(connectionIndex, true);
}

Buffer Server::WaitAnyData(
			size_t connectionIndex,
			Buffer::size_type size,
			bool isExactly)
		const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	size > GetReceivedSize(connectionIndex)
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	Buffer result = GetReceived(connectionIndex);
	if (size > result.size()) {
		throw Timeout();
	} else if (isExactly && size != result.size()) {
		throw TooMuchDataReceived();
	}
	result.resize(size);
	return result;
}

Buffer Server::WaitAndTakeAnyData(
			size_t connectionIndex,
			Buffer::size_type size,
			bool isExactly) {
	const Buffer result = WaitAnyData(connectionIndex, size, isExactly);
	ClearReceived(connectionIndex, size);
	return result;
}

bool Server::WaitData(
			size_t connectionIndex,
			const Buffer &waitData,
			bool isExactly)
		const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	!(isExactly
				?	waitData == GetReceived(connectionIndex)
				:	boost::starts_with(GetReceived(connectionIndex), waitData))
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return isExactly
		?	waitData == GetReceived(connectionIndex)
		:	boost::starts_with(GetReceived(connectionIndex), waitData);
}

bool Server::WaitData(
			size_t connectionIndex,
			const std::string &waitData,
			bool isExactly)
		const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	!(isExactly
				?	waitData == GetReceivedAsString(connectionIndex)
				:	boost::starts_with(GetReceivedAsString(connectionIndex), waitData))
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return isExactly
		?	waitData == GetReceivedAsString(connectionIndex)
		:	boost::starts_with(GetReceivedAsString(connectionIndex), waitData);
}

size_t Server::WaitData(
			size_t connectionIndex,
			const std::list<const std::string *> &data,
			bool isExactly)
		const {

	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	Util::CheckReceivedData(data, GetReceivedAsString(connectionIndex), isExactly) == 0
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return Util::CheckReceivedData(
		data,
		GetReceivedAsString(connectionIndex),
		isExactly);

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

std::string Server::GetReceivedAsString(size_t connectionIndex) const {
	const Buffer data = GetReceived(connectionIndex);
	std::string result(data.begin(), data.end());
	if (result.size() && result[result.size() - 1] == 0) {
		result.resize(result.size() - 1);
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////

Client::Client() {
	//...//
}

Client::~Client() {
	//...//
}

std::string Client::GetReceivedAsString() const {
	const Buffer data = GetReceived();
	std::string result(data.begin(), data.end());
	if (result.size() && result[result.size() - 1] == 0) {
		result.resize(result.size() - 1);
	}
	return result;
}

bool Client::WaitConnect() const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (!IsConnected() && pt::second_clock::local_time() <= toTime) {
			boost::this_thread::sleep(iterationSleepTime);
	}
	return IsConnected();
}

bool Client::WaitDisconnect() const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (IsConnected() && pt::second_clock::local_time() <= toTime) {
			boost::this_thread::sleep(iterationSleepTime);
	}
	return !IsConnected();
}

Buffer Client::WaitAnyData(Buffer::size_type size, bool isExactly) const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	size > GetReceivedSize()
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	Buffer result = GetReceived();
	if (size > result.size()) {
		throw Timeout();
	} else if (isExactly && size != result.size()) {
		throw TooMuchDataReceived();
	}
	result.resize(size);
	return result;
}

Buffer Client::WaitAndTakeAnyData(Buffer::size_type size, bool isExactly) {
	const Buffer result = WaitAnyData(size, isExactly);
	ClearReceived(size);
	return result;
}

bool Client::WaitData(const Buffer &waitData, bool isExactly) const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	!(isExactly
				?	waitData == GetReceived()
				:	boost::starts_with(GetReceived(), waitData))
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return isExactly
		?	waitData == GetReceived()
		:	boost::starts_with(GetReceived(), waitData);
}

bool Client::WaitData(const std::string &waitData, bool isExactly) const {
	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	!(isExactly
				?	waitData == GetReceivedAsString()
				:	boost::starts_with(GetReceivedAsString(), waitData))
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return isExactly
		?	waitData == GetReceivedAsString()
		:	boost::starts_with(GetReceivedAsString(), waitData);
}

size_t Client::WaitData(
			const std::list<const std::string *> &data,
			bool isExactly)
		const {

	const pt::ptime toTime = pt::second_clock::local_time() + GetWaitTime();
	while (	Util::CheckReceivedData(data, GetReceivedAsString(), isExactly) == 0
			&& pt::second_clock::local_time() <= toTime) {
		boost::this_thread::sleep(iterationSleepTime);
	}
	return Util::CheckReceivedData(data, GetReceivedAsString(), isExactly);

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
