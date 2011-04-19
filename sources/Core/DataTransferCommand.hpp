/**************************************************************************
 *   Created: 2008/08/19 15:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__DataTransferCommand_hpp__0808191519
#define INCLUDED_FILE__TUNNELEX__DataTransferCommand_hpp__0808191519

namespace TunnelEx {

	//! Handler command.
	enum DataTransferCommand {
		//! Send packet.
		DATA_TRANSFER_CMD_SEND_PACKET,
		//! Block this packet.
		DATA_TRANSFER_CMD_SKIP_PACKET,
		//! Close current tunnel.
		DATA_TRANSFER_CMD_CLOSE_TUNNEL
	};

}

#endif // INCLUDED_FILE__TUNNELEX__DataTransferCommand_hpp__0808191519