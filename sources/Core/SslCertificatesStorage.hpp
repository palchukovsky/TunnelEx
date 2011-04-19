/**************************************************************************
 *   Created: 2010/11/06 16:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__SslCertificatesStorage_hpp__1011061620
#define INCLUDED_FILE__TUNNELEX__SslCertificatesStorage_hpp__1011061620

#include "Collection.hpp"
#include "UniquePtr.hpp"
#include "Api.h"

namespace TunnelEx { namespace Helpers { namespace Crypto {
	class X509Shared;
	class X509Private;
} } }

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	//! SSL certificate.
	typedef ::TunnelEx::WString SslCertificateId;

	//! Collection of SSL certificates.
	typedef ::TunnelEx::Collection<::TunnelEx::SslCertificateId>
		SslCertificateIdCollection;

	//////////////////////////////////////////////////////////////////////////

	class TUNNELEX_CORE_API SslCertificatesStorage {

	public:

		SslCertificatesStorage(
				const ::TunnelEx::WString &certificatesFsStoragePath,
				const unsigned char *,
				const size_t);
		~SslCertificatesStorage() throw();

	public:
		
		void Insert(const ::TunnelEx::Helpers::Crypto::X509Shared &);
		void Insert(const ::TunnelEx::Helpers::Crypto::X509Private &);
		//! Deletes certificates from storage.
		//! If certificate was not found no error or exception will be returned.
		void Delete(const ::TunnelEx::SslCertificateIdCollection &ids);
		void Delete(const ::TunnelEx::WString &id);

		::TunnelEx::UniquePtr<::TunnelEx::SslCertificateIdCollection> GetInstalledIds()
				const;

		//! Returns a certificate.
		/** @param certificateId	local certificate ID
		  * @throw	TunnelEx::NotFoundException
		  */
		::TunnelEx::UniquePtr<::TunnelEx::Helpers::Crypto::X509Shared> GetCertificate(
					const ::TunnelEx::WString &certificateId)
				const;

		//! Returns a TRUE if certificate private (has private key)
		/** @param certificateId	local certificate ID
		  * @throw	TunnelEx::NotFoundException
		  */
		bool IsPrivateCertificate(
				const ::TunnelEx::WString &certificateId)
			const;

		//! Returns a private certificate copy.
		/** @param certificateId	local certificate ID
		  * @throw	TunnelEx::NotFoundException
		  */
		::TunnelEx::UniquePtr<::TunnelEx::Helpers::Crypto::X509Private>
		GetPrivateCertificate(
					const ::TunnelEx::WString &certificateId)
				const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__SslCertificatesStorage_hpp__1011061620
