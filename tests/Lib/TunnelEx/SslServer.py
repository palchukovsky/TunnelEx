'''
	Module: TunnelEx.SslServer
	Created: Feb 19, 2012 2:35:46 PM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

import socket
from M2Crypto import SSL

class SslServer:

	def __init__(self, certFile, port, host = 'localhost', backlog = 5):
		self._context = SSL.Context()
		self._context.load_cert_chain(certFile)
		sock = socket.socket()
		sock.bind((host, port))
		sock.listen(backlog)
		self._connection = SSL.Connection(self._context, sock)

	def Accept(self):
		clientSocket, clientAddress = self._connection.accept()
		return [clientSocket, clientAddress]

	def Close(self):
		self._connection.close()
