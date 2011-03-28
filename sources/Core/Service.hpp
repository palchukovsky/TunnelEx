/**************************************************************************
 *   Created: 2010/06/02 22:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Service.hpp 949 2010-06-06 05:56:16Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Service_hpp__1006022200
#define INCLUDED_FILE__TUNNELEX__Service_hpp__1006022200

#include "Rule.hpp"
#include "Exceptions.hpp"
#include "Instance.hpp"
#include "SharedPtr.hpp"
#include "Api.h"

namespace TunnelEx {

	class TUNNELEX_CORE_API Service : public ::TunnelEx::Instance {

	public:

		explicit Service(::TunnelEx::SharedPtr<
					const ::TunnelEx::ServiceRule> rule,
					const ::TunnelEx::ServiceRule::Service &serviceInfo)
				throw(TunnelEx::EndpointException);
		virtual ~Service() throw();

	public:

		const ::TunnelEx::ServiceRule & GetRule() const;
		const ::TunnelEx::ServiceRule::Service & GetService() const;

		bool IsStarted() const throw();

	public:

		virtual void Start() throw(TunnelEx::EndpointException) = 0;
		virtual void Stop() throw() = 0;
		
		virtual void DoWork() throw(TunnelEx::EndpointException) = 0;

	protected:

		void SetStarted(bool isStarted) throw();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__Service_hpp__1006022200
