/**************************************************************************
 *   Created: 2010/05/24 22:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ClientLib.hpp 1046 2010-11-02 12:07:07Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UpnpClientLib_hpp__1005242254
#define INCLUDED_FILE__TUNNELEX__UpnpClientLib_hpp__1005242254

#include "Dll.hpp"
#include <TunnelEx/String.hpp>
#include <TunnelEx/Exceptions.hpp>

namespace TunnelEx { namespace Mods { namespace Upnp {

	class ClientLib : private Helpers::Dll {

	public:

		class Exception : public TunnelEx::LocalException {
		public:
			Exception(const wchar_t *what) throw()
					: LocalException(what) {
				//...//
			}
			Exception(const Exception &rhs) throw()
					: LocalException(rhs) {
				//...//
			}
			virtual ~Exception() throw() {
				//...//
			}
			Exception & operator =(const Exception &rhs) throw() {
				LocalException::operator =(rhs);
				return *this;
			}
			virtual UniquePtr<LocalException> Clone() const {
				return UniquePtr<LocalException>(new Exception(*this));
			}
		};

		struct Urls {

			Urls() {
				//...//
			}
		
			~Urls() {
				dtor(impl);
			}
		
			UPNPUrls impl;
			boost::function<void(UPNPUrls &)> dtor;

		private:

			Urls(const Urls &);
			Urls & operator =(const Urls &);
		
		};

	public:

		ClientLib()
				: Dll("miniupnpc.dll") {
			//...//
		}

	public:

		boost::shared_ptr<UPNPDev> Discover() const {
			using namespace boost;
			typedef UPNPDev *(Proto)(int, const char *, const char *, int);
			UPNPDev *const devList = GetFunction<Proto>("upnpDiscover")(2000, 0, 0, 0);
			if (!devList) {
				throw Exception(L"No IGD UPnP device found");
			}
			return shared_ptr<UPNPDev>(
				devList,
				bind(&ClientLib::FreeDevList, this, _1));
		}

		int GetValidIgd(
					UPNPDev &devList,
					Urls &urls,
					IGDdatas &data,
					std::string &lanAddr)
				const {
			char lanAddrBuf[64];
			typedef int(Proto)(UPNPDev *, UPNPUrls *, IGDdatas *, char *, int);
			const int result = GetFunction<Proto>("UPNP_GetValidIGD")(
				&devList,
				&urls.impl,
				&data,
				lanAddrBuf,
				sizeof(lanAddrBuf));
			urls.dtor = bind(&ClientLib::FreeUrls, this, _1);
			lanAddr = lanAddrBuf;
			return result;
		}

		std::string GetExternalIPAddress(const Urls &urls, const IGDdatas &data) const {
			char externalIPAddress[16];
			typedef int(Proto)(const char *, const char *, char *);
			const int result = GetFunction<Proto>("UPNP_GetExternalIPAddress")(
				urls.impl.controlURL,
				data.first.servicetype,
				externalIPAddress);
			if (result != 0 || !externalIPAddress) {
				throw Exception(L"Could not get external address for UPnP device");
			}
			return externalIPAddress;
		}

		void AddPortMapping(
					const Urls &urls,
					const IGDdatas &data,
					const char *externalPort,
					const char *internalPort,
					const char *internalHost,
					const char *proto,
					const char *description) {
			typedef int(Proto)(
				const char *,
				const char *,
				const char *,
				const char *,
				const char *,
				const char *,
				const char *,
				const char *);
			const int result = GetFunction<Proto>("UPNP_AddPortMapping")(
				urls.impl.controlURL,
				data.first.servicetype,
				externalPort,
				internalPort,
				internalHost,
				description,
				proto,
				0);
			if (result != UPNPCOMMAND_SUCCESS) {
				WFormat message(
					L"Could not add new UPnP port mapping %1% %2% to %3%:%4% (%5%)");
				message
					% ConvertString<WString>(proto).GetCStr()
					% ConvertString<WString>(externalPort).GetCStr()
					% ConvertString<WString>(internalHost).GetCStr()
					% ConvertString<WString>(internalPort).GetCStr()
					% result;
				throw Exception(message.str().c_str());
			}
		}

		bool DeletePortMapping(
					const Urls &urls,
					const IGDdatas &data,
					const char *externalPort,
					const char *proto) {
			typedef int(Proto)(
				const char *,
				const char *,
				const char *,
				const char *,
				const char *);
			const int result = GetFunction<Proto>("UPNP_DeletePortMapping")(
				urls.impl.controlURL,
				data.first.servicetype,
				externalPort,
				proto,
				NULL);
			if (result == UPNPCOMMAND_SUCCESS) {
				return true;
			} else if (result == 714) {
				return false;
			} else {
				WFormat message(
					L"Could not delete UPnP port mapping for %1% port %2% (%3%)");
				message
					% ConvertString<WString>(proto).GetCStr()
					% ConvertString<WString>(externalPort).GetCStr()
					% result;
				throw Exception(message.str().c_str());
			}
		}
		
		bool GetGenericPortMappingEntry(
					const Urls &urls,
					const IGDdatas &data,
					size_t index,
					std::string &externalPortResult,
					std::string &internalHostResult,
					std::string &internalPortResult,
					std::string &protoResult,
					std::string &descriptionResult) {
			typedef int(Proto)(
				const char *,
				const char *,
				const char *,
				char *,
				char *,
				char *,
				char *,
				char *,
				char *,
				char *,
				char *);
			char indexStr[6];
			_snprintf(indexStr, 6, "%d", index);
			char internalHost[16];
			internalHost[0] = 0;
			char internalPort[6];
			internalPort[0] = 0;
			char externalPort[6];
			externalPort[0] = 0;
			char proto[4];
			proto[0] = 0;
			char description[80];
			description[0] = 0;
			char isEnabled[6];
			isEnabled[0] = 0;
			char remoteHost[64];
			remoteHost[0] = 0;
			char duration[16];
			duration[0] = 0;
			const int result = GetFunction<Proto>("UPNP_GetGenericPortMappingEntry")(
				urls.impl.controlURL,
				data.first.servicetype,
				indexStr,
				externalPort,
				internalHost,
				internalPort,
				proto,
				description,
				isEnabled,
				remoteHost,
				duration);
			if (result == UPNPCOMMAND_SUCCESS) {
				using namespace std;
				string externalPortStr = externalPort;
				string internalHostStr = internalHost;
				string internalPortStr = internalPort;
				string protoStr = proto;
				string descriptionStr = description;
				swap(externalPortStr, externalPortResult);
				swap(internalHostStr, internalHostResult);
				swap(internalPortStr, internalPortResult);
				swap(protoStr, protoResult);
				swap(descriptionStr, descriptionResult);
				return true;
			} else if (result == 713) {
				return false;
			} else {
				WFormat message(L"Could not request UPnP port mapping entry (%1%)");
				message % result;
				throw Exception(message.str().c_str());
			}
		}

		void FreeDevList(UPNPDev *devList) const {
			typedef void(Proto)(UPNPDev *devList);
			BOOST_ASSERT(devList);
			GetFunction<Proto>("freeUPNPDevlist")(devList);
		}

		void FreeUrls(UPNPUrls &urls) const {
			typedef void(Proto)(UPNPUrls *);
			GetFunction<Proto>("FreeUPNPUrls")(&urls);
		}

	};


} } }

#endif // INCLUDED_FILE__TUNNELEX__UpnpClientLib_hpp__1005242254
