/**************************************************************************
 *   Created: 2010/03/28 16:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ProxyExceptions.hpp 942 2010-05-29 20:46:50Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ProxyExceptions_hpp__1003281609
#define INCLUDED_FILE__TUNNELEX__ProxyExceptions_hpp__1003281609

#include "Api.h"
#include "Core/Exceptions.hpp"

namespace TunnelEx { namespace Mods { namespace Inet {

	class TUNNELEX_MOD_INET_API ProxyWorkingException
		: public TunnelEx::ConnectionOpeningException {
	public:
		explicit ProxyWorkingException(const wchar_t *what) throw();
		ProxyWorkingException(const ProxyWorkingException &) throw();
		virtual ~ProxyWorkingException() throw();
		const ProxyWorkingException & operator =(const ProxyWorkingException &) throw();
		virtual TunnelEx::UniquePtr<TunnelEx::LocalException> Clone() const;
	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__ProxyExceptions_hpp__1003281609
