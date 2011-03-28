/**************************************************************************
 *   Created: 2008/07/31 0:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ServerWorker.hpp 982 2010-09-05 19:01:12Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServerWorker_hpp__0807310006
#define INCLUDED_FILE__TUNNELEX__ServerWorker_hpp__0807310006

#include "String.hpp"
#include "UniquePtr.hpp"
#include "Server.hpp"
#include "Instance.hpp"
#include "IoHandle.h"

class ACE_Proactor;
class ACE_Reactor;

namespace TunnelEx {

	class TunnelRule;
	class TunnelRuleSet;
	class Tunnel;
	class Acceptor;
	class EndpointAddress;
	class LogicalException;
	class ConnectionOpeningException;
	class MessageBlock;

	class ServerWorker : private boost::noncopyable {

		template<class EndpointHandle>
		friend class AcceptHandler;

	public:

		class RuleInfo;

	private:

		template<class Base>
		class ConnectionOpeningExceptionImpl;

		class LicenseException;

	public:

		explicit ServerWorker(Server::Ref);
		~ServerWorker();

	public:

		size_t GetOpenedEndpointsNumber() const;
		//! @todo: temp-method remove when tunnel collection will be implemented [2008/09/10 0:28]
		size_t GetTunnelsNumber() const;

		UniquePtr<EndpointAddress> GetRealOpenedEndpointAddress(
				const WString &ruleUuid,
				const WString &endpointUuid)
			const throw(LogicalException, ConnectionOpeningException);

		bool Update(const RuleSet &rules);
		bool Update(const ServiceRule &rule);
		bool Update(const TunnelRule &rule);
		bool DeleteRule(const WString &ruleUuid);

		bool IsRuleEnabled(const ::TunnelEx::WString &uuid) const;

		void OpenTunnel(
				boost::shared_ptr<RuleInfo> ruleInfo,
				Acceptor &incomingConnectionAcceptor);
		void CloseTunnel(Instance::Id tunnelId, bool forceClosing);

		Server::Ref GetServer();

	public:

		ACE_Reactor & GetReactor();
		ACE_Proactor & GetProactor();

	private:

		class Implementation;
		std::auto_ptr<Implementation> m_pimpl;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__ServerWorker_hpp__0807310006
