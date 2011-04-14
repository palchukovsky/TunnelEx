/**************************************************************************
 *   Created: 2010/03/21 23:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: PathfinderEndpointAddress.hpp 1109 2010-12-26 06:33:37Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__PathfinderEndpointAddress_hpp__1003212359
#define INCLUDED_FILE__TUNNELEX__PathfinderEndpointAddress_hpp__1003212359

#include "Api.h"
#include "Modules/Inet/InetEndpointAddress.hpp"

namespace TunnelEx { namespace Mods { namespace Pathfinder {

	class TUNNELEX_MOD_PATHFINDER_API PathfinderEndpointAddress
			: public Mods::Inet::TcpEndpointAddress {

	public:

		typedef Mods::Inet::TcpEndpointAddress Base;

	public:

		PathfinderEndpointAddress();
		PathfinderEndpointAddress(const PathfinderEndpointAddress &);
		explicit PathfinderEndpointAddress(const WString &path, Server::ConstPtr = 0);
		virtual ~PathfinderEndpointAddress() throw();

		const PathfinderEndpointAddress & operator =(
				const PathfinderEndpointAddress &rhs);

		void Swap(PathfinderEndpointAddress &rhs) throw();

	public:

		virtual const TunnelEx::WString & GetResourceIdentifier() const;
		
		virtual bool IsHasMultiClientsType(void) const;
		
		virtual UniquePtr<Acceptor> OpenForIncomingConnections(
					const RuleEndpoint &,
					SharedPtr<const EndpointAddress>)
				const;

		virtual UniquePtr<EndpointAddress> Clone() const;

		virtual bool IsReadyToRecreateRemoteConnection() const {
			return IsReadyToRecreateConnection();
		}
		virtual bool IsReadyToRecreateLocalConnection() const {
			return IsReadyToRecreateConnection();
		}

	public:

		virtual void StatConnectionSetupCompleting() const throw();

		virtual void StatConnectionSetupCanceling() const throw();
		virtual void StatConnectionSetupCanceling(
				const ::TunnelEx::WString &reason)
			const
			throw();

	protected:

		virtual UniquePtr<Connection> CreateConnection(
				const RuleEndpoint &,
				SharedPtr<const EndpointAddress>) 
			const;

		virtual void ClearResourceIdentifierCache() throw();

		virtual const ACE_INET_Addr * GetFirstProxyAceInetAddr(bool, bool) const;

		bool IsReadyToRecreateConnection() const;

	public:

		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &host,
				Mods::Inet::NetworkPort port,
				const TunnelEx::SslCertificateId &certificate,
				const TunnelEx::SslCertificateIdCollection &remoteCertificates);

		static TunnelEx::WString CreateResourceIdentifier(
				const std::wstring &host,
				Mods::Inet::NetworkPort port,
				const TunnelEx::SslCertificateId &certificate,
				const TunnelEx::SslCertificateIdCollection &remoteCertificates,
				const Mods::Inet::ProxyList &);

	private:

		static void ConvertTcpToPathfinder(WString &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }

#endif // INCLUDED_FILE__TUNNELEX__PathfinderEndpointAddress_hpp__1003212359
