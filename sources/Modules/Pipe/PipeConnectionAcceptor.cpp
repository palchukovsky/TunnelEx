/**************************************************************************
 *   Created: 2008/11/27 2:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: PipeConnectionAcceptor.cpp 982 2010-09-05 19:01:12Z palchukovsky $
 **************************************************************************/
 
#include "Prec.h"

#include "PipeConnectionAcceptor.hpp"
#include "IncomingPipeConnection.hpp"
 
using namespace TunnelEx;
using namespace TunnelEx::Mods::Pipe;
 
UniquePtr<Connection> PipeConnectionAcceptor::Accept() throw(ConnectionOpeningException) {
	return UniquePtr<Connection>(
		new IncomingPipeConnection(
			GetRuleEndpoint(),
			GetRuleEndpointAddress(),
			m_acceptor));
}
