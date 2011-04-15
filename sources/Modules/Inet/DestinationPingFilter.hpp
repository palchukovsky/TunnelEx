/**************************************************************************
 *   Created: 2008/01/20 16:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: DestinationPingFilter.hpp 1008 2010-09-24 18:16:45Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__DestinationPingFilter_h__0801201639
#define INCLUDED_FILE__TUNNELEX__DestinationPingFilter_h__0801201639

#include "Core/Locking.hpp"
#include "Core/Filter.hpp"
#include "Core/String.hpp"

namespace TunnelEx {
	class RuleEndpoint;
}

namespace TunnelEx { namespace Mods { namespace Inet {

	class DestinationPingFilter : public Filter {

	public:

		class PingEndpoint;
		typedef std::list<PingEndpoint> Endpoints;

	public:

		DestinationPingFilter(SharedPtr<TunnelRule>, SharedPtr<RecursiveMutex>);
		virtual ~DestinationPingFilter() throw();

	public:
	
		virtual void ChangeRule(TunnelRule &);

	private:

		Endpoints m_endpoints;

		//! \todo: Maybe search be UUID (not by string) will be faster? [2008/01/22 12:08]
		typedef std::map<WString, Endpoints::iterator> EndpointUuids;
		EndpointUuids m_endpointUuids;

		class Thread;
		static Thread *m_thread;
		static unsigned int m_objectCount;

		bool m_isActive;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__DestinationPingFilter_h__0801201639
