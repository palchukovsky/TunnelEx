/**************************************************************************
 *   Created: 2010/11/07 15:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Crypto.cpp 1072 2010-11-25 20:02:26Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Crypto.hpp"

namespace ut = boost::unit_test;
namespace tex = TunnelEx;
namespace crypto = TunnelEx::Helpers::Crypto;
using namespace std;
using namespace boost;

namespace Test { BOOST_AUTO_TEST_SUITE(Crypto) 

BOOST_AUTO_TEST_CASE(Certificate) {

	auto_ptr<crypto::Rsa> rsa = crypto::Rsa::Generate(crypto::Key::SIZE_512);
	auto_ptr<crypto::X509Private> x509 = crypto::X509Private::GenerateVersion3(
		rsa->GetPrivateKey(),
		rsa->GetPublicKey(),
		rsa->GetPrivateKey(),
		string(),
		string(),
		string(),
		string(),
		string(),
		string(),
		string(),
		string(),
		string(),
		string(),
		string(),
		string());
	rsa.reset();

	vector<unsigned char> x509_1Buffer;
	x509->Export(x509_1Buffer);
	const string privateKey1 = x509->GetPrivateKey().Export();

	crypto::Pkcs12 pkcs12_1(*x509, "test friendly name", "test password");
	x509.reset();
	vector<unsigned char> pkcs12Buffer;
	pkcs12_1.Export(pkcs12Buffer);

	crypto::Pkcs12 pkcs12_2(&pkcs12Buffer[0], pkcs12Buffer.size());
	BOOST_CHECK(pkcs12_2.GetFriendlyName() == "test friendly name");
	BOOST_CHECK_THROW(pkcs12_2.GetCertificate("wrong password"), crypto::OpenSslException);
	BOOST_CHECK_THROW(pkcs12_2.GetCertificate(""), crypto::OpenSslException);
	x509 = pkcs12_2.GetCertificate("test password");
	vector<unsigned char> x509_2Buffer;
	x509->Export(x509_2Buffer);

	vector<unsigned char> x509_3Buffer;
	crypto::X509Shared(&x509_2Buffer[0], x509_2Buffer.size()).Export(x509_3Buffer);

	BOOST_CHECK(x509->GetPrivateKey().Export() == privateKey1);
	BOOST_CHECK(x509_1Buffer == x509_2Buffer);
	BOOST_CHECK(x509_1Buffer == x509_3Buffer);

}

BOOST_AUTO_TEST_SUITE_END() }
