'''
	Module: TunnelEx.TcpServer
	Created: Feb 25, 2012 1:03:09 AM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

import socket

class TcpServer:

	def __init__(self, port, host = 'localhost', backlog = 5):
		self._SetConnection(self._OpenPort(host, port, backlog))

	def SetConnectionTimeout(self, connection, timeout):
		connection.settimeout(timeout)

	def Accept(self):
		clientSocket, clientAddress = self._connection.accept()
		return [clientSocket, clientAddress]

	def Close(self):
		self._connection.close()

	def _OpenPort(self, host, port, backlog):
		result = socket.socket()
		result.bind((host, port))
		result.listen(backlog)
		return result

	def _SetConnection(self, connection):
		self._connection = connection
