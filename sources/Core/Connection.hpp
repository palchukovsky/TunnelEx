/**************************************************************************
 *   Created: 2007/03/01 1:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__Connection_h__0703010128
#define INCLUDED_FILE__Connection_h__0703010128

#include "DataTransferCommand.hpp"
#include "Instance.hpp"
#include "IoHandle.h"
#include "UniquePtr.hpp"
#include "SharedPtr.hpp"
#include "String.hpp"
#include "Time.h"
#include "Api.h"

namespace TunnelEx {

	class RuleEndpoint;
	class ConnectionSignal;
	class LogicalException;
	class ConnectionException;
	class ConnectionOpeningException;
	class MessageBlock;
	class EndpointAddress;

	//! Connection interface.
	/** @sa ::TunnelEx::Acceptor
	  */
	class TUNNELEX_CORE_API Connection : public ::TunnelEx::Instance {

	public:

		enum Mode {
			MODE_READ,
			MODE_WRITE,
			MODE_READ_WRITE
		};

	public:
		
		//! C-tor.
		explicit Connection(
				const ::TunnelEx::RuleEndpoint &ruleEndpoint,
				::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress> &ruleEndpointAddress);
		//! D-tor.
		virtual ~Connection() throw();

	public:

		//! Opens data transfer connection.
		/** @param dataTransferSignal	the signal for server notification,
		  *								connection should not use it in this
		  *								method as data listeners queue can be not
		  *								created at that moment;
		  * @param mode					the work mode;
		  * @throw TunnelEx::ConnectionException 
		  * @throw TunnelEx::LogicalException
		  */
		void Open(
				::TunnelEx::SharedPtr<::TunnelEx::ConnectionSignal> signal,
				Mode mode);

		//! Starts connection setup.
		/** @sa Setup
		  * @sa CompleteSetup
		  */
		void StartSetup();

		//! Starts read from connection.
		void StartReadRemote();

		//! Sends the message block to remote side.
		/** @param	messageBlock	the message block to send. Object 
		  *                         can change message block content.
		  * @throw TunnelEx::ConnectionException 
		  * @throw TunnelEx::LogicalException
		  * @return	command for further actions;
		  */
		::TunnelEx::DataTransferCommand SendToRemote(
				::TunnelEx::MessageBlock &messageBlock);

		//! Sends data to remote side.
		/** The internal buffer is not involved in the sending, it can lead
		  * to problems with memory or speed.
		  * @throw TunnelEx::ConnectionException 
		  * @throw TunnelEx::LogicalException 
		  */
		void SendToRemote(const char *data, size_t size);

		//! Sends the message block to tunnel.
		/** @param	messageBlock	the message block to send. Object 
		  *                         can change message block content.
		  * @return	command for further actions;
		  */
		void SendToTunnel(::TunnelEx::MessageBlock &messageBlock);

		//! Sends data into tunnel.
		/** The internal buffer is not involved in the sending, it can lead
		  * to problems with memory or speed.
		  */
		void SendToTunnel(const char *data, size_t size);

		//! Callback for data block sent event.
		void OnMessageBlockSent(const ::TunnelEx::MessageBlock &messageBlock);

		void SetForceClosingMode();

		bool IsSetupCompleted() const;

		bool IsSetupFailed() const;
			
	public:

		//! Returns true if connection does not exist for tunneling.
		virtual bool IsOneWay() const;

		//! Returns destination or source endpoint from rule.
		/** @sa	GetLocalAddress
		  * @sa	GetRemoteAddress
		  * @return	endpoint address from rule
		  */
		const ::TunnelEx::RuleEndpoint & GetRuleEndpoint() const;

		//! Returns destination or source rule endpoint address from rule.
		/** @sa	GetLocalAddress
		  * @sa	GetRemoteAddress
		  * @return	endpoint address from rule
		  */
		::TunnelEx::SharedPtr<const ::TunnelEx::EndpointAddress>
		GetRuleEndpointAddress()
			const;

		//! Returns address for local side of connection.
		/** Result can be nil for protocols that does not supports
		  * different addresses for rule endpoint address and real.
		  * @sa	GetRuleEndpoint
		  * @sa	GetRemoteAddress
		  * @return	address for local side of connection or nil
		  */
		virtual
		::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress>
		GetLocalAddress()
			const
			= 0;
		
		//! Returns address for remote side of connection.
		/** Result can be nil for protocols that does not supports
		  * different addresses for rule endpoint address and real.
		  * @sa	GetRuleEndpoint
		  * @sa	GetLocalAddress
		  * @return	address for remote side of connection or nil
		  */
		virtual
		::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress>
		GetRemoteAddress()
			const
			= 0;

	protected:

		//! Returns IO handle or zero if connection doesn't have own data stream.
		virtual ::TunnelEx::IoHandleInfo GetIoHandle() = 0;

		//! Internal setup implementation.
		/** Must call CompleteSetup after connection setup will be completed.
 		 * @sa StartSetup
		 * @sa CompleteSetup
		 */
		virtual void Setup();
		
		//! Must be called if connection has to be canceled.
		/** @sa Setup
		  * @sa CompleteSetup
		  * @sa StartSetup
		  */
		void CancelSetup(const ::TunnelEx::WString &reason);

		//! Stops read from connection.
		/** Works only for calls from Connection::ReadRemote! */ 
		void StopReadRemote();

		//! Reads data.
		virtual void ReadRemote(::TunnelEx::MessageBlock &messageBlock);

		//! Writes data.
		/** @throw TunnelEx::ConnectionException 
		  * @throw TunnelEx::LogicalException 
		  */
		virtual ::TunnelEx::DataTransferCommand Write(
				::TunnelEx::MessageBlock &messageBlock);

		//! Directly writes data, as is.
		/** @throw TunnelEx::ConnectionException 
		  * @throw TunnelEx::LogicalException 
		  */
		virtual ::TunnelEx::DataTransferCommand WriteDirectly(
				::TunnelEx::MessageBlock &messageBlock);

		//! Directly writes data, as is.
		/** @throw TunnelEx::ConnectionException 
		  * @throw TunnelEx::LogicalException 
		  */
		void WriteDirectly(const char *data, size_t size);

		//! Sets the connection idle time out.
		/** If idle more then @seconds Connection::OnTimeout will be called.
		  * Zero value disables timeout (default).
		  * @sa OnTimeout
		  */
		void ResetIdleTimeout(TimeSeconds seconds);

	private:

		//! Idle timeout hook.
		/** If it returns false - tunnel closes. If true - timer restarted.
		  * @sa SetIdleTimeout
		  */
		virtual bool OnIdleTimeout() throw();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

#endif // ifndef 