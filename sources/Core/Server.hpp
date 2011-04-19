/**************************************************************************
 *   Created: 2007/02/09 0:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE_Server_h__0702110532
#define INCLUDED_FILE_Server_h__0702110532

#include "Rule.hpp"
#include "String.hpp"
#include "Singleton.hpp"
#include "Time.h"
#include "Api.h"

namespace TunnelEx {

	class TunnelCollection;
	class LogicalException;
	class ConnectionOpeningException;
	class SslCertificatesStorage;

	namespace Singletons {

		//! A server class. 
		/* To get instance please use the Server-singleton. */
		//! \todo: Remove server from singletons. User can create many servers for one process.
		class TUNNELEX_CORE_API ServerPolicy {

			template<typename T, template<class> class L, template<class> class Th>
			friend class ::TunnelEx::Singletons::Holder;

		private:
			
			ServerPolicy();
			~ServerPolicy() throw();
			ServerPolicy(const ServerPolicy &);
			const ServerPolicy & operator =(const ServerPolicy &);

		public:
			
			//! Runs server.
			/* Throws an exception if server started. */
			void Start(
					const ::TunnelEx::RuleSet &,
					const ::TunnelEx::SslCertificatesStorage &);
			//! Returns true if server is started.
			bool IsStarted() const;
			//! Stops server.
			/* Throws an exception if server doesn't started. */
			void Stop();

			::TunnelEx::UniquePtr<EndpointAddress> GetRealOpenedEndpointAddress(
					const ::TunnelEx::WString &ruleUuid,
					const ::TunnelEx::WString &endpointUuid)
				const ;
					
			//! @todo: temp-method remove when tunnel collection will be implemented [2008/09/10 0:28]
			size_t GetTunnelsNumber() const;
			size_t GetOpenedEndpointsNumber() const;

			bool UpdateRule(const ::TunnelEx::ServiceRule &);
			bool UpdateRule(const ::TunnelEx::TunnelRule &);
			//! Deletes a rule from active list and closes input endpoints for it.
			/** @param	uuid	the rule's uuid to delete;
			  *	@return	true if rule was found and deleted, false otherwise;
			  */
			bool DeleteRule(const ::TunnelEx::WString &uuid);

			bool IsRuleEnabled(const ::TunnelEx::WString &uuid) const;

			const SslCertificatesStorage & GetCertificatesStorage() const;

			const ::TunnelEx::String & GetName() const;

		private:
			
			class Implementation;
			Implementation *m_pimpl;

		};

	}

	//! The server. Is a singleton.
	typedef ::TunnelEx::Singletons::Holder<
		::TunnelEx::Singletons::ServerPolicy,
		::TunnelEx::Singletons::DefaultLifetime> Server;

}

#endif // INCLUDED_FILE_Server_h__0702110532
