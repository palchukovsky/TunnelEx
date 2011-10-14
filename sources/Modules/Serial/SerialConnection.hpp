/**************************************************************************
 *   Created: 2009/09/17 16:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__OutcomingSerialPortConnection_hpp__0909171625
#define INCLUDED_FILE__TUNNELEX__OutcomingSerialPortConnection_hpp__0909171625

#include "SerialEndpointAddress.hpp"

#include "Core/Connection.hpp"
#include "Core/Log.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Error.hpp"

namespace TunnelEx { namespace Mods { namespace Serial {

	class SerialConnection : public TunnelEx::Connection {

	public:

		explicit SerialConnection(
					const SerialEndpointAddress &address,
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Connection(ruleEndpoint, ruleEndpointAddress) {
			const int connectResult = m_connector.connect(
				m_io,
				address.GetAceDevAddr(),
				0,
				ACE_Addr::sap_any,
				1,
				O_RDWR | FILE_FLAG_OVERLAPPED);
			if (connectResult != 0) {
				const Error error(errno);
				LogTracking("SerialConnection", "SerialConnection", __FILE__, __LINE__);
				WFormat message(L"Could not open serial line: \"%1% (%2%)\".");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
			// set the appropriate parameters
			ACE_TTY_IO::Serial_Params params;
			//! @todo: add params getting, when it will be implemented in ACE.
			/* if (m_io.control(ACE_TTY_IO::GETPARAMS, &params) != 0) {
				const Error error(errno);
				LogTracking("SerialConnection", "SerialConnection", __FILE__, __LINE__);
				WFormat message(L"Could not get serial device parameters: \"%1% (%2%)\".");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			} */
			params.modem = false;
			params.rcvenb = true;
			params.readmincharacters = 0;
			params.xonlim = 0;
			params.xofflim = 0;
			params.baudrate = address.GetBaudRate();
			params.databits = address.GetDataBits();
			params.stopbits = address.GetStopBits();
			params.xinenb = false;
			params.xoutenb = false;
			params.ctsenb = false;
			params.rtsenb = RTS_CONTROL_ENABLE;
			params.dtrdisable = true;
			params.dsrenb = false;
			params.paritymode = 0;
			switch (address.GetParity()) {
				default:
					assert(false);
				case SerialEndpointAddress::P_NONE:
					break;
				case SerialEndpointAddress::P_ODD:
					params.paritymode = "odd";
					break;
				case SerialEndpointAddress::P_EVEN:
					params.paritymode = "even";
					break;
				case SerialEndpointAddress::P_MARK:
					params.paritymode = "mark";
					break;
				case SerialEndpointAddress::P_SPACE:
					params.paritymode = "space";
					break;
			}
			switch (address.GetFlowControl()) {
				default:
					assert(false);
				case SerialEndpointAddress::FC_NONE:
					break;
				case SerialEndpointAddress::FC_XON_XOFF:
					params.xinenb
						= params.xoutenb
						= true;
					break;
				case SerialEndpointAddress::FC_RTS_CTS:
					params.ctsenb = true;
					params.rtsenb = RTS_CONTROL_HANDSHAKE;
					break;
				case SerialEndpointAddress::FC_DSR_DTR:
					params.dtrdisable = false;
					params.dsrenb = true;
					break;
			}
			if (m_io.control(ACE_TTY_IO::SETPARAMS, &params) != 0) {
				const Error error(errno);
				LogTracking("SerialConnection", "SerialConnection", __FILE__, __LINE__);
				WFormat message(L"Could not set serial device parameters: \"%1% (%2%)\".");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
			//! @todo: check, maybe it will be implemented in ACE in feature versions
			//! (workaround for data reading with proactor)
			COMMTIMEOUTS timeouts;
			timeouts.ReadIntervalTimeout = 1;
			timeouts.ReadTotalTimeoutMultiplier = 0;
			timeouts.ReadTotalTimeoutConstant = 0;
			timeouts.WriteTotalTimeoutMultiplier = 0;
			timeouts.WriteTotalTimeoutConstant = 0;
			if (!SetCommTimeouts(m_io.get_handle(), &timeouts)) {
				const Error error(errno);
				LogTracking("SerialConnection", "SerialConnection", __FILE__, __LINE__);
				WFormat message(L"Could not set device communication timeouts: \"%1% (%2%)\".");
				message % error.GetString().GetCStr() % error.GetErrorNo();
				throw ConnectionOpeningException(message.str().c_str());
			}
		}

		~SerialConnection() {
			LogTracking("SerialConnection", "~SerialConnection", __FILE__, __LINE__);
			m_io.close();
		}

	public:

		virtual AutoPtr<EndpointAddress> GetLocalAddress() const {
			return AutoPtr<EndpointAddress>();
		}

		virtual AutoPtr<EndpointAddress> GetRemoteAddress() const {
			return AutoPtr<EndpointAddress>();
		}

	protected:

		virtual IoHandleInfo GetIoHandle() {
			return IoHandleInfo(m_io.get_handle(), IoHandleInfo::TYPE_OTHER);
		}

	private:

		ACE_DEV_Connector m_connector;
		ACE_TTY_IO m_io;

	};

} } }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__OutcomingSerialPortConnection_hpp__0909171625
