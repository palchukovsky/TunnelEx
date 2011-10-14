/**************************************************************************
 *   Created: 2008/11/27 2:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeConnection_hpp__0811270249
#define INCLUDED_FILE__TUNNELEX__PipeConnection_hpp__0811270249

#include "PipeEndpointAddress.hpp"

#include "Core/Connection.hpp"

namespace TunnelEx { namespace Mods { namespace Pipe {

	class PipeConnection : public TunnelEx::Connection {

	public:

		explicit PipeConnection(
					const RuleEndpoint &ruleEndpoint,
					SharedPtr<const EndpointAddress> ruleEndpointAddress,
					std::auto_ptr<ACE_SPIPE_Stream> stream)
				: Connection(ruleEndpoint, ruleEndpointAddress),
				m_stream(stream) {
			//...//
		}
		
		virtual ~PipeConnection() throw() {
			try {
				const int result = m_stream->close();
				ACE_UNUSED_ARG(result);
				assert(result == 0);
			} catch (...) {
				assert(false);
			}
		}
	
	public:

		virtual AutoPtr<EndpointAddress> GetLocalAddress(void) const {
			return AutoPtr<EndpointAddress>();
		}
		
		virtual AutoPtr<EndpointAddress> GetRemoteAddress(void) const {
			return AutoPtr<EndpointAddress>();
		}

	protected:

		virtual IoHandleInfo GetIoHandle() {
			return IoHandleInfo(m_stream->get_handle(), IoHandleInfo::TYPE_OTHER);
		}

	private:

		 std::auto_ptr<ACE_SPIPE_Stream> m_stream;

	};


} } }

#endif // INCLUDED_FILE__TUNNELEX__PipeConnection_hpp__0811270249
