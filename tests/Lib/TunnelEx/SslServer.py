'''
	Module: TunnelEx.SslServer
	Created: Feb 19, 2012 2:35:46 PM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

from TunnelEx.TcpServer import TcpServer
from M2Crypto import SSL
from M2Crypto.SSL.timeout import timeout as SslTimeout

class SslServer(TcpServer):

	def __init__(self, certFile, port, host = 'localhost', backlog = 5):
		self._context = SSL.Context()
		self._context.load_cert_chain(certFile)
		self._SetConnection(
			SSL.Connection(
				self._context,
				self._OpenPort(host, port, backlog)))

	def SetConnectionTimeout(self, connection, timeoutSecs):
		timeout = SslTimeout(timeoutSecs)
		connection.set_socket_read_timeout(timeout)
		connection.set_socket_write_timeout(timeout)
