/**************************************************************************
 *   Created: 2008/11/27 2:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeConnectionAcceptor_hpp__0811270230
#define INCLUDED_FILE__TUNNELEX__PipeConnectionAcceptor_hpp__0811270230

#include "PipeEndpointAddress.hpp"

#include "Core/Acceptor.hpp"
#include "Core/Exceptions.hpp"
#include "Core/Error.hpp"

namespace TunnelEx { namespace Mods { namespace Pipe {

	class PipeConnectionAcceptor : public Acceptor {
	
	public:
	
		explicit PipeConnectionAcceptor(
					const PipeEndpointAddress &address,
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress)
				: Acceptor(ruleEndpoint, ruleEndpointAddress) {
			if (m_acceptor.open(address.GetAcePipeAddr()) != 0) {
				const Error error(errno);
				std::wostringstream exception;
				exception
					<< "Could not open pipe for listening: \""
					<< error.GetString().GetCStr() << " (" << error.GetErrorNo() << ")\".";
				throw ConnectionOpeningException(exception.str().c_str());
			}
		}
	
		virtual ~PipeConnectionAcceptor() {
			try {
				const int result = m_acceptor.close();
				ACE_UNUSED_ARG(result);
				BOOST_ASSERT(result == 0);
			} catch (...) {
				BOOST_ASSERT(false);
			}
		}
	
	public:
	
		virtual UniquePtr<Connection> Accept();

		virtual bool TryToAttach() {
			return false;
		}

		virtual UniquePtr<EndpointAddress> GetLocalAddress() const {
			UniquePtr<PipeEndpointAddress> result(new PipeEndpointAddress);
			if (m_acceptor.get_local_addr(result->GetAcePipeAddr()) != 0) {
				const Error error(errno);
				WFormat exception(L"Could not get listening pipe path: %1% (%2%)");
				exception % error.GetString().GetCStr() % error.GetErrorNo();
				throw SystemException(exception.str().c_str());
			}
			return result;
		}

		virtual IoHandleInfo GetIoHandle() {
			return IoHandleInfo(m_acceptor.get_handle(), IoHandleInfo::TYPE_OTHER);
		}
		
	private:
	
		mutable ACE_SPIPE_Acceptor m_acceptor;
	
	};
	
} } }

#endif // INCLUDED_FILE__TUNNELEX__PipeConnectionAcceptor_hpp__0811270230