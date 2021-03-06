/**************************************************************************
 *   Created: 2009/09/11 18:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__KeyRetrievePolicies_hpp__0909111802
#define INCLUDED_FILE__TUNNELEX__KeyRetrievePolicies_hpp__0909111802

#include "StoragePolicies.hpp"

namespace TunnelEx { namespace Licensing {

	//! Policy for license key xml retrieving.
	/** Should has method "inline static TunnelEx::String Get()";
	  */
	template<class ClientTrait, bool isTestMode>
	struct XmlLicenseKeyRetrievePolicy {
	
		typedef typename ClientTrait::LocalStorage LocalStorage;
		typedef typename ClientTrait::ConstantStorage ConstantStorage;
	
		//! Extracts encrypted license key to stream, without errors and statuses.
		template<class ResultStream>
		inline static void ExtractEncryptedLicenseKey(
					const std::string &key,
					ResultStream &resultStream) {
			const std::string keyStart = "-----BEGIN TUNNELEX LICENSE KEY-----";
			const std::string keyEnd = "-----END TUNNELEX LICENSE KEY-----";
			bool isStarted = false;
			typedef boost::split_iterator<std::string::const_iterator> Iterator;
			for (	Iterator i = boost::make_split_iterator(key, boost::first_finder("\n", boost::is_iequal()));
					i != Iterator();
					++i) {
				std::string line = boost::copy_range<std::string>(*i);
				boost::trim_if(line, boost::is_space() || boost::is_cntrl());
				if (line.empty()) {
					continue;
				}
				if (!isStarted) {
					isStarted = line == keyStart;
				} else if (line == keyEnd) {
					break;
				} else {
					resultStream << boost::copy_range<std::string>(*i) << "\n";
				}
			}
		}
	
		inline static bool Get(std::string &key, const boost::any &clientParam) {
			const std::string encryptedLicenseKey
				= LocalStorage::GetLicenseKey(clientParam);
			if (encryptedLicenseKey.empty()) {
				return false;
			}
			key = Import(encryptedLicenseKey, clientParam);
			return true;
		}

		inline static std::string Import(
					const std::string &licenseKey,
					const boost::any &clientParam) {
			return Import(
				licenseKey,
				LocalStorage::GetLocalAsymmetricPrivateKey(clientParam));
		}

		inline static std::string Import(
					const std::string &licenseKey,
					const std::string &privateKeyStr) {
			
			using namespace TunnelEx::Helpers::Crypto;
			
			std::string result;
			
			try {

				OutBase64Stream outBase64;
				ExtractEncryptedLicenseKey(licenseKey, outBase64);
				std::vector<unsigned char> encryptedLicenseKey;
				outBase64.Take(encryptedLicenseKey);
				if (encryptedLicenseKey.empty()) {
					encryptedLicenseKey.push_back('x');
				} else if (*encryptedLicenseKey.rbegin() == 0) {
					// encryptedLicenseKey.pop_back();
				}

				size_t encryptedLicenseKeyLen = encryptedLicenseKey.size();
				size_t encryptedLicenseKeyEnvKeyLen = encryptedLicenseKeyLen;
				size_t encryptedLicenseKeyEnvKeyStartIndex = 0;
				if (encryptedLicenseKeyLen >= 2) {
					encryptedLicenseKeyEnvKeyLen
						= *(reinterpret_cast<unsigned short *>(&*(encryptedLicenseKey.rbegin() + 1)));
				}
				if (!(encryptedLicenseKeyLen < (encryptedLicenseKeyEnvKeyLen + 2))) {
					encryptedLicenseKeyEnvKeyStartIndex
						= encryptedLicenseKeyLen - encryptedLicenseKeyEnvKeyLen - 2;
				} else {
					encryptedLicenseKeyEnvKeyLen = encryptedLicenseKeyLen;
				}
				encryptedLicenseKeyLen = encryptedLicenseKeyEnvKeyStartIndex;
				PrivateKey privateKey(privateKeyStr);
				Sealed sealedLicenseKey(
					&encryptedLicenseKey[0],
					encryptedLicenseKeyLen,
					&encryptedLicenseKey[encryptedLicenseKeyEnvKeyStartIndex],
					encryptedLicenseKeyEnvKeyLen,
					privateKey);
				Sealed::Buffer &decryptedWithAsymmetric
					= sealedLicenseKey.GetOpened();
		
				size_t decryptedWithAsymmetricLicenseKeyLen
					= decryptedWithAsymmetric.size();
				size_t signLen = 128;
				size_t signStartIndex = 0;
				if (!(decryptedWithAsymmetricLicenseKeyLen < signLen)) {
					signStartIndex
						= decryptedWithAsymmetricLicenseKeyLen - signLen;
				} else {
					signLen = decryptedWithAsymmetricLicenseKeyLen;
				}
				const size_t encryptedWithSymmetricLicenseKeyLen = signStartIndex;
				std::vector<unsigned char> serverPublicKeyStr;
				ConstantStorage::GetLicenseServerAsymmetricPublicKey(serverPublicKeyStr);
				if (serverPublicKeyStr.empty()) {
					serverPublicKeyStr.push_back('x');
				}
				SignSha1 sign(serverPublicKeyStr);
				const bool isValid = sign.Verify(
					&decryptedWithAsymmetric[0],
					encryptedWithSymmetricLicenseKeyLen,
					&decryptedWithAsymmetric[signStartIndex],
					signLen);
				
				if (!isValid) {
					decryptedWithAsymmetric.resize(0);
				}
				
				std::vector<unsigned char> symmetricKey;
				ConstantStorage::GetLocalSymmetricKey(symmetricKey);
				size_t token = 0;
				foreach (unsigned char ch, decryptedWithAsymmetric) {
					if (token >= signStartIndex) {
						break;
					}
					ch ^= symmetricKey[token++ % symmetricKey.size()];
					result.push_back(ch);
				}
				
			} catch (const ::TunnelEx::Helpers::Crypto::Exception &) {
				//...//
			}

			return result;

		}

	};

} }

#endif // #ifndef INCLUDED_FILE__TUNNELEX__KeyRetrievePolicies_hpp__0909111802
