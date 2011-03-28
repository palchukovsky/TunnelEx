/**************************************************************************
 *   Created: 2009/11/15 14:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Request.hpp 1074 2010-11-26 16:45:59Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Request_hpp__0911151453
#define INCLUDED_FILE__TUNNELEX__Request_hpp__0911151453

#include "Traits.hpp"

namespace TunnelEx { namespace Licensing {

	typedef ClientTrait<CLIENT_ONLINE_KEY_REQUST, false>::KeyRequest OnlineKeyRequest;
	typedef ClientTrait<CLIENT_ONLINE_KEY_REQUST, true>::KeyRequest OnlineKeyRequestTesting;
	typedef ClientTrait<CLIENT_OFFLINE_KEY_REQUST, false>::KeyRequest OfflineKeyRequest;
	typedef ClientTrait<CLIENT_SERVICE, false>::KeyRequest ServiceKeyRequest;

	template<class ClientTrait>
	class KeyRequest : private boost::noncopyable {
	
	public:
	
		typedef typename ClientTrait::LocalStorage LocalStorage;
		typedef typename ClientTrait::Comminication Comminication;
		typedef typename ClientTrait::RequestGeneration RequestGeneration;
		typedef typename ClientTrait::KeyRetrieve KeyRetrieve;

	public:
	
		inline explicit KeyRequest(
					const std::string &license,
					const boost::any &clientParam = boost::any())
				: m_clientParam(clientParam) {
			RequestGeneration::Generate(license, m_request, m_privateKey, m_clientParam);
		}

		inline explicit KeyRequest(
					const std::string &licenseKey,
					const std::string &privateKey,
					const boost::any &clientParam = boost::any())
				: m_privateKey(privateKey),
				m_licenseKey(licenseKey),
				m_clientParam(clientParam) {
			BOOST_ASSERT(!m_licenseKey.empty());
			BOOST_ASSERT(!m_privateKey.empty());
		}

		inline ~KeyRequest() {
			//...//
		}
	
	public:

		const std::string & GetContent() const {
			return m_request;
		}
	
		const std::string & GetPrivateKey() const {
			return m_privateKey;
		}

		void Send() {
			m_licenseKey = Comminication::SendRequest(m_request);
		}

		template<class License> 
		bool TestKey() const {
			return typename License::TestKey(
				 KeyRetrieve::Import(m_licenseKey, m_privateKey),
				 m_clientParam);
		}
		
		bool Accept() {
			BOOST_ASSERT(!m_licenseKey.empty());
			LocalStorage::StoreLicenseKey(m_licenseKey, m_privateKey, m_clientParam);
			return true;
		}
	
	private:
		
		std::string m_request;
		std::string m_privateKey;
		std::string m_licenseKey;
		const boost::any m_clientParam;
	
	};

} }

#endif // INCLUDED_FILE__TUNNELEX__Request_hpp__0911151453
