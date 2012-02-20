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
from TunnelEx.SslServer import SslServer
from TunnelEx.Test.EchoServer import EchoServer

def Main():
	if len(sys.argv) > 1:
		if sys.argv[1] == 'ssl-echo-server':
			server = EchoServer(SslServer("A:\\server.pem", int(sys.argv[2])))
			server.WaitAndAnswer()
			return
	print 'Help:'
	print '	ssl-echo-server ${port}'

if __name__ == '__main__':
	Main()
