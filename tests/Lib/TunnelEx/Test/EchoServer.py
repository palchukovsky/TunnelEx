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

	def __init__(self, server):
		self._server = server

	def WaitAndAnswer(self):
		connection = self._server.Accept()[0]
		self._server.Close()
		while True:
			data = None
			try:
				data = connection.recv()
			except:
				print "Connection lost at reading."
				break
			if not data:
				break
			connection.send(data)
		connection.close()
