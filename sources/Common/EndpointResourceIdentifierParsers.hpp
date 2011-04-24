/**************************************************************************
 *   Created: 2010/01/02 6:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__EndpointResourceIdentifierParsers_hpp__1001020637
#define INCLUDED_FILE__TUNNELEX__EndpointResourceIdentifierParsers_hpp__1001020637

#include "Core/Exceptions.hpp"
#include "Core/SslCertificatesStorage.hpp"

/* include in precompile header:
	#include "StringUtil.hpp"
	#include "CompileWarningsBoost.h"
	#	include <boost/algorithm/string.hpp>
	#	include <boost/function.hpp>
	#	include <boost/lexical_cast.hpp>
	#	include <boost/regex.hpp>
	#include "CompileWarningsBoost.h"
	#include <string>
	#include <vector>
*/

namespace TunnelEx { namespace Helpers {

	struct EndpointResourceIdentifierParsers {

		typedef boost::algorithm::split_iterator<std::wstring::const_iterator>
			UrlSplitConstIterator;

		static void ParseUrlParam(
					UrlSplitConstIterator source,
					const wchar_t *const requiredParamName,
					boost::function<void(UrlSplitConstIterator)> parser,
					bool isArray) {
			using namespace TunnelEx;
			for ( ; !source.eof(); ++source) {
				UrlSplitConstIterator paramIt = boost::make_split_iterator(
					*source,
					boost::first_finder(L"=", boost::is_equal()));
				if (paramIt == source) {
					throw InvalidLinkException(L"Wrong parameter format");
				}
				const UrlSplitConstIterator paramName = paramIt;
				const UrlSplitConstIterator paramVal = ++paramIt;
				if (!paramVal.eof()) {
					if (iequals(*paramName, requiredParamName)) {
						if (boost::begin(*paramVal) == boost::end(*paramVal)) {
							throw TunnelEx::InvalidLinkException(
								L"Could not get parameter value");
						}
						parser(paramVal);
						if (!isArray) {
							break;
						}
					}
				} else {
					throw TunnelEx::InvalidLinkException(
						L"Could not get parameter value");
				}
			}
		}

		template<class T>
		static void ParseUrlParamValue(UrlSplitConstIterator source, T &result) {
			try {
				result = boost::lexical_cast<T>(
					boost::copy_range<std::wstring>(*source));
			} catch (const boost::bad_lexical_cast &) {
				throw TunnelEx::InvalidLinkException(
					L"Could not get typed parameter value");
			}
		}

		template<>
		static void ParseUrlParamValue(
					UrlSplitConstIterator source,
					unsigned char &result) {
			unsigned short tmp = 0;
			ParseUrlParamValue(source, tmp);
			result = unsigned char(tmp);
		}

		template<>
		static void ParseUrlParamValue(
					UrlSplitConstIterator source,
					std::wstring &result) {
			result = StringUtil::DecodeUrl(boost::copy_range<std::wstring>(*source));
		}

		template<>
		static void ParseUrlParamValue(
					UrlSplitConstIterator source,
					bool &result) {
			if (iequals(*source, "true")) {
				result = true;
			} else if (iequals(*source, "false")) {
				result = false;
			} else {
				throw TunnelEx::InvalidLinkException(
					L"Could not get parameter value with boolean type");
			}
		}

		template<>
		static void ParseUrlParamValue(
					UrlSplitConstIterator source,
					WString &result) {
			result = StringUtil::DecodeUrl(
					boost::copy_range<std::wstring>(*source))
				.c_str();
		}

		template<>
		static void ParseUrlParamValue(
					UrlSplitConstIterator source,
					SslCertificateIdCollection &result) {
			SslCertificateId tmp;
			ParseUrlParamValue(source, tmp);
			result.Append(tmp);
		}

		static void ParseEndpointHostPort(
					UrlSplitConstIterator source,
					std::wstring &hostResult,
					unsigned short &portResult,
					bool hostCanBeAny,
					bool portCanBeAny) {
			boost::wregex exp(L"([^:]*):(\\d+|\\*)");
			boost::wsmatch what;
			if (!boost::regex_match(boost::begin(*source), boost::end(*source), what, exp)) {
				throw TunnelEx::InvalidLinkException(L"Format is invalid");
			}
			std::wstring host = what[1];
			unsigned short port = 0;
			if (what[2].str()[0] != L'*') {
				try {
					port = boost::lexical_cast<unsigned short>(what[2].str());
				} catch (const boost::bad_lexical_cast &) {
					port = std::numeric_limits<unsigned short>::max();
				}
			}
			if ((port == 0 && !portCanBeAny) || (host == L"*" && !hostCanBeAny)) {
				throw TunnelEx::InvalidLinkException(
					L"Could not parse resource identifier");
			}
			host.swap(hostResult);
			portResult = port;
		}

		static void ParseEndpointCertificates(
					UrlSplitConstIterator source,
					SslCertificateId &certificate,
					SslCertificateIdCollection &remoteCertificates) {
			SslCertificateId certificateTmp;
			ParseUrlParam(
				source,
				L"certificate",
				boost::bind(
					&ParseUrlParamValue<SslCertificateId>,
					_1,
					boost::ref(certificateTmp)),
				false);
			if (!certificateTmp.IsEmpty()) {
				SslCertificateIdCollection remoteCertificatesTmp;
				ParseUrlParam(
					source,
					L"remote_certificate",
					boost::bind(
						&ParseUrlParamValue<SslCertificateIdCollection>,
						_1,
						boost::ref(remoteCertificatesTmp)),
					true);
#				if defined(_DEBUG) && defined(TEST)
					for (size_t i = 0; i < remoteCertificates.GetSize(); ++i) {
						assert(!remoteCertificates[i].IsEmpty());
					}
#				endif
				remoteCertificatesTmp.Swap(remoteCertificates);
			} else {
#				if defined(_DEBUG) && defined(TEST)
				{
					SslCertificateIdCollection remoteCertificates;
					ParseUrlParam(
						source,
						L"remote_certificate",
						boost::bind(
							&ParseUrlParamValue<SslCertificateIdCollection>,
							_1,
							boost::ref(remoteCertificates)),
						false);
					assert(remoteCertificates.GetSize() == 0);
				}
#				endif
				SslCertificateIdCollection().Swap(remoteCertificates);
			}
			certificateTmp.Swap(certificate);
		}

	};

} }

#endif // INCLUDED_FILE__TUNNELEX__EndpointResourceIdentifierParsers_hpp__1001020637
