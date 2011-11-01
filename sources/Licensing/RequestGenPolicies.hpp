/**************************************************************************
 *   Created: 2009/11/19 1:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__RequestGenPolicies_hpp__0911190107
#define INCLUDED_FILE__TUNNELEX__RequestGenPolicies_hpp__0911190107

#include "Core/String.hpp"
#include "Types.hpp"

namespace TunnelEx { namespace Licensing {

	template<class ClientTrait, bool isTestMode>
	struct RequestGenerationPolicy {
	
		inline static void Generate(
					const std::string &license,
					std::string &requestResult,
					std::string &privateKeyResult,
					const boost::any &clientParam) {
		
			namespace pt = boost::posix_time;
			using namespace TunnelEx::Helpers::Crypto;
			using namespace TunnelEx::Helpers::Xml;
			
			typedef ClientTrait::ConstantStorage ConstantStorage;
			typedef ClientTrait::LocalStorage LocalStorage;
			
			boost::shared_ptr<Document> doc = Document::CreateNew("LicenseKeyRequest");
			doc->GetRoot()->SetAttribute("Version", "1.2");
			
			doc->GetRoot()->CreateNewChild("License")->SetContent(boost::to_upper_copy(license));
			
			boost::shared_ptr<Node> release = doc->GetRoot()->CreateNewChild("Release");
			{
				std::string now
					= pt::to_iso_extended_string(pt::second_clock::universal_time());
				assert(now[10] == 'T');
				now[10] = ' ';
				release->CreateNewChild("Time")->SetContent(now);
			}
			
			{
				boost::shared_ptr<Node> product = release->CreateNewChild("Product");
				product->SetAttribute("Name", ProductTrait<PRODUCT_TUNNELEX>::GetUuid());
				product->SetAttribute("Edition", EditionTrait<EDITION_STANDARD>::GetUuid());
				product->SetAttribute("Version", std::wstring(TUNNELEX_VERSION_FULL_W));
			}
			
			{
				typedef ClientTrait::WorkstationPropertiesQuery WorkstationPropertiesQuery;
				typedef ClientTrait::WorkstationPropertiesLocal WorkstationPropertiesLocal;
				WorkstationPropertyValues localProps;
				WorkstationPropertiesLocal::Get(localProps, clientParam);
				boost::shared_ptr<Node> worstation
					= doc->GetRoot()->CreateNewChild("Workstation");
				BOOST_FOREACH (
						const WorkstationPropertyValues::value_type &prop,
						localProps) {
					boost::shared_ptr<Node> node = worstation->CreateNewChild("WorkstationProperty");
					node->SetAttribute(
						"Name",
						WorkstationPropertiesQuery::CastPropertyToString(prop.first));
					node->SetContent(prop.second);
				}
			}

			{
				boost::shared_ptr<Node> errorsListNode;
				size_t errorIndex = std::numeric_limits<size_t>::min();
				Error error;
				while (ClientTrait::Notification::GetError(errorIndex++, error)) {
					if (!errorsListNode) {
						errorsListNode = doc->GetRoot()->CreateNewChild("Errors");
					}
					auto errorNode = errorsListNode->CreateNewChild("Error");
					if (!error.license.empty()) {
						errorNode->SetAttribute("License", error.license);
					}
					errorNode->SetAttribute("Trait", boost::lexical_cast<std::string>(error.client));
					errorNode->SetAttribute("Time", error.time);
					errorNode->SetAttribute("Point", error.point);
					errorNode->SetContent(error.error);
				}
			}

			const std::auto_ptr<const Rsa> rsa(Rsa::Generate(Key::SIZE_512));
			doc->GetRoot()
				->CreateNewChild("PublicKey")
				->SetContent(rsa->GetPublicKey().Export());

			TunnelEx::UString xml;
			doc->Dump(xml);
			
			std::vector<unsigned char> serverPubKeyStr;
			ConstantStorage::GetLicenseServerAsymmetricPublicKey(serverPubKeyStr);
			PublicKey serverPubKey(serverPubKeyStr);
			Seale seale(xml.GetCStr(), xml.GetLength(), serverPubKey);
			
			InBase64Stream base64(true);
			base64
				<< seale.GetSealed()
				<< seale.GetEnvKey()
				<< unsigned short(seale.GetEnvKey().size());
			
			std::string request = "-----BEGIN TUNNELEX LICENSE KEY REQUEST-----\r\n";
			request += base64.GetString();
			request += "-----END TUNNELEX LICENSE KEY REQUEST-----\r\n";
			
			rsa->GetPrivateKey().Export().swap(privateKeyResult);
			requestResult.swap(request);

		}
		
	};
		
} }

#endif // INCLUDED_FILE__TUNNELEX__RequestGenPolicies_hpp__0911190107
