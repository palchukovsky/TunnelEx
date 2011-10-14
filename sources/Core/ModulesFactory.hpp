/**************************************************************************
 *   Created: 2007/03/28 7:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__PluginsFactory_h__0703280731
#define INCLUDED_FILE__PluginsFactory_h__0703280731

#include "Singleton.hpp"
#include "String.hpp"
#include "SmartPtr.hpp"
#include "Server.hpp"

namespace TunnelEx {

	class TunnelRule;
	class ServiceRule;
	class EndpointAddress;
	class Connection;
	class Service;
	class RecursiveMutex;
	class PreListener;
	class PostListener;
	class Filter;

	namespace Singletons {

		//! A modules factory class. 
		class ModulesFactoryPolicy : private boost::noncopyable {

			template<typename T, template<class> class L, template<class> class Th>
			friend class Holder;
			
		private:

			ModulesFactoryPolicy();
			~ModulesFactoryPolicy();

		public:

			//! Creates filters and attaches rules objects to it.
			/** @sa Filter
			  * @param rule					the reference to rule object,
			  *								this reference will be attached
			  *								to the filter object and should
			  *								be valid all filter's live;
			  * @param ruleChangingMutex	the mutex for rule changes locking;
			  * @param filters				the result;
			  */
			void CreateFilters(
					SharedPtr<TunnelRule> rule,
					SharedPtr<RecursiveMutex> ruleChangingMutex,
					std::vector<SharedPtr<Filter> > &filters);

			void CreatePreListeners(
					Server::Ref,
					const TunnelRule &,
					const Connection &currentConnection,
					const Connection &oppositeConnection,
					boost::function<void(SharedPtr<PreListener>)> preListenersReceiveCallback);
				
			void CreatePostListeners(
					Server::Ref,
					const TunnelRule &,
					const Connection &currentConnection,
					const Connection &oppositeConnection,
					boost::function<void(SharedPtr<PostListener>)> postListenersReceiveCallback);

			/**
			  * @throw TunnelEx::InvalidLinkException
			  */
			AutoPtr<EndpointAddress> CreateEndpointAddress(
					const WString &resourceIdentifier);

			/** 
			  * @throw TunnelEx::InvalidLinkException
			  * @throw TunnelEx::LocalException
			  */
			AutoPtr<Service> CreateService(
					TunnelEx::SharedPtr<const ServiceRule>,
					const ServiceRule::Service &);

		private:

			class Implementation;
			Implementation *m_pimpl;

		};

	}

	//! The modules factory. Is a singleton.
	typedef ::TunnelEx::Singletons::Holder<
			::TunnelEx::Singletons::ModulesFactoryPolicy,
			::TunnelEx::Singletons::DefaultLifetime>
		ModulesFactory;

}

#endif // INCLUDED_FILE__PluginsFactory_h__0703280731
