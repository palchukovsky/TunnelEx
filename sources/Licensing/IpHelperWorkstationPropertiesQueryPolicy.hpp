/**************************************************************************
 *   Created: 2009/12/06 11:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: IpHelperWorkstationPropertiesQueryPolicy.hpp 1072 2010-11-25 20:02:26Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__IpHelperWorkstationPropertiesQueryPolicy_hpp__0912061142
#define INCLUDED_FILE__TUNNELEX__IpHelperWorkstationPropertiesQueryPolicy_hpp__0912061142

#include "Types.hpp"

namespace TunnelEx { namespace Licensing {

	template<typename ClientTrait, bool isTestMode>
	struct WorkstationPropertiesLocalPolicy {

		inline static bool Get(
					WorkstationPropertyValues &result,
					const boost::any &) {
		
			// IPHLPAPI.lib
			
			using namespace TunnelEx::Helpers;
			using namespace TunnelEx::Helpers::Crypto;
			
			WorkstationPropertyValues props;

			{
				OSVERSIONINFOEX verInfo;
				ZeroMemory(&verInfo, sizeof(OSVERSIONINFOEX));
				verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
				if (GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&verInfo))) {
					std::ostringstream oss;
					oss
						<< verInfo.dwMajorVersion << " " << verInfo.dwMinorVersion
						<< " " << verInfo.dwBuildNumber << " " << verInfo.dwPlatformId
						<< " " << verInfo.wProductType;
					props[WORKSTATION_PROPERTY_OS_VER]
						= DigestSha1(oss.str()).GetAscii();
				} else {
					BOOST_ASSERT(false);
					props[WORKSTATION_PROPERTY_OS_VER] = std::string();
				}
			}

			{
				std::vector<char> windowsDir(MAX_PATH, 0);
				if (GetWindowsDirectoryA(&windowsDir[0], UINT(windowsDir.size()))) {
					windowsDir[3] = 0;
					DWORD serialNumber;
					if (GetVolumeInformationA(&windowsDir[0], 0, 0, &serialNumber, 0, 0, 0, 0)) {
						std::ostringstream oss;
						oss << serialNumber;
						props[WORKSTATION_PROPERTY_OS_VOLUME]
							= DigestSha1(oss.str()).GetAscii();
					} else {
						BOOST_ASSERT(false);
					}
				} else {
					BOOST_ASSERT(false);
				}
			}

			{
				std::vector<unsigned char> adaptersInfo(sizeof(IP_ADAPTER_INFO));
				ULONG adaptersInfoBufferLen = ULONG(adaptersInfo.size());
				if (	GetAdaptersInfo(
							reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]),
							&adaptersInfoBufferLen)
						== ERROR_BUFFER_OVERFLOW) {
					adaptersInfo.resize(adaptersInfoBufferLen);
				}
				std::string adapterDigests;
				if (	GetAdaptersInfo(
							reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]),
							&adaptersInfoBufferLen)
						== NO_ERROR) {
					PIP_ADAPTER_INFO adapter
						= reinterpret_cast<PIP_ADAPTER_INFO>(&adaptersInfo[0]);
					while (adapter) {
						switch (adapter->Type) {
							case MIB_IF_TYPE_ETHERNET:
							case MIB_IF_TYPE_TOKENRING:
							case IF_TYPE_IEEE80211:
							case IF_TYPE_IEEE80212:
							case IF_TYPE_FASTETHER:
							case IF_TYPE_FASTETHER_FX:
								{
									std::ostringstream oss;
									oss
										<< adapter->AdapterName
										<< " "
										<< StringUtil::BinToAscii(adapter->Address, adapter->AddressLength);
									adapterDigests
										+= DigestSha1(oss.str()).GetAscii();
								}
								break;
						}
						adapter = adapter->Next;
					}
				}
				BOOST_ASSERT(!adapterDigests.empty());
				if (!adapterDigests.empty()) {
					props[WORKSTATION_PROPERTY_ADAPTER] = adapterDigests;
				}
			}

			{
				boost::format path("%3%\\%1%\\%4% %2%\\%5%");
				path % "Microsoft" % "NT" % "SOFTWARE" % "Windows" % "CurrentVersion";
				HKEY key;
				LONG status
					= RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.str().c_str(), 0, KEY_READ, &key);
				BOOST_ASSERT(status == ERROR_SUCCESS);
				if (status == ERROR_SUCCESS) {
					//! @todo: replace with unique_ptr [2009/11/10 22:56]
					boost::shared_ptr<HKEY__> autoKey(key, &RegCloseKey);
					DWORD start = 0;
					std::vector<BYTE> buffer;
					DWORD dataType;
					const char *const items[] = {
						"BuildGUID",
						"DigitalProductId",
						"InstallDate",
						"ProductId",
						"SystemRoot",
						"ProductName",
						"PathName"};
					foreach (const char *const item, items) {
						DWORD bufferSize = DWORD(buffer.size()) - start;
						if (bufferSize < 255) {
							buffer.resize(buffer.size() + (255 - bufferSize));
							bufferSize = DWORD(buffer.size()) - start;
						}
						status = RegQueryValueExA(key, item, NULL, &dataType, &buffer[0], &bufferSize);
						BOOST_ASSERT(
							status == ERROR_SUCCESS
							|| (status == ERROR_FILE_NOT_FOUND && std::string(item) == "BuildGUID"));
						if (status == ERROR_SUCCESS) {
							start += bufferSize;
						}
					}
					props[WORKSTATION_PROPERTY_INSTALLATION_INFO] = !buffer.empty()
						?	DigestSha1(buffer).GetAscii()
						:	std::string();
				}
			}

			props.swap(result);
			return true;

		}

	};

} }

#endif // INCLUDED_FILE__TUNNELEX__IpHelperWorkstationPropertiesQueryPolicy_hpp__0912061142
