'''
	Module: TunnelEx.Test.EchoServer
	Created: Feb 19, 2012 2:21:20 PM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

class EchoServer:

	def __init__(self, server, isDumpOn):
		self._server = server
		self._isDumpOn = isDumpOn

	def WaitAndAnswer(self):
		connection = self._server.Accept()[0]
		self._server.Close()
		self._server.SetConnectionTimeout(connection, 5)
		while True:
			data = None
			try:
				data = connection.recv(1024)
			except:
				print "Connection lost at reading."
				break
			if not data:
				break
			if self._isDumpOn:
				print '[{0} bytes]:<{1}>'.format(len(data), data)
			connection.send(data)
		connection.close()
