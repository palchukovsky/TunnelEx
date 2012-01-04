/**************************************************************************
 *   Created: 2007/03/01 2:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__Tunnel_h__0703010205
#define INCLUDED_FILE__Tunnel_h__0703010205

#include "Instance.hpp"
#include "SmartPtr.hpp"

class ACE_Proactor;

namespace TunnelEx {

	class TunnelRule;
	class Listener;
	class Connection;
	class ServerWorker;
	class TunnelConnectionSignal;

	//! Connection process handler.
	/** Opens and manages the current tunnel instance. */
	class Tunnel : public Instance {
	
	private:

		struct ReadWriteConnections {
			
			SharedPtr<Connection> read;
			SharedPtr<Connection> write;

			ReadWriteConnections();
			explicit ReadWriteConnections(
						SharedPtr<Connection> read,
						SharedPtr<Connection> write); 
			
			void Swap(ReadWriteConnections &) throw();

		};

		struct Licenses;

		class ListenerBinder;
		template<class Base>
		class ConnectionOpeningExceptionImpl;

		typedef ACE_Thread_Mutex AllConnectionsClosedMutex;
		typedef ACE_Guard<AllConnectionsClosedMutex> AllConnectionsClosedLock;
		typedef ACE_Thread_Condition<AllConnectionsClosedMutex> AllConnectionsClosedCondition;

	public:
		
		//! C'tor for new tunnel instance.
		explicit Tunnel(
				const bool isStatic,
				ServerWorker &server,
				SharedPtr<const TunnelRule> rule,
				SharedPtr<Connection> sourceRead,
				SharedPtr<Connection> sourceWrite);

		//! D'tor.
		~Tunnel() throw();

	public:

		bool IsStatic() const {
			return m_isStatic;
		}
		
		const Connection & GetIncomingReadConnection() const {
			return const_cast<Tunnel *>(this)->GetIncomingReadConnection();
		}
		const Connection & GetIncomingWriteConnection() const {
			return const_cast<Tunnel *>(this)->GetIncomingWriteConnection();
		}
		const Connection & GetOutcomingReadConnection() const {
			return const_cast<Tunnel *>(this)->GetOutcomingReadConnection();
		}
		const Connection & GetOutcomingWriteConnection() const {
			return const_cast<Tunnel *>(this)->GetOutcomingWriteConnection();
		}
		
		const TunnelRule & GetRule() const {
			return *m_rule;
		}

		const ACE_Proactor & GetProactor() const {
			return const_cast<Tunnel *>(this)->GetProactor();
		}

		ACE_Proactor & GetProactor();

		ServerWorker & GetServer() {
			return m_server;
		}

		const ServerWorker & GetServer() const {
			return const_cast<Tunnel *>(this)->GetServer();
		}

		void StartSetup();

		//! Closes current destination and tries to open next. Returns
		//! false if there no more destinations in list.
		bool Switch(
			const boost::optional<SharedPtr<Connection>> &sourceRead,
			const boost::optional<SharedPtr<Connection>> &sourceWrite);

		bool IsSetupFailed() const;

		bool IsSourceSetupFailed() const;
		bool IsDestinationSetupFailed() const;

		void MarkAsDead() throw() {
			assert(!m_isDead);
			m_isDead = true;
			DisconnectDataTransferSignals();
		}
		bool IsDead() const {
			return m_isDead;
		}

	private:

		void Init();

		ReadWriteConnections CreateDestinationConnections(size_t &) const;

		void DisconnectDataTransferSignals() throw();

		void ReportOpened() const;
		void ReportClosed() const;

		Connection & GetIncomingReadConnection() {
			return *m_source.read;
		}
		Connection & GetIncomingWriteConnection() {
			return *m_source.write;
		}
		Connection & GetOutcomingReadConnection() {
			return *m_destination.read;
		}
		Connection & GetOutcomingWriteConnection() {
			return *m_destination.write;
		}

		void OnConnectionSetup(Instance::Id);
		
		void OnConnectionClose(Instance::Id);
		void OnConnectionClosed(Instance::Id);

		void StartRead();

		Licenses & GetLicenses();

	private:

		const bool m_isStatic;

		ServerWorker &m_server;
		const SharedPtr<const TunnelRule> m_rule;

		SharedPtr<TunnelConnectionSignal> m_sourceDataTransferSignal;
		SharedPtr<TunnelConnectionSignal> m_destinationDataTransferSignal;

		std::list<Connection *> m_connectionsToSetup;
		long m_connectionsToClose;
		size_t m_setupComplitedConnections;

		ReadWriteConnections m_source;
		ReadWriteConnections m_destination;

		std::list<SharedPtr<const Listener>> m_listeners;

		unsigned int m_destinationIndex;

		long m_closedConnections;
		AllConnectionsClosedMutex m_allConnectionsClosedMutex;
		AllConnectionsClosedCondition m_allConnectionsClosedCondition;

		bool m_isDead;

	};

}

#endif // ifndef INCLUDED_FILE__Tunnel_h__0703010205
