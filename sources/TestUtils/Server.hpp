/**************************************************************************
 *   Created: 2009/04/12 17:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Server.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Server_hpp__0904121756
#define INCLUDED_FILE__TUNNELEX__Server_hpp__0904121756

namespace Test {

	//////////////////////////////////////////////////////////////////////////

	typedef std::vector<char> Buffer;

	//////////////////////////////////////////////////////////////////////////

	class SendError : public std::exception {
	public:
		explicit SendError(const char *what)
				: exception(what) {
			//...//
		}
	};

	//////////////////////////////////////////////////////////////////////////

	class ReceiveError : public std::exception {
	public:
		explicit ReceiveError(const char *what)
				: exception(what) {
			//...//
		}
	};

	//////////////////////////////////////////////////////////////////////////

	class Server {

	public:

		Server() {
			//...//
		}
		virtual ~Server() {
			//...//
		}

	private:

		Server(const Server &);
		const Server & operator =(const Server &);

	public:

		virtual bool IsConnected() const = 0;
		virtual std::size_t GetNumberOfAcceptedConnections() const = 0;

	public:

		virtual void CloseConnection(std::size_t connectionIndex) = 0;

	public:

		virtual void Send(
				std::size_t connectionIndex,
				const std::string &)
			throw(SendError)
			= 0;
		virtual void Send(
				std::size_t connectionIndex,
				const Buffer &)
			throw(SendError)
			= 0;

		virtual Buffer GetReceived(
				std::size_t connectionIndex)
			const
			throw(ReceiveError)
			= 0;
		std::string GetReceivedAsString(
					std::size_t connectionIndex)
				const
				throw(ReceiveError) {
			const Buffer data = GetReceived(connectionIndex);
			std::string result(data.begin(), data.end());
			if (result.size() && result[result.size() - 1] == 0) {
				result.resize(result.size() - 1);
			}
			return result;
		}
		virtual void ClearReceived(std::size_t connectionIndex) = 0;

	};

	//////////////////////////////////////////////////////////////////////////

	class Client {

	public:

		Client() {
			//...//
		}
		virtual ~Client() {
			//...//
		}

	private:

		Client(const Client &);
		const Client & operator =(const Client &);

	public:

		virtual void Send(const std::string &) throw(SendError) = 0;
		virtual void Send(const Buffer &) throw(SendError) = 0;

		virtual Buffer Receive() throw(ReceiveError) = 0;
		std::string ReceiveAsString() throw(ReceiveError) {
			const Buffer data = Receive();
			std::string result(data.begin(), data.end());
			if (result.size() && result[result.size() - 1] == 0) {
				result.resize(result.size() - 1);
			}
			return result;
		}

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // #ifndef INCLUDED_FILE__TUNNELEX__Server_hpp__0904121756
