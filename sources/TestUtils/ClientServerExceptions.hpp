/**************************************************************************
 *   Created: 2011/06/30 22:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ClientServerExceptions_hpp__1106302237
#define INCLUDED_FILE__TUNNELEX__ClientServerExceptions_hpp__1106302237

namespace TestUtil {

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

}

#endif // INCLUDED_FILE__TUNNELEX__ClientServerExceptions_hpp__1106302237
