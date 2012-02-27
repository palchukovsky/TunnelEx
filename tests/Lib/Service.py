'''
	Module: Service
	Created: Feb 19, 2012 2:16:54 PM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

import sys
import os
from TunnelEx.TcpServer import TcpServer
from TunnelEx.SslServer import SslServer
from TunnelEx.Test.EchoServer import EchoServer
from TunnelEx import Global

def ShowHelp():
	print 'Help:'
	print '	Commands:'
	print '		tcp-echo-server ${port}'
	print '		ssl-echo-server ${port}'
	print '	Options:'
	print '		dump - make dumps into stdout'

def Main():

	if len(sys.argv) < 2:
		ShowHelp()
		return

	isDumpOn = False
	for arg in sys.argv:
		if arg == 'dump':
			isDumpOn = True
			break

	if sys.argv[1] == 'tcp-echo-server' and len(sys.argv) > 2:
		print 'tcp-echo-server at port {0}:{1}...'.format('localhost', sys.argv[2])
		server = EchoServer(TcpServer(int(sys.argv[2])), isDumpOn)
		if isDumpOn:
			print 'starting echo...'
		server.WaitAndAnswer()
		if isDumpOn:
			print 'echo completed'
		return

	if sys.argv[1] == 'ssl-echo-server' and len(sys.argv) > 2:
		pemFilePath = os.path.join(Global.testsCryptoKeysDir, 'Server.pem')
		server = EchoServer(SslServer(pemFilePath, int(sys.argv[2])), isDumpOn)
		server.WaitAndAnswer()
		return

	ShowHelp()

if __name__ == '__main__':
	Main()
