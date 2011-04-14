/**************************************************************************
 *   Created: 2008/10/23 10:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Error.hpp 1086 2010-12-07 08:53:15Z palchukovsky $
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

		//! Returns true if error could be resolved to std::string.
		bool CheckError() const;

	private:

		int m_errorNo;

	};

}

#endif // INCLUDED_FILE__ErrorString_hpp__0810231028
