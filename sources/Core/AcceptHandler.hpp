/**************************************************************************
 *   Created: 2008/07/12 10:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__AcceptHandler_hpp__0807121027
#define INCLUDED_FILE__TUNNELEX__AcceptHandler_hpp__0807121027

#include "Server.hpp"
#include "Exceptions.hpp"
#include "Log.hpp"
#include "IoHandle.h"
#include "SmartPtr.hpp"
#include "Error.hpp"

namespace TunnelEx {

	//! Handles incoming connections, accepts it and initiates new tunnel opening.
	template<class EndpointHandle>
	class AcceptHandler : public ACE_Event_Handler {

	public:

		typedef ACE_Event_Handler Base;

	public:

		static boost::shared_ptr<AcceptHandler<typename EndpointHandle> > CreateInstance(
				ServerWorker &server,
				EndpointHandle endpointHandle,
				const RuleEndpoint &endpoint,
				const SharedPtr<const EndpointAddress> address) {
			
			boost::shared_ptr<ACE_Barrier> dtorBarrier(new ACE_Barrier(2));
			AcceptHandler *const instance(new AcceptHandler(
				server,
				endpointHandle,
				endpoint,
				address,
				dtorBarrier));
			// 
			int registerHandlerResult
				= instance->m_server.GetReactor().register_handler(
					instance,
					GetEventsMask());
#			ifdef ACE_WIN32
				// Special case for pipe support on Windows (module Pipe, class ACE_SPIPE_Acceptor):
				//! @todo: !errno - added as workaround ACE in Vista, check in new ACE version and remove if posible [2008/12/25 21:46]
				if (registerHandlerResult != 0 && (errno == ENOSYS || !errno)) {
					registerHandlerResult = instance->m_server
						.GetReactor()
						.register_handler(instance, instance->get_handle());
				}
#			endif // ACE_WIN32
			if (registerHandlerResult != 0) {
				const Error error(errno);
				delete instance;
				WFormat message(L"Could not register accept event object: %1% (%2%)");
				message % error.GetStringW() % error.GetErrorNo();	
				throw ConnectionOpeningException(message.str().c_str());
			}

			typedef AcceptHandler<typename EndpointHandle> MyType;
			boost::shared_ptr<MyType> result(instance, &MyType::DeleteInstance);
			return result;

		}

		static void DeleteInstance(AcceptHandler *instance) {
			ACE_Reactor &reactor = instance->m_server.GetReactor();
			ACE_thread_t reactorOwner;
			reactor.owner(&reactorOwner);
			boost::shared_ptr<ACE_Barrier> barrier;
			if (ACE_OS::thr_self() == reactorOwner) {
				instance->m_dtorBarrier.reset();
			} else {
				barrier = instance->m_dtorBarrier;
			}
			const bool waitAcceptor
				= reactor.remove_handler(instance, GetEventsMask()) != -1;
			assert(waitAcceptor);
			if (waitAcceptor && barrier) {
				barrier->wait();
			}
		}

	private:

		explicit AcceptHandler(
					ServerWorker &server,
					EndpointHandle endpointHandle,
					const RuleEndpoint &endpoint,
					const SharedPtr<const EndpointAddress> address,
					boost::shared_ptr<ACE_Barrier> dtorBarrier)
				: m_server(server),
				m_endpointHandle(endpointHandle),
				m_address(address),
				m_acceptor(m_address->OpenForIncomingConnections(
					endpoint,
					m_address)),
				m_dtorBarrier(dtorBarrier) {
			const bool isSilent = m_endpointHandle->rule->IsSilent();
			if (
					(!isSilent && Log::GetInstance().IsInfoRegistrationOn())
					|| (isSilent && Log::GetInstance().IsDebugRegistrationOn())) {
				String buffer;
				std::ostringstream message;
				message << "Endpoint ";
				const AutoPtr<const EndpointAddress> acceptorLocalAddress
					= m_acceptor->GetLocalAddress();
				const WString &acceptorLocalAddressId
					= acceptorLocalAddress->GetResourceIdentifier();
				const WString &ruleEndpointId
					= m_address->GetResourceIdentifier();
				if (acceptorLocalAddressId != ruleEndpointId) {
					message
						<< "\""
						<< ConvertString(acceptorLocalAddressId, buffer).GetCStr()
						<< "\"";
					message					
						<< " ("
						<< ConvertString(ruleEndpointId, buffer).GetCStr()
						<< ")";
				} else {
					message					
						<< " \""
						<< ConvertString(ruleEndpointId, buffer).GetCStr()
						<< "\"";
				}
				message
					<< " opened for incoming connections - "
					<< m_acceptor->GetInstanceId();
				if (!isSilent) {
					Log::GetInstance().AppendInfo(message.str());
				} else {
					Log::GetInstance().AppendDebug(message.str());
				}
			}
			TUNNELEX_OBJECTS_DELETION_CHECK_CTOR(m_instancesNumber);
		}

		virtual ~AcceptHandler() {
			try {
				const bool isSilent = m_endpointHandle->rule->IsSilent();
				if (
						(!isSilent && Log::GetInstance().IsInfoRegistrationOn())
						|| (isSilent && Log::GetInstance().IsDebugRegistrationOn())) {
					String buffer;
					std::ostringstream message;
					message << "Closing incoming connections endpoint ";
					const AutoPtr<const EndpointAddress> acceptorLocalAddress
						= m_acceptor->GetLocalAddress();
					const WString &acceptorLocalAddressId
						= acceptorLocalAddress->GetResourceIdentifier();
					const WString &ruleEndpointId
						= !m_acceptor->GetRuleEndpoint().IsCombined()
							?	m_acceptor->GetRuleEndpoint().GetReadResourceIdentifier()
							:	m_acceptor->GetRuleEndpoint().GetCombinedResourceIdentifier();
					if (acceptorLocalAddressId != ruleEndpointId) {
						message
							<< "\""
							<< ConvertString(acceptorLocalAddressId, buffer).GetCStr()
							<< "\"";
						message					
							<< " ("
							<< ConvertString(ruleEndpointId, buffer).GetCStr()
							<< ")";
					} else {
						message					
							<< " \""
							<< ConvertString(ruleEndpointId, buffer).GetCStr()
							<< "\"";
					}
					message
						<< " - "
						<< m_acceptor->GetInstanceId();
					if (!isSilent) {
						Log::GetInstance().AppendInfo(message.str());
					} else {
						Log::GetInstance().AppendDebug(message.str());
					}
				}
				m_acceptor.Reset();
				if (m_dtorBarrier) {
					m_dtorBarrier->wait();
				}
			} catch (...)  {
				Format message(
					"Unknown system error occurred: %1%:%2%."
						" Please restart the service"
						" and contact product support to resolve this issue."
						" %3% %4%");
				message
					% __FILE__ % __LINE__
					% TUNNELEX_NAME % TUNNELEX_BUILD_IDENTITY;
				Log::GetInstance().AppendFatalError(message.str());
				assert(false);
			}
			TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(m_instancesNumber);
		}

	private:

		AcceptHandler(const AcceptHandler &);
		const AcceptHandler & operator =(const AcceptHandler &);

	public:

		//! Return the passive-mode socket's I/O handle.
		virtual ACE_HANDLE get_handle() const {
			return m_acceptor->GetIoHandle().handle;
		}

	public:

		const Acceptor & GetAcceptor() const {
			return *m_acceptor;
		}

	public:

		//! Called by a reactor when there's a new connection to accept.
		virtual int handle_input(ACE_HANDLE = ACE_INVALID_HANDLE) {
			OpenTunnel();
			return 0;
		}

#		ifdef ACE_WIN32
			//! Method created only for Windows-pipes.
			virtual int handle_signal(int, siginfo_t *, ucontext_t *) {
				Log::GetInstance().AppendDebug(
					"Incoming connection signal detected, initializing tunnel...");
				OpenTunnel();
				return 0;
			}
#		endif // ACE_WIN32

		//! Called when this object is destroyed, e.g., when it's
		//! removed from a reactor.
		virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask) {
			delete this;
			return 0;
		}
		
	private:

		static ACE_Reactor_Mask GetEventsMask() {
			return ACE_Event_Handler::ACCEPT_MASK | ACE_Event_Handler::READ_MASK;
		}

	private:
	
		bool OpenTunnel() {
			try {
				m_server.OpenTunnel(m_endpointHandle, *m_acceptor);
				return true;
			} catch (const TunnelEx::ConnectionOpeningGracefullyCanceled &ex) {
				// see TEX-698
				Log::GetInstance().AppendDebugEx(
					[&ex]() -> Format {
						Format message(
							"False alarm, no connections accepted: \"%1%\".");
						message % WString(ex.GetWhat());
						return message;
					});
			} catch (const TunnelEx::LocalException &ex) {
				Log::GetInstance().AppendError(
					ConvertString<String>(ex.GetWhat()).GetCStr());
			} catch (const std::exception &ex) {
				Log::GetInstance().AppendError(ex.what());
				throw;
			} catch (...) {
				Log::GetInstance().AppendSystemError(
					"Unknown system error occurred in the connection accepting process.");
				throw;
			}
			return false;
		}

	private:

		ServerWorker &m_server;
		const EndpointHandle m_endpointHandle;
		const SharedPtr<const EndpointAddress> m_address;
		AutoPtr<Acceptor> m_acceptor;
		boost::shared_ptr<ACE_Barrier> m_dtorBarrier;
		
		TUNNELEX_OBJECTS_DELETION_CHECK_DECLARATION(m_instancesNumber);

	};
	
	TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION_TEMPLATE(
		AcceptHandler,
		m_instancesNumber,
		EndpointHandle);

}

#endif // INCLUDED_FILE__TUNNELEX__AcceptHandler_hpp__0807121027
