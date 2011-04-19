/**************************************************************************
 *   Created: 2010/12/19 19:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServiceFilesSecurity_h__1012191955
#define INCLUDED_FILE__TUNNELEX__ServiceFilesSecurity_h__1012191955

/* include into pre-compiler header:
#include "Format.hpp"
#include "CompileWarningsBoost.h"
#	include <boost/filesystem/path.hpp>
#include "CompileWarningsBoost.h"
#include <Aclapi.h>
#include <string> */

#include "Core/Error.hpp"
#include "Core/Log.hpp"
#include "Core/String.hpp"

namespace TunnelEx { namespace Helpers {

	struct ServiceFilesSecurity {

		//! Sets security for service files.
		static void Set(const boost::filesystem::wpath &file) {
			
			// implementation from http://www.rsdn.ru/forum/winapi/23876.flat.aspx#23876
			// original method for registry key, reimplemented for file

			SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
			PSID sidSystem = NULL;
			PSID sidAdmins = NULL;
			PACL dacl = NULL;

			const ULONG accessNum = 2;
			EXPLICIT_ACCESS explicitAccess[accessNum];

			ULONG result = ERROR_SUCCESS;

			for ( ; ; ) {

				// create SYSTEM SID
				const BOOL sysSidAllocResult
					= AllocateAndInitializeSid(
						&sia,
						1,
						SECURITY_LOCAL_SYSTEM_RID,
						0, 0, 0, 0, 0, 0, 0,
						&sidSystem);
				if (!sysSidAllocResult) {
					result = GetLastError();
					break;
				}

				// create Local Administrators alias SID
				const BOOL localAdminSidAllocResult
					= AllocateAndInitializeSid(
						&sia,
						2,
						SECURITY_BUILTIN_DOMAIN_RID,
						DOMAIN_ALIAS_RID_ADMINS,
						0, 0, 0, 0, 0, 0,
						&sidAdmins);
				if (!localAdminSidAllocResult) {
					result = GetLastError();
					break;
				}

				// fill an entry for the SYSTEM account
				explicitAccess[0].grfAccessMode = GRANT_ACCESS;
				explicitAccess[0].grfAccessPermissions = FILE_ALL_ACCESS;
				explicitAccess[0].grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
				explicitAccess[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
				explicitAccess[0].Trustee.pMultipleTrustee = NULL;
				explicitAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
				explicitAccess[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
				explicitAccess[0].Trustee.ptstrName = (LPTSTR)sidSystem;

				// fill an entry entries for the Administrators alias
				explicitAccess[1].grfAccessMode = GRANT_ACCESS;
				explicitAccess[1].grfAccessPermissions = FILE_ALL_ACCESS;
				explicitAccess[1].grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
				explicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
				explicitAccess[1].Trustee.pMultipleTrustee = NULL;
				explicitAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
				explicitAccess[1].Trustee.TrusteeType = TRUSTEE_IS_ALIAS;
				explicitAccess[1].Trustee.ptstrName = (LPTSTR)sidAdmins;

				// create a DACL
				result = SetEntriesInAcl(accessNum, explicitAccess, NULL, &dacl);
				if (result != ERROR_SUCCESS) {
					break;
				}

				result = SetNamedSecurityInfoW(
					const_cast<wchar_t *>(file.string().c_str()),
					SE_FILE_OBJECT,
					DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
					0,
					0,
					dacl,
					0);

				break;

			}
			
			if (sidSystem != NULL) {
				FreeSid(sidSystem);
			}
			if (sidAdmins != NULL) {
				FreeSid(sidAdmins);
			}
			if (dacl != NULL) {
				LocalFree((HLOCAL)dacl);
			}

			Error error(result);
			if (error.IsError()) {
				Format message("Could not apply security settings for \"%1%\": %2%.");
				message % ConvertString<String>(file.string().c_str()).GetCStr();
				message % ConvertString<String>(error.GetString()).GetCStr();
				Log::GetInstance().AppendWarn(message.str());
			}

		}

	};

} }

#endif // INCLUDED_FILE__TUNNELEX__ServiceFilesSecurity_h__1012191955
