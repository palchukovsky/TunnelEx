/**************************************************************************
 *   Created: 2010/05/24 22:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Client.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Client_hpp__1005242258
#define INCLUDED_FILE__TUNNELEX__Client_hpp__1005242258

#include "Api.h"
#include <TunnelEx/Exceptions.hpp>

namespace TunnelEx { namespace Mods { namespace Upnp {

	class TUNNELEX_MOD_UPNP_API Client {

	public:

		enum Proto {
			PROTO_TCP,
			PROTO_UDP
		};

		class Exception : public TunnelEx::LocalException {
		public:
			explicit Exception(const wchar_t *) throw();
			Exception(const Exception &) throw();
			virtual ~Exception() throw();
			const Exception & operator =(const Exception &) throw();
			virtual UniquePtr<LocalException> Clone() const;
		};

		class DeviceNotExistException : public Exception {
		public:
			explicit DeviceNotExistException(const wchar_t *) throw();
			DeviceNotExistException(const DeviceNotExistException &) throw();
			virtual ~DeviceNotExistException() throw();
			const DeviceNotExistException & operator =(
					const DeviceNotExistException &)
				throw();
			virtual UniquePtr<LocalException> Clone() const;
		};

	public:

		Client();
		~Client() throw();

	private:

		Client(const Client &);
		const Client & operator =(const Client &);

	public:

		std::string GetExternalIpAddress() const;
		const std::string & GetLocalIpAddress() const;

		void AddPortMapping(
				const std::string &externalPort,
				const std::string &internalHost,
				const std::string &internalPort,
				Proto,
				const std::string &id,
				bool force)
			throw(TunnelEx::LocalException);
		bool DeletePortMapping(const std::string &id) throw();

		bool CheckMapping(
				const std::string &externalPort,
				const std::string &internalHost,
				const std::string &internalPort,
				Proto,
				const std::string &id)
			const
			throw(TunnelEx::LocalException);

	private:

		const char * ProtoToStr(Proto) const throw();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__Client_hpp__1005242258
