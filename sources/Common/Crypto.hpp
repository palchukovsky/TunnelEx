/**************************************************************************
 *   Created: 2009/10/23 21:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Crypto.hpp 1118 2010-12-30 23:16:18Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Crypto_hpp__0910232114
#define INCLUDED_FILE__TUNNELEX__Crypto_hpp__0910232114

#include "StringUtil.hpp"
#include "Foreach.h"
#include <openssl/pkcs12.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include "CompileWarningsBoost.h"
#	include <boost/noncopyable.hpp>
#	include <boost/assert.hpp>
#	include <boost/static_assert.hpp>
#include "CompileWarningsBoost.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <list>
#include <exception>
#include <memory>
#include <limits>

#pragma comment(lib, "libeay32.lib")

#define CRYPTO_OPENSSL_ERROR_STR_MAX_LEN 120
#define CRYPTO_OPENSSL_ERROR_STR_BUFFER_LEN CRYPTO_OPENSSL_ERROR_STR_MAX_LEN * 3

namespace TunnelEx {
namespace Helpers {
namespace Crypto {

	//////////////////////////////////////////////////////////////////////////

	class OpenSslError {
	
	private:
		
		typedef std::list<unsigned long> ErrorCodes;

	private:

		explicit OpenSslError(const ErrorCodes &errorCodes) throw()
				: m_errorCodes(errorCodes) {
			m_buffer[0] = 0;
#			ifndef NDEBUG
				LoadStringError();
#			endif // #ifndef NDEBUG
		}

	public:
	
		static void PopLast() {
			ERR_get_error();
		}

		static OpenSslError GetLast(bool removesTheEntryFromLibQuery) {
			ErrorCodes errors;
			unsigned long(*getter)(void) = removesTheEntryFromLibQuery
				?	&ERR_get_error
				:	&ERR_peek_error;
			for ( ; ; ) {
				const unsigned long error = getter();
				if (error == 0) {
					break;
				} else if (error != 0x140A4044) { // SSL_F_SSL_CLEAR, ERR_R_INTERNAL_ERROR)
					errors.push_back(error);
				}
			}
			return OpenSslError(errors);
		}

		static std::string ErrorNoToString(const unsigned long errorNo) {
			char buffer[CRYPTO_OPENSSL_ERROR_STR_BUFFER_LEN];
			ERR_error_string_n(errorNo, &buffer[0], sizeof buffer);
			const std::string result = &buffer[0];
			return result;
		}

		static bool CheckError(const unsigned long errorNo) {
			return
				errorNo != 0x140A4044 // SSL_F_SSL_CLEAR, ERR_R_INTERNAL_ERROR
				&& ERR_lib_error_string(errorNo) != 0
				&& ERR_func_error_string(errorNo) != 0
				&& ERR_reason_error_string(errorNo) != 0;
		}

	public:

		const char * GetAsString() const throw() {
			if (!m_buffer[0]) {
				const_cast<OpenSslError *>(this)->LoadStringError();
			}
			return m_buffer;
		}

		size_t GetErrorsNumb() const {
			return m_errorCodes.size();
		}

		bool IsReason(const unsigned long reason) const {
			return
				GetErrorsNumb() == 1
				&& ERR_GET_REASON(*m_errorCodes.begin()) == reason;
		}

	private:

		void LoadStringError() throw() {
			BOOST_ASSERT(m_buffer[0] == 0);
			char *ptr = m_buffer;
			foreach (const unsigned long error, m_errorCodes) {
				ptr += strlen(ptr);
				if (m_buffer + CRYPTO_OPENSSL_ERROR_STR_BUFFER_LEN - 2 <= ptr) {
					break;
				} else if (ptr != m_buffer){
					ptr[0] = ';';
					ptr[1] = ' ';
					ptr += 2;
				}
				const size_t len = m_buffer + CRYPTO_OPENSSL_ERROR_STR_BUFFER_LEN - ptr;
				ERR_error_string_n(error, ptr, len);
			}
		}

	private:

		ErrorCodes m_errorCodes;
		char m_buffer[CRYPTO_OPENSSL_ERROR_STR_BUFFER_LEN];

	};

	//////////////////////////////////////////////////////////////////////////

	class Exception : public std::exception {
	public:
		Exception() {
			//...//
		}
		explicit Exception(const char *what)
				: exception(what) {
			//...//
		}
	};

	class OpenSslException : public Exception {
	public:
		explicit OpenSslException(const OpenSslError &error)
				: m_error(error) {
			//...//
		}
		virtual ~OpenSslException() {
			//...//
		}
		virtual const char * what() const throw() {
			return m_error.GetAsString();
		}
	private:
		OpenSslError m_error;
	};

	//////////////////////////////////////////////////////////////////////////

	struct Util {

	private:

		static int pint(const char **s, int n, int min, int max, int *e) {
			// copy-past, code source: https://www.openevidence.org/oedoxy/base__utils_8c-source.html#l00082
			int retval = 0;
			while (n) {
				if (**s < '0' || **s > '9') { *e = 1; return 0; }
				retval *= 10;
				retval += **s - '0';
				--n; ++(*s);
			}
			if (retval < min || retval > max) *e = 1;
			return retval;
		}

	public:

		static time_t ConvertAsn1TimeToTimeTUtc(const ASN1_TIME *const asn1Time) {
			// copy-past, code source: https://www.openevidence.org/oedoxy/group__base__utils.html
			char days[2][12] = {
				{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
				{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
			};
			const char *s;
			int generalized;
			struct tm t;
			int i, year, isleap, offset;
			time_t retval;
			if (asn1Time->type == V_ASN1_GENERALIZEDTIME) {
				generalized = 1;
			} else if (asn1Time->type == V_ASN1_UTCTIME) {
				generalized = 0;
			} else {
				throw Crypto::Exception("Could not get certificate time format");
			}
			s = reinterpret_cast<const char *>(asn1Time->data); // Data should be always null terminated
			if (s == 0 || s[asn1Time->length] != '\0') {
				throw Crypto::Exception("Could not get certificate time field length");
			}
			{
				int error = 0;
				if (generalized) {
					t.tm_year = pint(&s, 4, 0, 9999, &error) - 1900;
				} else {
					t.tm_year = pint(&s, 2, 0, 99, &error);
					if (t.tm_year < 50) t.tm_year += 100;
				}
				if (error) {
					throw Crypto::Exception("Could not get certificate time");
				}
			}
			{
				int error = 0;
				t.tm_mon = pint(&s, 2, 1, 12, &error) - 1;
				t.tm_mday = pint(&s, 2, 1, 31, &error);
				if (error) {
					throw Crypto::Exception("Could not get certificate time");
				}

			}
			{
				int error = 0;
				// NOTE: It's not yet clear, if this implementation is 100% correct
				// for GeneralizedTime... but at least misinterpretation is
				// impossible --- we just throw an exception
				t.tm_hour = pint(&s, 2, 0, 23, &error);
				t.tm_min = pint(&s, 2, 0, 59, &error);
				if (*s >= '0' && *s <= '9') {
					t.tm_sec = pint(&s, 2, 0, 59, &error);
				} else {
					t.tm_sec = 0;
				}
				if (error) {
					throw Crypto::Exception("Could not get certificate time");
				}
			}
			if (generalized) {
				// skip fractional seconds if any
				while (*s == '.' || *s == ',' || (*s >= '0' && *s <= '9')) ++s;
				// special treatment for local time
				if (*s == 0) {
					t.tm_isdst = -1;
					retval = mktime(&t); // Local time is easy :)
					if (retval == (time_t) - 1) {
						throw Crypto::Exception("Could not get certificate time");
					}
					return retval;
				}
			}
			if (*s == 'Z') {
				offset = 0;
				++s;
			} else if (*s == '-' || *s == '+') {
				i = (*s++ == '-');
				int error = 0;
				offset = pint(&s, 2, 0, 12, &error);
				offset *= 60;
				offset += pint(&s, 2, 0, 59, &error);
				if (error) {
					throw Crypto::Exception("Certificate time field format is wrong");
				}
				if (i) offset = -offset;
			} else {
				throw Crypto::Exception("Certificate time field format is wrong");
			}
			if (*s) {
				throw Crypto::Exception("Certificate time field format is wrong");
			}
			// And here comes the hard part --- there's no standard function to
			// convert struct tm containing UTC time into time_t without
			// messing global timezone settings (breaks multithreading and may
			// cause other problems) and thus we have to do this "by hand"
			// 
			// NOTE: Overflow check does not detect too big overflows, but is
			// sufficient thanks to the fact that year numbers are limited to four
			// digit non-negative values.
			retval = t.tm_sec;
			retval += (t.tm_min - offset) * 60;
			retval += t.tm_hour * 3600;
			retval += (t.tm_mday - 1) * 86400;
			year = t.tm_year + 1900;
			BOOST_STATIC_ASSERT(sizeof(time_t) == 8);
			/*if (sizeof(time_t) == 4) {
				// This is just to avoid too big overflows being undetected, finer
				// overflow detection is done below.
				if (year < 1900 || year > 2040) {
					throw Crypto::Exception("Wrong certificate time field format");
				}
			}*/
			// FIXME: Does POSIX really say, that all years divisible by 4 are
			// leap years (for consistency)??? Fortunately, this problem does
			// not exist for 32-bit time_t and we should'nt be worried about
			// this until the year of 2100 :)
			isleap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
			for (i = t.tm_mon - 1; i >= 0; --i) {
				retval += days[isleap][i] * 86400;
			}
			retval += (time_t(year) - time_t(1970)) * time_t(31536000);
			if (year < 1970) {
				retval -= ((1970 - year + 2) / 4) * 86400;
				BOOST_STATIC_ASSERT(sizeof(time_t) == 8);
				/* if (sizeof(time_t) > 4) */ {
					for (i = 1900; i >= year; i -= 100) {
						if (i % 400 == 0) continue;
						retval += 86400;
					}
				}
				if (retval >= 0) {
					throw Crypto::Exception("Certificate time field format is wrong");
				}
			} else {
				retval += ((year - 1970 + 1) / 4) * 86400;
				BOOST_STATIC_ASSERT(sizeof(time_t) == 8);
				/*if (sizeof(time_t) > 4)*/ {
					for (i = 2100; i < year; i += 100) {
						// The following condition is the reason to
						// start with 2100 instead of 2000
						if (i % 400 == 0) continue;
						retval -= 86400;
					}
				}
				if (retval < 0) {
					throw Crypto::Exception("Certificate time field format is wrong");
				}
			}
			return retval;
		}

	public:

		static void AddX509Entry(
					X509_NAME &name,
					const char *field,
					const std::string &value) {
			if (value.empty()) {
				return;
			}
			const int result = X509_NAME_add_entry_by_txt(
				&name,
				field,
				MBSTRING_ASC,
				reinterpret_cast<const unsigned char *>(value.c_str()),
				-1,
				-1,
				0);
			if (!result) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

		static void AddX509ExtensionVersion3(
					X509 *const certificate,
					const int extensionId,
					char *const value) {
			X509V3_CTX context;
			X509V3_set_ctx_nodb(&context);
			X509V3_set_ctx(&context, certificate, certificate, NULL, NULL, 0);
			X509_EXTENSION *const extension
				= X509V3_EXT_conf_nid(NULL, &context, extensionId, value);
			if (!extension) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			const int result = X509_add_ext(certificate, extension, -1);
			X509_EXTENSION_free(extension);
			if (!result) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

		static void AddX509ExtensionVersion3(
					STACK_OF(X509_EXTENSION) *const sk,
					int nid,
					char *const value) {
			X509_EXTENSION *const extension
				= X509V3_EXT_conf_nid(NULL, NULL, nid, value);
			if (!extension) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			sk_X509_EXTENSION_push(sk, extension);
		}

		static void AddX509Entries(
					X509_NAME &name ,
					const std::string &commonNameOptional,
					const std::string &organizationOptional,
					const std::string &organizationUnitOptional,
					const std::string &cityOptional,
					const std::string &stateOrProvinceOptional,
					const std::string &countryOptional) {
			AddX509Entry(name, "CN", commonNameOptional);
			AddX509Entry(name, "O", organizationOptional);
			AddX509Entry(name, "OU",organizationUnitOptional);
			AddX509Entry(name, "L", cityOptional);
			AddX509Entry(name, "ST", stateOrProvinceOptional);
			AddX509Entry(name, "C", countryOptional);
		}

		static void AddX509Entries(
					X509 *const certificate,
					const std::string &issuerCommonNameOptional,
					const std::string &issuerOrganizationOptional,
					const std::string &issuerOrganizationUnitOptional,
					const std::string &issuerCityOptional,
					const std::string &issuerStateOrProvinceOptional,
					const std::string &issuerCountryOptional,
					const std::string &subjectCommonNameOptional,
					const std::string &subjectOrganizationOptional,
					const std::string &subjectOrganizationUnitOptional,
					const std::string &subjectCityOptional,
					const std::string &subjectStateOrProvinceOptional,
					const std::string &subjectCountryOptional) {
			AddX509Entries(
				*X509_get_issuer_name(certificate),
				issuerCommonNameOptional,
				issuerOrganizationOptional,
				issuerOrganizationUnitOptional,
				issuerCityOptional,
				issuerStateOrProvinceOptional,
				issuerCountryOptional);
			AddX509Entries(
				*X509_get_subject_name(certificate),
				subjectCommonNameOptional,
				subjectOrganizationOptional,
				subjectOrganizationUnitOptional,
				subjectCityOptional,
				subjectStateOrProvinceOptional,
				subjectCountryOptional);
		}

		static void AddX509RequestEntries(
					X509_REQ &request,
					const std::string &subjectCommonNameOptional,
					const std::string &subjectOrganizationOptional,
					const std::string &subjectOrganizationUnitOptional,
					const std::string &subjectCityOptional,
					const std::string &subjectStateOrProvinceOptional,
					const std::string &subjectCountryOptional) {
			AddX509Entries(
				*X509_REQ_get_subject_name(&request),
				subjectCommonNameOptional,
				subjectOrganizationOptional,
				subjectOrganizationUnitOptional,
				subjectCityOptional,
				subjectStateOrProvinceOptional,
				subjectCountryOptional);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	//! @todo: replace with unique_ptr [2009/10/25 14:50]
	class AutoBio : private boost::noncopyable {
	public:
		explicit AutoBio(BIO_METHOD *method) /*throw(Crypto::Exception)*/
				: m_bio(BIO_new(method)) {
			if (!m_bio) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}
		explicit AutoBio(BIO *bio) throw()
				: m_bio(bio) {
			BOOST_ASSERT(m_bio);
		}
		~AutoBio() throw() {
			if (m_bio) {
				BIO_free(m_bio);
			}
		}
	public:
		BIO * Get() throw() {
			return m_bio;
		}
		const BIO * Get() const throw() {
			return const_cast<AutoBio *>(this)->Get();
		}
		BIO * Release() throw() {
			BIO *result = m_bio;
			if (m_bio) {
				m_bio = 0;
			}
			return result;
		}
	private:
		BIO *m_bio;
	};

	//////////////////////////////////////////////////////////////////////////

	//! @todo: replace with unique_ptr [2009/10/25 14:50]
	class AutoBioChain : private boost::noncopyable {
	public:
		explicit AutoBioChain(BIO *bio) throw()
				: m_bio(bio) {
			BOOST_ASSERT(m_bio);
		}
		~AutoBioChain() throw() {
			BIO_free_all(m_bio);
		}
	public:
		BIO * Get() throw() {
			return m_bio;
		}
	private:
		BIO *m_bio;
	};

	//////////////////////////////////////////////////////////////////////////

	class Asn1Integer : private boost::noncopyable {
		friend class BigNumber;
	private:
		Asn1Integer() 
				: m_impl(ASN1_INTEGER_new()) {
			if (!m_impl) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}
	public:
		~Asn1Integer() {
			ASN1_INTEGER_free(m_impl);
		}
	public:
		ASN1_INTEGER & Get() {
			return *m_impl;
		}
	private:
		ASN1_INTEGER *const m_impl;

	};

	//////////////////////////////////////////////////////////////////////////

	class BigNumber : private boost::noncopyable {
	private:
		BigNumber()
				: m_impl(BN_new())  {
			if (!m_impl) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}
		explicit BigNumber(BIGNUM *const impl)
				: m_impl(impl)  {
			BOOST_ASSERT(impl);
		}
	public:
		~BigNumber() {
			BN_free(m_impl);
		}
	public:
		static std::auto_ptr<BigNumber> GeneratePseudoRandomSerial() {
			std::auto_ptr<BigNumber> result(new BigNumber);
			if (!BN_pseudo_rand(result->m_impl, 64, 0, 0)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return result;
		}
	public:
		std::auto_ptr<Asn1Integer> GetAsAsn1Integer() const {
			std::auto_ptr<Asn1Integer> result(new Asn1Integer);
			if (!BN_to_ASN1_INTEGER(m_impl, result->m_impl)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return result;
		}
	private:
		BIGNUM *const m_impl;
	};

	//////////////////////////////////////////////////////////////////////////

	class OutBase64Stream : private boost::noncopyable {
	
	public:
	
		typedef std::vector<unsigned char> Buffer;

	public:

		explicit OutBase64Stream() /*throw(Crypto::Exception)*/ {
			//...//
		}
		
		~OutBase64Stream() {
			//...//
		}
		
	public:
	
		void Take(std::vector<unsigned char> &result) {
		
			const std::string &str = m_encoded.str();
			const int encodedSize = int(str.size());
			if (!encodedSize) {
				result.clear();
				return;
			}
			
			AutoBio base64(BIO_f_base64());
			AutoBio mem(
				BIO_new_mem_buf(const_cast<char *>(str.c_str()), encodedSize));
			BIO *const bioPush = BIO_push(base64.Get(), mem.Get());
			if (!bioPush) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			base64.Release();
			mem.Release();
			AutoBioChain bio(bioPush);
			std::vector<unsigned char> buffer;
			size_t readedTotal = 0;
			for ( ; ; ) {
				const int pending = BIO_pending(bio.Get());
				if (!pending) {
					break;
				}
				buffer.resize(buffer.size() + pending);
				const int readed = BIO_read(bio.Get(), &buffer[readedTotal], pending);
				readedTotal += readed;
				if (!readed) {
					break;
				}
			}

			buffer.resize(readedTotal);
			m_encoded.clear();
			buffer.swap(result);

		}

		OutBase64Stream & operator <<(const std::string &in) {
			m_encoded << in;
			return *this;
		}

	private:

		std::ostringstream m_encoded;

	};

	//////////////////////////////////////////////////////////////////////////
	
	class InBase64Stream : private boost::noncopyable {
	
	public:

		typedef std::vector<unsigned char> Buffer;

	public:

		InBase64Stream(bool isMultiLine) /*throw(Crypto::Exception)*/ {
			AutoBio base64(BIO_f_base64());
			if (!isMultiLine) {
				BIO_set_flags(base64.Get(), BIO_FLAGS_BASE64_NO_NL);
			}
			AutoBio mem(BIO_s_mem());
			m_bio = BIO_push(base64.Get(), mem.Get());
			m_bioBase64 = base64.Release();
			m_bioMem = mem.Release();
		}
		
		~InBase64Stream() {
			BIO_free_all(m_bio);
		}
		
	public:
	
		InBase64Stream & operator <<(const Buffer &in) {
			if (BIO_write(m_bio, &in[0], int(in.size())) < 0) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return *this;
		}

		InBase64Stream & operator <<(unsigned short in) {
			if (BIO_write(m_bio, &in, sizeof(in)) < 0) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return *this;
		}

		InBase64Stream & operator <<(const std::wstring &in) {
			if (BIO_write(m_bio, in.c_str(), int(in.size())) < 0) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return *this;
		}

		InBase64Stream & operator <<(const std::string &in) {
			if (BIO_write(m_bio, in.c_str(), int(in.size())) < 0) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return *this;
		}

		InBase64Stream & operator <<(const char *in) {
			if (BIO_write(m_bio, in, int(strlen(in))) < 0) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return *this;
		}

		void Flush() {
			BIO_flush(m_bio);
		}

		std::string GetString() const {
			const_cast<InBase64Stream *>(this)->Flush();
			BUF_MEM *buffer;
			BIO_get_mem_ptr(m_bioMem, &buffer);
			return std::string(buffer->data, buffer->data + buffer->length);
		}

	private:

		BIO *m_bioBase64;
		BIO *m_bioMem;
		BIO *m_bio;

	};

	//////////////////////////////////////////////////////////////////////////

	class Key : private boost::noncopyable {

	public:

		enum Size {
			SIZE_512	= 512,
			SIZE_1024	= 1024,
			SIZE_2048	= 2048,
			SIZE_4096	= 4096,
			SIZE_8192	= 8192
		};

	protected:
	
		explicit Key(bool isPublic) /*throw(Crypto::Exception)*/
				: m_isPublic(isPublic),
				m_key(EVP_PKEY_new()) {
			if (!m_key) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

		Key(EVP_PKEY *const key, bool isPublic) throw()
				: m_isPublic(isPublic),
				m_key(key) {
			BOOST_ASSERT(key);
		}

		Key(const Key &rhs) /*throw(Crypto::Exception)*/
				: m_isPublic(rhs.m_isPublic) {
			EVP_PKEY *dupKey = EVP_PKEY_new();
			if (!dupKey) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			try {
				switch(rhs.m_key->type) {
					default:
						BOOST_ASSERT(false);
						throw Exception("Wrong or unsupported algorithm type");
					case EVP_PKEY_RSA: // currently supports only RSA
						{
							RSA *rsa = 0;
							RSA *rsaDupKey = 0;
							try {
								rsa = EVP_PKEY_get1_RSA(rhs.m_key);
								if (!rsa) {
									throw OpenSslException(OpenSslError::GetLast(true));
								}
								rsaDupKey = m_isPublic
									?	RSAPublicKey_dup(rsa)
									:	RSAPrivateKey_dup(rsa);
								if (!rsaDupKey) {
									throw OpenSslException(OpenSslError::GetLast(true));
								}
								if (!EVP_PKEY_set1_RSA(dupKey, rsaDupKey)) {
									throw OpenSslException(OpenSslError::GetLast(true));
								}
							} catch (...) {
								if (rsaDupKey != 0) {
									RSA_free(rsaDupKey);
								}
								if (rsa != 0) {
									RSA_free(rsa);
								}
								throw;
							}
							RSA_free(rsaDupKey);
							RSA_free(rsa);
						}
						break;
				}
			} catch (...) {
				EVP_PKEY_free(dupKey);
				throw;
			}
			m_key = dupKey;
		}

		~Key() throw() {
			EVP_PKEY_free(m_key);
		}

	protected:

		void Swap(Key &rhs) throw() {
			EVP_PKEY *const keyTmp = m_key;
			bool isPublicTmp = m_isPublic;
			m_key = rhs.m_key;
			m_isPublic = rhs.m_isPublic;
			rhs.m_key = keyTmp;
			rhs.m_isPublic = isPublicTmp;
			
		}

	public:

		EVP_PKEY & Get() {
			return *m_key;
		}

		const EVP_PKEY & Get() const {
			return const_cast<Key *>(this)->Get();
		}

		Size GetSize() const {

			const BIGNUM *size;
			switch (m_key->type) {
				case EVP_PKEY_RSA: 
					size = m_key->pkey.rsa->n;
					break;
				case EVP_PKEY_DSA:
					size = m_isPublic
						?	m_key->pkey.dsa->pub_key
						:	m_key->pkey.dsa->priv_key;
					break;
				default:
					BOOST_ASSERT(false);
					throw Exception("Wrong or unsupported algorithm type");
			}

			const int result = BN_num_bits(size);
			switch (result) {
				case SIZE_512:
				case SIZE_1024:
				case SIZE_2048:
				case SIZE_4096:
				case SIZE_8192:
					return Size(result);
				default:
					BOOST_ASSERT(false);
					throw Exception("Wrong key size");
			}
		
		}

	private:

		bool m_isPublic;
		EVP_PKEY *m_key;
		
	};

	//////////////////////////////////////////////////////////////////////////
	
	class PrivateKey : public Key {

		friend class Rsa;

	protected:

		PrivateKey() /*throw(Crypto::Exception)*/
				: Key(false) {
			//...//
		}

	public:

		explicit PrivateKey(
					const std::string &key)
				/*throw(Crypto::Exception)*/
				: Key(
					Load(
						reinterpret_cast<const unsigned char *>(key.c_str()),
						key.size()),
					false) {
			//...//
		}

		explicit PrivateKey(
					const std::vector<unsigned char> &key)
				/*throw(Crypto::Exception)*/
				: Key(Load(&key[0], key.size()), false) {
			//...//
		}

		explicit PrivateKey(
					const unsigned char *ptr,
					size_t len)
				/*throw(Crypto::Exception)*/
				: Key(Load(ptr, len), false) {
			//...//
		}

		explicit PrivateKey(EVP_PKEY *const keyImpl) throw()
				: Key(keyImpl, false) {
			//...//
		}

		const PrivateKey & operator =(
					const PrivateKey &rhs)
				/*throw(Crypto::Exception)*/ {
			PrivateKey(rhs).Swap(*this);
			return *this;
		}

	public:

		void Swap(PrivateKey &rhs) throw() {
			Key::Swap(rhs);
		}
	
		std::string Export() const {
			AutoBio bio(BIO_s_mem());
			if (!PEM_write_bio_PrivateKey(bio.Get(), &const_cast<PrivateKey *>(this)->Get(), 0, 0, 0, 0, 0)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			return std::string(buffer->data, buffer->data + buffer->length);
		}
		
	private:
	
		EVP_PKEY * Load(const unsigned char *ptr, size_t len) const /*throw(Crypto::Exception)*/ {
			BIO *const bio = BIO_new_mem_buf(const_cast<unsigned char *>(ptr), int(len));
			if (!bio) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			AutoBio in(bio);
			EVP_PKEY *const result = PEM_read_bio_PrivateKey(in.Get(), 0, 0, "");
			if (!result) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return result;
		}
		
	};

	//////////////////////////////////////////////////////////////////////////
	
	class PublicKey : public Key {

		friend class Rsa;
	
	protected:
	
		PublicKey() /*throw(Crypto::Exception)*/
				: Key(true) {
			//...//
		}

	public:

		explicit PublicKey(
					const std::vector<unsigned char> &key)
				/*throw(Crypto::Exception)*/
				: Key(Load(&key[0], key.size()), true) {
			//...//
		}
	
		explicit PublicKey(
					const unsigned char *ptr,
					size_t len)
				/*throw(Crypto::Exception)*/
				: Key(Load(ptr, len), true) {
			//...//
		}

		explicit PublicKey(EVP_PKEY *key) /*throw(Crypto::Exception)*/
				: Key(key, true) {
			//...//
		}

		const PublicKey & operator =(
					const PublicKey &rhs)
				/*throw(Crypto::Exception)*/ {
			PublicKey(rhs).Swap(*this);
			return *this;
		}
		
	public:

		void Swap(PublicKey &rhs) throw() {
			Key::Swap(rhs);
		}

		std::string Export() const {
			AutoBio bio(BIO_s_mem());
			if (!PEM_write_bio_PUBKEY(bio.Get(), &const_cast<PublicKey *>(this)->Get())) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			return std::string(buffer->data, buffer->data + buffer->length);
		}

	private:

		EVP_PKEY * Load(const unsigned char *ptr, size_t len) const /*throw(Crypto::Exception)*/ {
			BIO *const bio
				= BIO_new_mem_buf(const_cast<unsigned char *>(ptr), int(len));
			if (!bio) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			AutoBio in(bio);
			EVP_PKEY *const result = PEM_read_bio_PUBKEY(in.Get(), NULL, NULL, NULL);
			if (!result) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return result;
		}

	};

	//////////////////////////////////////////////////////////////////////////
	
	class Seale : private boost::noncopyable {
	
	public:

		typedef std::vector<unsigned char> Buffer;

	public:

		explicit Seale(
					const unsigned char *const data,
					const size_t dataLen,
					const PublicKey &publicKey)
				/*throw(Crypto::Exception)*/ {
			Encrypt(data, dataLen, publicKey);
		}
		
	public:
	
		const Buffer & GetSealed() const {
			return m_sealed;
		}
		
		const Buffer & GetEnvKey() const {
			return m_envKey;
		}
	
	private:
	
		void Encrypt(
					const unsigned char *dataPtr,
					size_t dataLen,
					const PublicKey &publicKey)
				/*throw(Crypto::Exception)*/ {

			EVP_PKEY *publicKeyPtr = const_cast<EVP_PKEY *>(&publicKey.Get());
			Buffer envKey(EVP_PKEY_size(publicKeyPtr));
			unsigned char *envKeyPtrs[1];
			envKeyPtrs[0] = &envKey[0];
			int envKeyLen = 0;
			EVP_CIPHER_CTX ctx;

			if (!EVP_SealInit(&ctx, EVP_rc4(), envKeyPtrs, &envKeyLen, 0, &publicKeyPtr, 1)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			Buffer buffer(dataLen + EVP_CIPHER_CTX_block_size(&ctx));
			
			int len1;
			int len2;
			if (
				!EVP_SealUpdate(&ctx, &buffer[0], &len1, dataPtr, int(dataLen))
				||	!EVP_OpenFinal(&ctx, &buffer[len1], &len2)
				||	len1 + len2 == 0) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			
			buffer.resize(len1 + len2);
			
			m_sealed.swap(buffer);
			m_envKey.swap(envKey);

		}
		
	private:
	
		Buffer m_sealed;
		Buffer m_envKey;
		
	};
	
	//////////////////////////////////////////////////////////////////////////
	
	class Sealed : private boost::noncopyable {
	
	public:
	
		typedef std::vector<unsigned char> Buffer;

	public:

		explicit Sealed(
					const unsigned char *const encrypted,
					size_t encryptedLen,
					const unsigned char *const envKey,
					size_t envKeyLen,
					const PrivateKey &privateKey)
				/*throw(Crypto::Exception)*/ {
			Open(encrypted, encryptedLen, envKey, envKeyLen, privateKey);
		}
		
	public:
	
		Buffer & GetOpened() {
			return m_opened;
		}
		
		const Buffer & GetOpened() const {
			return const_cast<Sealed *>(this)->GetOpened();
		}
	
	protected:
	
		void Open(
					const unsigned char *const encrypted,
					size_t encryptedLen,
					const unsigned char *const envKey,
					size_t envKeyLen,
					const PrivateKey &privateKey)
				/*throw(Crypto::Exception)*/ {
			EVP_CIPHER_CTX ctx;
			BOOST_ASSERT(encryptedLen <= (1024 * 5));
			Buffer buffer(encryptedLen + 1);
			int len1,
				len2;
			if (	!EVP_OpenInit(
						&ctx,
						EVP_rc4(),
						envKey,
						int(envKeyLen),
						NULL,
						const_cast<EVP_PKEY *>(&privateKey.Get()))) {
				throw OpenSslException(OpenSslError::GetLast(true));
			} else if(	!EVP_OpenUpdate(
							&ctx,
							&buffer[0],
							&len1,
							encrypted,
							int(encryptedLen))) {
				BOOST_ASSERT(false);
				throw OpenSslException(OpenSslError::GetLast(true));
			} else {
				BOOST_ASSERT(size_t(len1) < buffer.size());
				if (!EVP_OpenFinal(&ctx, &buffer[len1], &len2) || len1 + len2 == 0) {
					BOOST_ASSERT(false);
					throw OpenSslException(OpenSslError::GetLast(true));
				}
			}
			buffer.resize(len1 + len2);
			m_opened.swap(buffer);
		}
		
	private:
		
		Buffer m_opened;

	};

	//////////////////////////////////////////////////////////////////////////
	
	class SignSha1 : private boost::noncopyable {
	
	public:
	
		explicit SignSha1(const std::vector<unsigned char> &publicKey)
				/*throw(Crypto::Exception)*/
				: m_publicKey(&publicKey[0], publicKey.size()) {
			if (!EVP_VerifyInit(&m_ctx, EVP_sha1())) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}
		
	public:
	
		bool Verify(
					unsigned char *dataPtr,
					size_t dataLen,
					unsigned char *signPtr,
					size_t signLen)
				/*throw(Crypto::Exception)*/ {
			if (!EVP_VerifyUpdate(&m_ctx, dataPtr, dataLen)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			const int verifyResult = EVP_VerifyFinal(
				&m_ctx,
				signPtr,
				unsigned int(signLen),
				&m_publicKey.Get());
			if (verifyResult == -1) {
				throw OpenSslException(OpenSslError::GetLast(true));
			} else if (verifyResult == 0) {
				OpenSslError::PopLast();
			}
			return verifyResult != 0;
		}
		
	private:
	
		PublicKey m_publicKey;
		EVP_MD_CTX m_ctx;
	
	};
	
	//////////////////////////////////////////////////////////////////////////
	
	class DigestSha1 : private boost::noncopyable {
	
	public:
	
		explicit DigestSha1(const std::string &source) /*throw(Crypto::Exception)*/ {
			Calc(reinterpret_cast<const unsigned char *>(source.c_str()), source.size());
		}
		
		explicit DigestSha1(const unsigned char *ptr, size_t size) /*throw(Crypto::Exception)*/ {
			Calc(ptr, size);
		}
		
		explicit DigestSha1(const std::vector<unsigned char> &buffer) /*throw(Crypto::Exception)*/ {
			Calc(&buffer[0], buffer.size());
		}

	private:
	
		void Calc(const unsigned char *ptr, size_t size) {
			SHA_CTX ctx;
			std::vector<unsigned char> buffer(SHA_DIGEST_LENGTH);
			if (!SHA1_Init(&ctx) || !SHA1_Update(&ctx, ptr, size) || !SHA1_Final(&buffer[0], &ctx)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			m_digest.swap(buffer);
		}

	public:
	
		const std::string GetAscii() const {
			return StringUtil::BinToAscii(m_digest);
		}
		
	private:
		
		std::vector<unsigned char> m_digest;
	
	};

	//////////////////////////////////////////////////////////////////////////
	
	class Rsa {

	private:

		struct AutoHandle : private boost::noncopyable {
			explicit AutoHandle(RSA *rsaIn) throw()
					: rsa(rsaIn) {
				//...//
			}
			~AutoHandle() throw() {
				Reset();
			}
			void Release() throw() {
				rsa = 0;
			}
			void Reset() throw() {
				if (rsa) {
					RSA_free(rsa);
					rsa = 0;
				}
			}
			RSA *rsa;
		};

	protected:

		Rsa() {
			//...//
		}

	public:
	
		static std::auto_ptr<Rsa> Generate(const Key::Size size) /* throw(Crypto::Exception) */ {
	
			AutoHandle newKey(RSA_generate_key(size, RSA_F4, 0, 0));
			if (!newKey.rsa) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}

			unsigned long keyLen = i2d_RSAPublicKey(newKey.rsa, 0);
			keyLen += i2d_RSAPrivateKey(newKey.rsa, 0);

			std::vector<unsigned char> buffer(keyLen);
			unsigned char *i2dPtr = &buffer[0];
			keyLen = i2d_RSAPublicKey(newKey.rsa, &i2dPtr);
			keyLen += i2d_RSAPrivateKey(newKey.rsa, &i2dPtr);
			BOOST_ASSERT(keyLen == buffer.size());
			newKey.Reset();
			
			const unsigned char *d2iPtr = &buffer[0];
			AutoHandle publicKey(d2i_RSAPublicKey(NULL, &d2iPtr, keyLen));
			keyLen -= long(d2iPtr - &buffer[0]);
			AutoHandle privateKey(d2i_RSAPrivateKey(NULL, &d2iPtr, keyLen));
			if (!publicKey.rsa || !privateKey.rsa) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}

			std::auto_ptr<Rsa> result(new Rsa);
			
			if (!EVP_PKEY_assign_RSA(&result->m_privateKey.Get(), privateKey.rsa)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			privateKey.Release();
			if (!EVP_PKEY_assign_RSA(&result->m_publicKey.Get(), publicKey.rsa)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			publicKey.Release();

			return result;

		}
		
	public:
	
		const PrivateKey & GetPrivateKey() const {
			return const_cast<Rsa *>(this)->GetPrivateKey();
		}

		PrivateKey & GetPrivateKey() {
			return m_privateKey;
		}
		
		const PublicKey & GetPublicKey() const {
			return const_cast<Rsa *>(this)->GetPublicKey();
		}

		PublicKey & GetPublicKey() {
			return m_publicKey;
		}

	private:
	
		PublicKey m_publicKey;
		PrivateKey m_privateKey;
	
	};

	//////////////////////////////////////////////////////////////////////////

	class X509Shared {

	protected:

		X509Shared()
				: m_cert(X509_new()) {
			if (!m_cert) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

	public:

		explicit X509Shared(X509 *const cert) throw()
				: m_cert(cert) {
			BOOST_ASSERT(m_cert);
		}

		explicit X509Shared(const unsigned char *data, size_t dataLen) {
			const size_t headLen = std::min(size_t(27), dataLen);
			if (memcmp("-----BEGIN CERTIFICATE-----", data, headLen)) {
				m_cert = d2i_X509(0, &data, dataLen);
			} else {
				AutoBio bio(BIO_s_mem());
				if (BIO_write(bio.Get(), data, long(dataLen)) != long(dataLen)) {
					throw OpenSslException(OpenSslError::GetLast(true));
				}
				m_cert = PEM_read_bio_X509(bio.Get(), 0, 0, 0);
			}
			if (!m_cert) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

		X509Shared(const X509Shared &rhs)
				: m_cert(X509_dup(rhs.m_cert)) {
			if (!m_cert) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

		virtual ~X509Shared() throw() {
			X509_free(m_cert);
		}

		const X509Shared & operator =(const X509Shared &rhs) {
			X509Shared(rhs).Swap(*this);
			return *this;
		}

		void Swap(X509Shared &rhs) throw() {
			X509 *const oldCert = m_cert;
			m_cert = rhs.m_cert;
			rhs.m_cert = oldCert;
		}

	public:

		std::string GetSerialNumber() const {
			const ASN1_INTEGER *const asn1Serial = X509_get_serialNumber(m_cert);
			if (!asn1Serial) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			std::ostringstream result;
			if (asn1Serial->type == V_ASN1_NEG_INTEGER) {
				result << "-";
			}
			result << std::setfill('0') << std::hex << std::uppercase;
			for (int i = 0; i < asn1Serial->length; ++i) {
				result << std::setw(2) << int(asn1Serial->data[i]);
				if (i + 1 < asn1Serial->length) {
					result << ':';
				}
			}
			return result.str();
		}

		time_t GetValidBeforeTimeUtc() const {
			return Util::ConvertAsn1TimeToTimeTUtc(X509_get_notAfter(m_cert));
		}

		time_t GetValidAfterTimeUtc() const {
			return Util::ConvertAsn1TimeToTimeTUtc(X509_get_notBefore(m_cert));
		}

		std::string GetSubjectCommonName() const {
			return GetCommonName(*X509_get_subject_name(m_cert));
		}

		std::string GetIssuerCommonName() const {
			return GetCommonName(*X509_get_issuer_name(m_cert));
		}

		std::string GetSubjectOrganization() const {
			return GetOrganization(*X509_get_subject_name(m_cert));
		}

		std::string GetIssuerOrganization() const {
			return GetOrganization(*X509_get_issuer_name(m_cert));
		}

		std::string GetSubjectOrganizationUnit() const {
			return GetOrganizationUnit(*X509_get_subject_name(m_cert));
		}

		std::string GetIssuerOrganizationUnit() const {
			return GetOrganizationUnit(*X509_get_issuer_name(m_cert));
		}

		std::string GetSubjectCity() const {
			return GetCity(*X509_get_subject_name(m_cert));
		}

		std::string GetIssuerCity() const {
			return GetCity(*X509_get_issuer_name(m_cert));
		}

		std::string GetSubjectStateOrProvince() const {
			return GetStateOrProvince(*X509_get_subject_name(m_cert));
		}

		std::string GetIssuerStateOrProvince() const {
			return GetStateOrProvince(*X509_get_issuer_name(m_cert));
		}

		std::string GetSubjectCountry() const {
			return GetCountry(*X509_get_subject_name(m_cert));
		}

		std::string GetIssuerCountry() const {
			return GetCountry(*X509_get_issuer_name(m_cert));
		}

		const X509 & Get() const {
			return const_cast<X509Shared *>(this)->Get();
		}

		X509 & Get() {
			return *m_cert;
		}

	protected:

		std::string GetCommonName(X509_NAME &name) const {
			return GetEntry(name, NID_commonName);
		}

		std::string GetOrganization(X509_NAME &name) const {
			return GetEntry(name, NID_organizationName);
		}

		std::string GetOrganizationUnit(X509_NAME &name) const {
			return GetEntry(name, NID_organizationalUnitName);
		}

		std::string GetCity(X509_NAME &name) const {
			return GetEntry(name, NID_localityName);
		}

		std::string GetStateOrProvince(X509_NAME &name) const {
			return GetEntry(name, NID_stateOrProvinceName);
		}

		std::string GetCountry(X509_NAME &name) const {
			return GetEntry(name, NID_countryName);
		}

		std::string GetEntry(X509_NAME &name, int nid) const {
			for (int pos = -1; ; ) {
				pos = X509_NAME_get_index_by_NID(&name, nid, pos);
				if (pos == -1) {
					return std::string();
				}
				X509_NAME_ENTRY &entry = *X509_NAME_get_entry(&name, pos);
				unsigned char *out;
				int outlen = ASN1_STRING_to_UTF8(&out, X509_NAME_ENTRY_get_data(&entry));
				if(outlen < 0) {
					continue;
				}
				try {
					const std::string result(&out[0], &out[outlen]);
					OPENSSL_free(out);
					return result;
				} catch (...) {
					OPENSSL_free(out);
					throw;
				}
			}
		}

	public:

		PublicKey GetPublicKey() const {
			EVP_PKEY *const key = X509_get_pubkey(m_cert);
			if (!key) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			return PublicKey(key);
		}

		void Export(std::ostream &stream) const {
			AutoBio bio(BIO_s_mem());
			if (!PEM_write_bio_X509(bio.Get(), const_cast<X509 *>(m_cert))) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			stream.write(buffer->data, buffer->length);
		}

		void Export(std::vector<unsigned char> &resultBuffer) const {
			AutoBio bio(BIO_s_mem());
			if (!PEM_write_bio_X509(bio.Get(), const_cast<X509 *>(m_cert))) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			std::vector<unsigned char>(
					buffer->data,
					buffer->data + buffer->length)
				.swap(resultBuffer);
		}

		std::string Export() const {
			AutoBio bio(BIO_s_mem());
			if (!PEM_write_bio_X509(bio.Get(), const_cast<X509 *>(m_cert))) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			return std::string(buffer->data, buffer->data + buffer->length);
		}

		void Print(std::ostream &stream) const {
			AutoBio bio(BIO_s_mem());
			X509_print(bio.Get(), m_cert);
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			stream.write(buffer->data, buffer->length);
		}

		void Print(std::vector<unsigned char> &resultBuffer) const {
			PrintToContainer(resultBuffer);
		}

		void Print(std::string &resultBuffer) const {
			PrintToContainer(resultBuffer);
		}

	protected:

		template<typename Container>
		void PrintToContainer(Container &result) const {
			AutoBio bio(BIO_s_mem());
			X509_print(bio.Get(), m_cert);
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			Container(buffer->data, buffer->data + buffer->length).swap(result);
		}

	private:

		X509 *m_cert;

	};

	//////////////////////////////////////////////////////////////////////////

	class X509Private : public X509Shared {

	public:

		explicit X509Private(
					X509 *const certificate,
					EVP_PKEY *const privateKey)
				throw()
				: X509Shared(certificate),
				m_privateKey(privateKey) {
			//...//
		}

		explicit X509Private(
					const unsigned char *data,
					size_t dataLen,
					const PrivateKey &privateKey)
				: X509Shared(data, dataLen),
				m_privateKey(privateKey) {
			//...//
		}

	protected:

		explicit X509Private(const PrivateKey &privateKey)
				: m_privateKey(privateKey) {
			//...//
		}

	public:

		const X509Private & operator =(const X509Private &rhs) {
			X509Private(rhs).Swap(*this);
			return *this;
		}

		void Swap(X509Private &rhs) throw() {
			X509Shared::Swap(rhs);
			rhs.m_privateKey.Swap(m_privateKey);
		}

	public:

		static std::auto_ptr<X509Private> GenerateVersion3(
					const PrivateKey &privateKey,
					const PublicKey &publicKey,
					const PrivateKey &signKey,
					const std::string &issuerCommonNameOptional,
					const std::string &issuerOrganizationOptional,
					const std::string &issuerOrganizationUnitOptional,
					const std::string &issuerCityOptional,
					const std::string &issuerStateOrProvinceOptional,
					const std::string &issuerCountryOptional,
					const std::string &subjectCommonNameOptional,
					const std::string &subjectOrganizationOptional,
					const std::string &subjectOrganizationUnitOptional,
					const std::string &subjectCityOptional,
					const std::string &subjectStateOrProvinceOptional,
					const std::string &subjectCountryOptional) {

			std::auto_ptr<X509Private> result(new X509Private(privateKey));
			X509_set_version(&result->Get(), 2);
			
			{
				const std::auto_ptr<Asn1Integer> serial(
					BigNumber::GeneratePseudoRandomSerial()->GetAsAsn1Integer());
				if (!X509_set_serialNumber(&result->Get(), &serial->Get())) {
					throw OpenSslException(OpenSslError::GetLast(true));
				}
			}

			X509_set_pubkey(
				&result->Get(),
				const_cast<EVP_PKEY *>(&publicKey.Get()));

			X509_gmtime_adj(X509_get_notBefore(&result->Get()), 0);
			X509_gmtime_adj(
				X509_get_notAfter(&result->Get()),
				std::numeric_limits<long>::max());

			Util::AddX509Entries(
				&result->Get(),
				issuerCommonNameOptional,
				issuerOrganizationOptional,
				issuerOrganizationUnitOptional,
				issuerCityOptional,
				issuerStateOrProvinceOptional,
				issuerCountryOptional,
				subjectCommonNameOptional,
				subjectOrganizationOptional,
				subjectOrganizationUnitOptional,
				subjectCityOptional,
				subjectStateOrProvinceOptional,
				subjectCountryOptional);

			Util::AddX509ExtensionVersion3(
				&result->Get(),
				NID_basic_constraints,
				"critical, CA:TRUE");
			Util::AddX509ExtensionVersion3(
				&result->Get(),
				NID_key_usage,
				"critical, nonRepudiation, digitalSignature, keyEncipherment,"
					" dataEncipherment, keyCertSign, cRLSign");
			Util::AddX509ExtensionVersion3(
				&result->Get(),
				NID_subject_key_identifier,
				"hash");
			Util::AddX509ExtensionVersion3(
				&result->Get(),
				NID_netscape_cert_type,
				"sslCA, client, server");

			if (	!X509_sign(
						&result->Get(),
						const_cast<EVP_PKEY *>(&signKey.Get()),
						EVP_sha1())) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}

			return result;

		}

	public:

		const PrivateKey & GetPrivateKey() const {
			return const_cast<X509Private *>(this)->GetPrivateKey();
		}

		PrivateKey & GetPrivateKey() {
			return m_privateKey;
		}

	private:

		PrivateKey m_privateKey;

	};

	//////////////////////////////////////////////////////////////////////////

	class X509Request : private boost::noncopyable {

	public:
		
		explicit X509Request(
					const PublicKey &publicKey,
					const PrivateKey &signKey,
					const std::string &subjectCommonNameOptional,
					const std::string &subjectOrganizationOptional,
					const std::string &subjectOrganizationUnitOptional,
					const std::string &subjectCityOptional,
					const std::string &subjectStateOrProvinceOptional,
					const std::string &subjectCountryOptional)
				: m_request(X509_REQ_new()) {

			if (!m_request) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}

			if (!X509_REQ_set_version(m_request, 0L)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}

			if (!X509_REQ_set_pubkey(m_request, const_cast<EVP_PKEY *>(&publicKey.Get()))) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}

			Util::AddX509RequestEntries(
				*m_request,
				subjectCommonNameOptional,
				subjectOrganizationOptional,
				subjectOrganizationUnitOptional,
				subjectCityOptional,
				subjectStateOrProvinceOptional,
				subjectCountryOptional);

			STACK_OF(X509_EXTENSION) *const extension = sk_X509_EXTENSION_new_null();
			if (!extension) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			try {
				Util::AddX509ExtensionVersion3(
					extension,
					NID_key_usage,
					"critical,digitalSignature,keyEncipherment");
				Util::AddX509ExtensionVersion3(
					extension,
					NID_netscape_cert_type,
					"client,email");
				if (!X509_REQ_add_extensions(m_request, extension)) {
					throw OpenSslException(OpenSslError::GetLast(true));
				}
			} catch (...) {
				sk_X509_EXTENSION_pop_free(extension, X509_EXTENSION_free);
				throw;
			}

			if (!X509_REQ_sign(m_request, const_cast<EVP_PKEY *>(&signKey.Get()), EVP_sha1())) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}

		}

	public:

		std::string Export() const {
			AutoBio bio(BIO_s_mem());
			if (!PEM_write_bio_X509_REQ(bio.Get(), const_cast<X509_REQ *>(m_request))) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			return std::string(buffer->data, buffer->data + buffer->length);
		}

	private:

		X509_REQ *const m_request;

	};

	//////////////////////////////////////////////////////////////////////////

	class Pkcs12 : private boost::noncopyable {

	public:

		explicit Pkcs12(
					const X509Private &cert,
					const std::string &friendlyName,
					const std::string &password)
				: m_pkcs12(
					PKCS12_create(
						const_cast<char *>(password.c_str()),
						const_cast<char *>(friendlyName.c_str()),
						const_cast<EVP_PKEY *>(&cert.GetPrivateKey().Get()),
						const_cast<X509 *>(&cert.Get()),
						NULL,
						0,
						0,
						0,
						0,
						0)) {
			if (!m_pkcs12) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

		explicit Pkcs12(const unsigned char *data, long dataLen)
				: m_pkcs12(d2i_PKCS12(0, &data, dataLen)) {
			if (!m_pkcs12) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
		}

		~Pkcs12() {
			PKCS12_free(m_pkcs12);
		}

	public:

		std::auto_ptr<X509Private> GetCertificate(
					const std::string &password)
				const {
			X509 *cert;
			EVP_PKEY *privateKey;
			stack_st_X509 *ca = 0;
			if (!PKCS12_parse(m_pkcs12, password.c_str(), &privateKey, &cert, &ca)) {
				throw OpenSslException(OpenSslError::GetLast(true));
			}
			BOOST_ASSERT(cert);
			BOOST_ASSERT(privateKey);
			if (ca) {
				sk_X509_pop_free(ca, X509_free);
			}
			return std::auto_ptr<X509Private>(
				new X509Private(cert, privateKey));
		}

		std::string GetFriendlyName() const {
			const STACK_OF(PKCS7) *const safes = PKCS12_unpack_authsafes(m_pkcs12);
			if (!safes) {
				return std::string();
			}
			const char *name = 0;
		    for (int i = 0; !name && i < sk_PKCS7_num(safes); ++i) {
				PKCS7 *const safe = sk_PKCS7_value(safes, i);
				if (OBJ_obj2nid(safe->type) != NID_pkcs7_data) {
					continue;
				}
				const STACK_OF(PKCS12_SAFEBAG) *const bags
					= PKCS12_unpack_p7data(safe);
				if (!bags) {
					continue;
				}
				for (int i = 0; !name && i < sk_PKCS12_SAFEBAG_num(bags); ++i) {
					PKCS12_SAFEBAG *const bag
						= sk_PKCS12_SAFEBAG_value(bags, i);
					name = PKCS12_get_friendlyname(bag);
				}
				sk_PKCS12_SAFEBAG_pop_free(bags, PKCS12_SAFEBAG_free);
			}
			sk_PKCS7_pop_free(safes, PKCS7_free);
			if (!name) {
				return std::string();
			}
			return name;
		}

		void Export(std::ostream &stream) const {
			AutoBio bio(BIO_s_mem());
			i2d_PKCS12_bio(bio.Get(), m_pkcs12);
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			stream.write(buffer->data, buffer->length);
		}

		void Export(std::vector<unsigned char> &resultBuffer) const {
			AutoBio bio(BIO_s_mem());
			i2d_PKCS12_bio(bio.Get(), m_pkcs12);
			BUF_MEM *buffer;
			BIO_get_mem_ptr(bio.Get(), &buffer);
			std::vector<unsigned char>(
					buffer->data,
					buffer->data + buffer->length)
				.swap(resultBuffer);
		}

	private:

		PKCS12 *const m_pkcs12;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__Crypto_hpp__0910232114
