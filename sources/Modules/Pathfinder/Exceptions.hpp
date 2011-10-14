/**************************************************************************
 *   Created: 2010/03/28 21:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Exceptions_hpp__1003282108
#define INCLUDED_FILE__TUNNELEX__Exceptions_hpp__1003282108

#include "Core/Exceptions.hpp"

namespace TunnelEx { namespace Mods { namespace Pathfinder {

	//////////////////////////////////////////////////////////////////////////

	class ServiceException
		: public TunnelEx::ConnectionOpeningException {
	
	public:
	
		explicit ServiceException(const wchar_t *what) throw()
				: ConnectionOpeningException(what) {
			//...//
		}

		ServiceException(const ServiceException &rhs) throw()
				: ConnectionOpeningException(rhs) {
			//...//
		}

		virtual ~ServiceException() throw() {
			//...//
		}

		const ServiceException & operator =(const ServiceException &rhs) throw() {
			ConnectionOpeningException::operator =(rhs);
			return *this;
		}

		virtual TunnelEx::AutoPtr<TunnelEx::LocalException> Clone() const {
			return AutoPtr<LocalException>(new ServiceException(*this));
		}

	};

	//////////////////////////////////////////////////////////////////////////

	class LicensingException : public ServiceException {
	
	public:
	
		explicit LicensingException(const wchar_t *what) throw()
				: ServiceException(what) {
			//...//
		}

		LicensingException(const LicensingException &rhs) throw()
				: ServiceException(rhs) {
			//...//
		}

		virtual ~LicensingException() throw() {
			//...//
		}

		const LicensingException & operator =(const LicensingException &rhs) throw() {
			ServiceException::operator =(rhs);
			return *this;
		}

		virtual TunnelEx::AutoPtr<TunnelEx::LocalException> Clone() const {
			return AutoPtr<LocalException>(new LicensingException(*this));
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } }

#endif // INCLUDED_FILE__TUNNELEX__Exceptions_hpp__1003282108
