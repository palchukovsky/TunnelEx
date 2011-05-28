/**************************************************************************
 *   Created: 2008/08/05 9:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__TunnelDataTransferSignal_hpp__0808050950
#define INCLUDED_FILE__TUNNELEX__TunnelDataTransferSignal_hpp__0808050950

#include "ConnectionSignal.hpp"
#include "Log.hpp"
#include "DataTransferCommand.hpp"
#include "ServerWorker.hpp"

namespace TunnelEx {

	class Tunnel;
	class MessageBlock;

	class TunnelConnectionSignal : public ConnectionSignal {

	private:

		class MessageBlockHandlingCombiner {
		public:
			typedef DataTransferCommand result_type;
		public:
			explicit MessageBlockHandlingCombiner(Tunnel &tunnel)
					: m_tunnel(tunnel) {
				//...//
			}
			MessageBlockHandlingCombiner(const MessageBlockHandlingCombiner &rhs)
					: m_tunnel(rhs.m_tunnel) {
				//...//
			}
		private:
			const MessageBlockHandlingCombiner & operator =(
					const MessageBlockHandlingCombiner &rhs);
		public:
			template<typename InputIterator>
			DataTransferCommand operator ()(InputIterator first, InputIterator last) {
				DataTransferCommand lastSlotResult = DATA_TRANSFER_CMD_SEND_PACKET;
				for (	;
						first != last && lastSlotResult == DATA_TRANSFER_CMD_SEND_PACKET;
						lastSlotResult = *first++);
				if (lastSlotResult == DATA_TRANSFER_CMD_CLOSE_TUNNEL) {
					m_tunnel.GetServer().CloseTunnel(m_tunnel.GetInstanceId());
				}
#				ifdef _DEBUG
					// just a checking
					switch (lastSlotResult) {
						case DATA_TRANSFER_CMD_SEND_PACKET:
						case DATA_TRANSFER_CMD_SKIP_PACKET:
						case DATA_TRANSFER_CMD_CLOSE_TUNNEL:
							break;
						default:
							assert(false);
							break;
					}
#				endif // _DEBUG
				return lastSlotResult;
			}
		private:
			Tunnel &m_tunnel;
		};

	public:

		typedef void(OnConnectionSetupCompletedSlotSignature)(Instance::Id);
		typedef
				boost::function<OnConnectionSetupCompletedSlotSignature>
			OnAllConnectionsSetupCompletedSlot;

		typedef void(OnConnectionCloseSlotSignature)(Instance::Id);
		typedef boost::function<OnConnectionCloseSlotSignature> OnConnectionCloseSlot;

		typedef void(OnConnectionClosedSlotSignature)(Instance::Id);
		typedef boost::function<OnConnectionClosedSlotSignature> OnConnectionClosedSlot;

		typedef DataTransferCommand(OnNewMessageBlockSlotSignature)(MessageBlock &);
		typedef boost::function<OnNewMessageBlockSlotSignature> OnNewMessageBlockSlot;

		typedef void(OnMessageBlockSentSlotSignature)(const MessageBlock &);
		typedef boost::function<OnMessageBlockSentSlotSignature> OnMessageBlockSentSlot;

	private:

		typedef boost::signals2::signal<
				OnConnectionSetupCompletedSlotSignature>
			OnConnectionSetupCompletedSignal;
		typedef boost::signals2::signal<
				OnNewMessageBlockSlotSignature,
				MessageBlockHandlingCombiner>
			OnNewMessageBlockSignal;
		typedef boost::signals2::signal<OnConnectionCloseSlotSignature>
			OnConnectionCloseSignal;
		typedef boost::signals2::signal<OnConnectionClosedSlotSignature>
			OnConnectionClosedSignal;
		typedef boost::signals2::signal<OnMessageBlockSentSlotSignature>
			OnMessageBlockSentSignal;

	public:

		explicit TunnelConnectionSignal(Tunnel &tunnel)
				:  m_tunnel(tunnel),
				m_onNewMessageBlockSignal(MessageBlockHandlingCombiner(m_tunnel)) {
			//...//
		}

		virtual ~TunnelConnectionSignal() {
			//...//
		}

	public:

		virtual void OnConnectionSetupCompleted(Instance::Id instanceId) {
			m_onConnectionSetupCompletedSignal(instanceId);
		}

		virtual void OnNewMessageBlock(MessageBlock &messageBlock) {
			m_onNewMessageBlockSignal(messageBlock);
		}
		
		virtual void OnConnectionClose(Instance::Id instanceId) {
			m_onConnectionCloseSignal(instanceId);
		}

		virtual void OnConnectionClosed(Instance::Id instanceId) {
			m_onConnectionClosedSignal(instanceId);
		}

		virtual void OnMessageBlockSent(const ::TunnelEx::MessageBlock &messageBlock) {
			m_onMessageBlockSentSignal(messageBlock);
		}

		virtual Tunnel & GetTunnel() {
			return m_tunnel;
		}

		virtual const Tunnel & GetTunnel() const {
			return const_cast<TunnelConnectionSignal *>(this)->GetTunnel();
		}

	public:

		boost::signals2::connection ConnectToOnConnectionSetupCompleted(
					const OnAllConnectionsSetupCompletedSlot &slot) {
			return m_onConnectionSetupCompletedSignal.connect(slot);
		}

		boost::signals2::connection ConnectToOnNewMessageBlockSignal(
					const OnNewMessageBlockSlot &slot) {
			return m_onNewMessageBlockSignal.connect(slot);
		}
		
		boost::signals2::connection ConnectToOnConnectionCloseSignal(
					const OnConnectionCloseSlot &slot) {
			return m_onConnectionCloseSignal.connect(slot);
		}

		boost::signals2::connection ConnectToOnConnectionClosedSignal(
					const OnConnectionClosedSlot &slot) {
			return m_onConnectionClosedSignal.connect(slot);
		}

		boost::signals2::connection ConnectToOnMessageBlockSentSignal(
					const OnMessageBlockSentSlot &slot) {
			return m_onMessageBlockSentSignal.connect(slot);
		}

		void DisconnectDataTransfer() {
			m_onMessageBlockSentSignal.disconnect_all_slots();
			m_onNewMessageBlockSignal.disconnect_all_slots();
			m_onConnectionSetupCompletedSignal.disconnect_all_slots();
		}

		void DisconnectAll() {
			m_onConnectionSetupCompletedSignal.disconnect_all_slots();
			m_onNewMessageBlockSignal.disconnect_all_slots();
			m_onConnectionCloseSignal.disconnect_all_slots();
			m_onConnectionClosedSignal.disconnect_all_slots();
			m_onMessageBlockSentSignal.disconnect_all_slots();
		}

	private:

		Tunnel &m_tunnel;
		
		OnConnectionSetupCompletedSignal m_onConnectionSetupCompletedSignal;
		OnNewMessageBlockSignal m_onNewMessageBlockSignal;
		OnConnectionCloseSignal m_onConnectionCloseSignal;
		OnConnectionClosedSignal m_onConnectionClosedSignal;
		OnMessageBlockSentSignal m_onMessageBlockSentSignal;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__TunnelDataTransferSignal_hpp__0808050950
