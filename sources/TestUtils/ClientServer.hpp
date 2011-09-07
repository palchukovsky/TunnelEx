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

#include "ClientServerExceptions.hpp"

namespace TestUtil {

	//////////////////////////////////////////////////////////////////////////

	typedef std::vector<char> Buffer;

	////////////////////////////////////////////////////////////////////////////////

	class Connector : private boost::noncopyable {

	public:

		Connector(const boost::posix_time::time_duration &waitTime)
				: m_waitTime(waitTime) {
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

		Server(const boost::posix_time::time_duration &waitTime);
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

		void WaitAnyData(
				size_t connectionIndex,
				Buffer::size_type size,
				bool isExactly,
				Buffer &result)
			const;

		void WaitAndTakeAnyData(
				size_t connectionIndex,
				Buffer::size_type size,
				bool isExactly,
				Buffer &result);

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
			Buffer buffer;
			WaitAnyData(connectionIndex, sizeof(T), isExactly, buffer);
			return reinterpret_cast<const T &>(*&buffer[0]);
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
			std::auto_ptr<Buffer> buffer(new Buffer(sizeof(val), 0));
			memcpy(&(*buffer)[0], &val, sizeof(val));
			Send(connectionIndex, buffer);
		}
		/** @throw SendError 
		  */
		virtual void Send(size_t connectionIndex, const std::string &) = 0;
		/** @throw SendError 
		  */
		virtual void Send(size_t connectionIndex, std::auto_ptr<Buffer>) = 0;

		/** @throw ReceiveError
		  */
		virtual Buffer::size_type GetReceivedSize(size_t connectionIndex) const = 0;
		
		/** @throw ReceiveError
		  */
		virtual void GetReceived(
				size_t connectionIndex,
				size_t maxSize,
				Buffer &result)
			const = 0;

		/** @throw ReceiveError
		  */
		void GetReceived(
				size_t connectionIndex,
				size_t maxSize,
				std::string &result)
			const;

		virtual void ClearReceived(size_t connectionIndex, size_t bytesCount = 0) = 0;

	protected:

		virtual bool WaitDataReceiveEvent(
				size_t connectionIndex,
				const boost::system_time &waitUntil,
				Buffer::size_type minSize)
			const
			= 0;

	};

	//////////////////////////////////////////////////////////////////////////

	class Client : public Connector {

	public:

		Client(const boost::posix_time::time_duration &timeOut);
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
		virtual void Send(std::auto_ptr<Buffer>) = 0;
		/** @throw SendError 
		  */
		template<typename T>
		void SendVal(const T &val) {
			std::auto_ptr<Buffer> buffer(new Buffer(sizeof(val), 0));
			memcpy(&(*buffer)[0], &val, sizeof(val));
			Send(buffer);
		}

		/** @throw ReceiveError
		  */
		virtual Buffer::size_type GetReceivedSize() const = 0;
		/** @throw ReceiveError
		  */
		virtual void GetReceived(size_t maxSize, Buffer &result) const = 0;
		/** @throw ReceiveError
		  */
		void GetReceived(size_t maxSize, std::string &) const;

		virtual bool IsConnected() const = 0;

		virtual void ClearReceived(size_t bytesCount = 0) = 0;
		virtual void Disconnect() = 0;

	public:

		bool WaitConnect(bool infiniteTimeout) const;
		bool WaitDisconnect() const;

	public:
	
		void WaitAnyData(
				Buffer::size_type size,
				bool isExactly,
				Buffer &result)
			const;
		void WaitAndTakeAnyData(
				Buffer::size_type size,
				bool isExactly,
				Buffer &result);

	public:
	
		bool WaitData(const Buffer &, bool isExactly) const;
		
		bool WaitData(const std::string &, bool isExactly) const;
		
		size_t WaitData(
					const std::list<const std::string *> &,
					bool isExactly)
				const;
		
		template<typename T>
		T WaitData(bool isExactly) const {
			Buffer buffer;
			WaitAnyData(sizeof(T), isExactly, buffer);
			return reinterpret_cast<const T &>(*&buffer[0]);
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

	protected:

		virtual bool WaitDataReceiveEvent(
				const boost::system_time &waitUntil,
				Buffer::size_type minSize)
			const
			= 0;

	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // #ifndef INCLUDED_FILE__TUNNELEX__Server_hpp__0904121756
