/**************************************************************************
 *   Created: 2009/04/12 17:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Server_hpp__0904121756
#define INCLUDED_FILE__TUNNELEX__Server_hpp__0904121756

namespace TestUtil {

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

	class Timeout : public std::exception {
	public:
		Timeout()
				: exception("Timeout") {
			//...//
		}
	};

	//////////////////////////////////////////////////////////////////////////

	class ConnectionClosed : public std::exception {
	public:
		ConnectionClosed()
				: exception("Connection closed") {
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

	////////////////////////////////////////////////////////////////////////////////

	class TooMuchDataReceived : public std::exception {
	public:
		explicit TooMuchDataReceived()
				: exception("Too much data received") {
			//...//
		}
	};

	////////////////////////////////////////////////////////////////////////////////

	class Connector : private boost::noncopyable {

	public:

		Connector()
				: m_waitTime(0, 0, 0) {
			//...//
		}

	public:

		void SetWaitTime(const boost::posix_time::time_duration &time) {
			m_waitTime = time;
		}

		const boost::posix_time::time_duration & GetWaitTime() const {
			return m_waitTime;
		}

	private:

		boost::posix_time::time_duration m_waitTime;

	};

	//////////////////////////////////////////////////////////////////////////

	class Server : public Connector {

	public:

		Server();
		virtual ~Server();

	private:

		Server(const Server &);
		const Server & operator =(const Server &);

	public:

		virtual bool IsConnected(bool onlyIfActive) const = 0;
		virtual bool IsConnected(size_t connectionIndex, bool onlyIfActive) const = 0;
		virtual size_t GetNumberOfAcceptedConnections(bool onlyIfActive) const = 0;

	public:

		bool WaitConnect(size_t connectionsNumber, bool infiniteTimeout) const;
		bool WaitDisconnect(size_t connectionIndex) const;

	public:

		Buffer WaitAnyData(
				size_t connectionIndex,
				Buffer::size_type size,
				bool isExactly)
			const;

		Buffer WaitAndTakeAnyData(
				size_t connectionIndex,
				Buffer::size_type size,
				bool isExactly);

	public:

		bool WaitData(
				size_t connectionIndex,
				const Buffer &,
				bool isExactly)
			const;

		bool WaitData(
				size_t connectionIndex,
				const std::string &,
				bool isExactly)
			const;

		size_t WaitData(
				size_t connectionIndex,
				const std::list<const std::string *> &,
				bool isExactly)
			const;

		template<typename T>
		T WaitData(size_t connectionIndex, bool isExactly) const {
			return reinterpret_cast<const T &>(
				*&WaitAnyData(connectionIndex, sizeof(T), isExactly)[0]);
		}

	public:
	
		template<typename T>
		bool WaitAndTakeData(size_t connectionIndex, const T &data, bool isExactly) {
			if (!WaitData(connectionIndex, data, isExactly)) {
				return false;
			}
			ClearReceived(connectionIndex, data.size());
			return true;
		}
		
		size_t WaitAndTakeData(
					size_t connectionIndex,
					const std::list<const std::string *> &,
					bool isExactly);
		
		template<typename T>
		T WaitAndTakeData(size_t connectionIndex, bool isExactly) {
			const T result = WaitData<T>(connectionIndex, isExactly);
			ClearReceived(connectionIndex, sizeof(T));
			return result;
		}

	public:

		virtual void CloseConnection(size_t connectionIndex) = 0;

	public:

		/** @throw SendError 
		  */
		template<typename T>
		void SendVal(size_t connectionIndex, const T &val) {
			Buffer buffer(sizeof(val), 0);
			memcpy(&buffer[0], &val, sizeof(val));
			Send(connectionIndex, buffer);
		}
		/** @throw SendError 
		  */
		virtual void Send(size_t connectionIndex, const std::string &) = 0;
		/** @throw SendError 
		  */
		virtual void Send(size_t connectionIndex, const Buffer &) = 0;

		/** @throw ReceiveError
		  */
		virtual Buffer::size_type GetReceivedSize(size_t connectionIndex) const = 0;
		
		/** @throw ReceiveError
		  */
		virtual Buffer GetReceived(size_t connectionIndex) const = 0;

		/** @throw ReceiveError
		  */
		std::string GetReceivedAsString(size_t connectionIndex) const;

		virtual void ClearReceived(size_t connectionIndex, size_t bytesCount = 0) = 0;

	};

	//////////////////////////////////////////////////////////////////////////

	class Client : public Connector {

	public:

		Client();
		virtual ~Client();

	private:

		Client(const Client &);
		const Client & operator =(const Client &);

	public:

		/** @throw SendError
		  */
		virtual void Send(const std::string &) = 0;
		/** @throw SendError
		  */
		virtual void Send(const Buffer &) = 0;
		/** @throw SendError 
		  */
		template<typename T>
		void SendVal(const T &val) {
			Buffer buffer(sizeof(val), 0);
			memcpy(&buffer[0], &val, sizeof(val));
			Send(buffer);
		}

		/** @throw ReceiveError
		  */
		virtual Buffer::size_type GetReceivedSize() const = 0;
		/** @throw ReceiveError
		  */
		virtual Buffer GetReceived() const = 0;
		/** @throw ReceiveError
		  */
		std::string GetReceivedAsString() const;

		virtual bool IsConnected() const = 0;

		virtual void ClearReceived(size_t bytesCount = 0) = 0;
		virtual void Disconnect() = 0;

	public:

		bool WaitConnect() const;
		bool WaitDisconnect() const;

	public:
	
		Buffer WaitAnyData(Buffer::size_type size, bool isExactly) const;
		Buffer WaitAndTakeAnyData(Buffer::size_type size, bool isExactly);

	public:
	
		bool WaitData(const Buffer &, bool isExactly) const;
		
		bool WaitData(const std::string &, bool isExactly) const;
		
		size_t WaitData(
					const std::list<const std::string *> &,
					bool isExactly)
				const;
		
		template<typename T>
		T WaitData(bool isExactly) const {
			return reinterpret_cast<const T &>(*&WaitAnyData(sizeof(T), isExactly)[0]);
		}

	public:
	
		size_t WaitAndTakeData(
					const std::list<const std::string *> &,
					bool isExactly);
		
		template<typename T>
		bool WaitAndTakeData(const T &data, bool isExactly) {
			if (!WaitData(data, isExactly)) {
				return false;
			}
			ClearReceived(data.size());
			return true;
		}
		
		template<typename T>
		T WaitAndTakeData(bool isExactly) {
			const T result = WaitData<T>(isExactly);
			ClearReceived(sizeof(T));
			return result;
		}

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // #ifndef INCLUDED_FILE__TUNNELEX__Server_hpp__0904121756
