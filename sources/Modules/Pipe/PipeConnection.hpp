/**************************************************************************
 *   Created: 2008/11/27 2:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: PipeConnection.hpp 1131 2011-02-22 21:13:12Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PipeConnection_hpp__0811270249
#define INCLUDED_FILE__TUNNELEX__PipeConnection_hpp__0811270249

#include "PipeEndpointAddress.hpp"

#include <TunnelEx/Connection.hpp>

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
				BOOST_ASSERT(result == 0);
			} catch (...) {
				BOOST_ASSERT(false);
			}
		}
	
	public:

		virtual UniquePtr<EndpointAddress> GetLocalAddress(void) const {
			return UniquePtr<EndpointAddress>();
		}
		
		virtual UniquePtr<EndpointAddress> GetRemoteAddress(void) const {
			return UniquePtr<EndpointAddress>();
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
