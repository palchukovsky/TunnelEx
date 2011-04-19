/**************************************************************************
 *   Created: 2008/07/22 6:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Instance_hpp__0807220652
#define INCLUDED_FILE__TUNNELEX__Instance_hpp__0807220652

#include "Api.h"

namespace TunnelEx {

	//! Object with unique runtime instance identifier.
	class TUNNELEX_CORE_API Instance {

	public:

		//! Unique, not persistent, object instance identifier.
		typedef unsigned long long Id;

	public:
		
		Instance();
		
	protected:

		~Instance() throw();

	public:

		//! Returns unique, not persistent, object instance identifier.
		Id GetInstanceId() const;

	private:

		Instance(const Instance &);
		const Instance & operator =(const Instance &);

	};

}

#endif // INCLUDED_FILE__TUNNELEX__Instance_hpp__0807220652
