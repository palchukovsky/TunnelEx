/**************************************************************************
 *   Created: 2010/11/07 15:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "Crypto.hpp"

namespace tex = TunnelEx;
namespace crypto = TunnelEx::Helpers::Crypto;

namespace Test {

	TEST(Crypto, Certificate) {

		std::auto_ptr<crypto::Rsa> rsa = crypto::Rsa::Generate(crypto::Key::SIZE_512);
		std::auto_ptr<crypto::X509Private> x509 = crypto::X509Private::GenerateVersion3(
			rsa->GetPrivateKey(),
			rsa->GetPublicKey(),
			rsa->GetPrivateKey(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string(),
			std::string());
		rsa.reset();

		std::vector<unsigned char> x509_1Buffer;
		x509->Export(x509_1Buffer);
		const std::string privateKey1 = x509->GetPrivateKey().Export();

		crypto::Pkcs12 pkcs12_1(*x509, "test friendly name", "test password");
		x509.reset();
		std::vector<unsigned char> pkcs12Buffer;
		pkcs12_1.Export(pkcs12Buffer);

		crypto::Pkcs12 pkcs12_2(&pkcs12Buffer[0], pkcs12Buffer.size());
		EXPECT_EQ("test friendly name", pkcs12_2.GetFriendlyName());
		EXPECT_THROW(pkcs12_2.GetCertificate("wrong password"), crypto::OpenSslException);
		EXPECT_THROW(pkcs12_2.GetCertificate(""), crypto::OpenSslException);
		x509 = pkcs12_2.GetCertificate("test password");
		std::vector<unsigned char> x509_2Buffer;
		x509->Export(x509_2Buffer);

		std::vector<unsigned char> x509_3Buffer;
		crypto::X509Shared(&x509_2Buffer[0], x509_2Buffer.size()).Export(x509_3Buffer);

		EXPECT_EQ(privateKey1, x509->GetPrivateKey().Export());
		EXPECT_EQ(x509_1Buffer, x509_2Buffer);
		EXPECT_EQ(x509_1Buffer, x509_3Buffer);

	}

}
