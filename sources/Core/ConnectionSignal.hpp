/**************************************************************************
 *   Created: 2008/08/05 9:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__DataTransferSignal_hpp__0808050948
#define INCLUDED_FILE__TUNNELEX__DataTransferSignal_hpp__0808050948

#include "Api.h"
#include "SmartPtr.hpp"
#include "Instance.hpp"

namespace TunnelEx {

	class Tunnel;
	class MessageBlock;

	class TUNNELEX_CORE_API ConnectionSignal {

	public:
		
		ConnectionSignal();
		virtual ~ConnectionSignal() throw();

	private:

		ConnectionSignal(const ConnectionSignal &);
		const ConnectionSignal & operator =(const ConnectionSignal &);

	public:

		virtual void OnConnectionSetupCompleted(::TunnelEx::Instance::Id) = 0;

		virtual void OnNewMessageBlock(::TunnelEx::MessageBlock &) = 0;
		virtual void OnMessageBlockSent(::TunnelEx::MessageBlock &) = 0;

		virtual void OnConnectionClose(::TunnelEx::Instance::Id) = 0;
		virtual void OnConnectionClosed(::TunnelEx::Instance::Id) = 0;

		virtual ::TunnelEx::Tunnel & GetTunnel() = 0;
		virtual const ::TunnelEx::Tunnel & GetTunnel() const = 0;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__DataTransferSignal_hpp__0808050948
