/**************************************************************************
 *   Created: 2008/10/23 10:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__Error_hpp__0810231028
#define INCLUDED_FILE__Error_hpp__0810231028

#include "Api.h"
#include "String.hpp"

namespace TunnelEx {

	class TUNNELEX_CORE_API Error {

	public:

		explicit Error(int errorNo) throw();
		~Error() throw();

	public:

		::TunnelEx::WString GetString() const;
		int GetErrorNo() const;

		bool IsError() const;

		//! Returns true if error could be resolved to string.
		bool CheckError() const;

	private:

		int m_errorNo;

	};

}

#endif // INCLUDED_FILE__ErrorString_hpp__0810231028
