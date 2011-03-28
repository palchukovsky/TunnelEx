/**************************************************************************
 *   Created: 2008/01/22 11:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Uuid.hpp 1079 2010-12-01 05:19:27Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Uuid_h__0801221110
#define INCLUDED_FILE__TUNNELEX__Uuid_h__0801221110

#ifndef _WINDOWS
#	include "CompileWarningsAce.h"
#		include <ace/UUID.h>
#	include "CompileWarningsAce.h"
#	include <locale>
#	include <memory>
#else
#	include "CompileWarningsBoost.h"
#		include <boost/shared_ptr.hpp>
#		include <boost/algorithm/string.hpp>
#	include "CompileWarningsBoost.h"
#	include <Rpc.h>
#	include <RpcDce.h>
#	include <exception>
#	pragma comment(lib, "Rpcrt4.lib")
#endif
#include <string>

namespace TunnelEx { namespace Helpers {

	class Uuid {

	public:
		
#		ifndef _WINDOWS

			Uuid() {
				using namespace std;
				auto_ptr<ACE_Utils::UUID> impl(ACE_Utils::UUID_Generator().generate_UUID());
				const char *pch = impl->to_string()->c_str();
				const locale locale;
				while (*pch) {
					m_uuid.push_back(toupper<char>(*pch++, locale));
				}
			}

#		else 

			Uuid() {
				UUID id;
				if (UuidCreate(&id) == RPC_S_OK) {
					unsigned short *uuidStrPtr;
					if (UuidToStringW(&id, &uuidStrPtr) == RPC_S_OK) {
						boost::shared_ptr<unsigned short *> uuidStrSmartPtr(&uuidStrPtr, &RpcStringFreeW);
						m_uuid = reinterpret_cast<std::wstring::value_type *>(uuidStrPtr);
					}
				}
				if (m_uuid.empty()) {
					throw std::exception("Could not generate new UUID");
				}
				boost::to_upper(m_uuid);
			}

#		endif

	public:

		const std::wstring & GetAsString() const {
			return m_uuid;
		}

	private:
		
		std::wstring m_uuid;

	};

} }

#endif // INCLUDED_FILE__TUNNELEX__Uuid_h__0801221110
