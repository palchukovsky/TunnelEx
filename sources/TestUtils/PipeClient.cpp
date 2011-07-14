/**************************************************************************
 *   Created: 2011/07/12 17:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"
#include "PipeClient.hpp"
#include "PipeConnection.hpp"

using namespace TestUtil;

PipeClient::PipeClient(const std::string &path) {
	//...//
}

PipeClient::~PipeClient() {
	//...//
}

void PipeClient::Send(const std::string &message) {
	std::auto_ptr<Buffer> buffer(
		new Buffer(message.begin(), message.end()));
	Send(buffer);
}

void PipeClient::Send(std::auto_ptr<Buffer> buffer) {
	GetConnection().Send(buffer);
}

Buffer::size_type PipeClient::GetReceivedSize() const {
	return GetConnection().GetReceivedSize();
}
		
void PipeClient::GetReceived(
			Buffer::size_type maxSize,
			Buffer &result)
		const {
	GetConnection().GetReceived(maxSize, result);
}

bool PipeClient::IsConnected() const {
	return m_connection.get() && m_connection->IsActive();
}

void PipeClient::ClearReceived(size_t bytesCount/* = 0*/) {
	GetConnection().ClearReceived(bytesCount);
}

void PipeClient::Disconnect() {
	m_connection->Close();
}

bool PipeClient::WaitDataReceiveEvent(
			const boost::system_time &waitUntil,
			Buffer::size_type minSize)
		const {
	return GetConnection().WaitDataReceiveEvent(waitUntil, minSize);
}

PipeClient::Connection & PipeClient::GetConnection() {
	if (!m_connection.get()) {
		throw ConnectionClosed();
	}
	return *m_connection;
}
const PipeClient::Connection & PipeClient::GetConnection() const {
	return const_cast<Self *>(this)->GetConnection();
}
