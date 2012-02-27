
import sys
import socket
from M2Crypto import SSL
from TunnelEx.Test import Ftp
from M2Crypto.SSL.timeout import timeout as SslTimeout

class Test:

	class UnknownParameterType(Exception):
		pass

	def __init__(self):
		self._timeout = 5

	def ConnectToFtpServer(self, host, port, user, password, isPassive = None):
		"""Creates connection to FTP server and returns as object."""
		if isPassive is not None:
			isPassive = bool(isPassive)
		return Ftp.PlainConnection(
			str(host),
			int(port),
			str(user),
			str(password),
			isPassive)

	def ConnectToFtpesServer(
			self
			, host
			, port
			, user
			, password
			, isPassive = None
			, isTransferConnectionProtected = True):
		"""Creates connection to FTP server via SSL (Explicit FTPS) and returns as object."""
		if isPassive is not None:
			isPassive = bool(isPassive)
		return Ftp.SecureConnection(
			str(host),
			int(port),
			str(user),
			str(password),
			isPassive,
			bool(isTransferConnectionProtected))

	def CompareFtpServersFilesStructures(self, server1, server2):
		"""Compares two FTP servers files structures and returns True if they are identical."""
		return server1.Compare(server2)

	def ConnectToTcpServer(self, host, port):
		"""Creates TCP connection to server and returns as object."""
		result = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		result.settimeout(self._timeout)
		result.connect((str(host), int(port)))
		return result

	def ConnectToSslServer(self, host, port):
		"""Creates SSL connection to server and returns as object."""
		context = SSL.Context()
		context.set_verify(SSL.verify_none, depth = 0)
		# caFilePath = os.path.join(Global.testsCryptoKeysDir, 'Server.cer')
		# if context.load_verify_locations(caFilePath) != 1:
		#	raise Exception('No CA certificate')
		result = SSL.Connection(context)
		timeout = SslTimeout(self._timeout)
		result.set_socket_read_timeout(timeout)
		result.set_socket_write_timeout(timeout)
		result.connect((str(host), int(port)))
		return result

	def ReadFilesStructureFromFtpServer(self, connection):
		return Ftp.Struct(connection)

	def Dump(self, dumpSubj):
		"""Dumps into a string."""
		if isinstance(dumpSubj, Ftp.Struct):
			return dumpSubj.Dump()
		else:
			raise Test.UnknownParameterType

	def SendToConnection(self, connection, data):
		"""Sends data to connection."""
		if isinstance(connection, socket.socket) or isinstance(connection, SSL.Connection):
			connection.send(str(data))
		else:
			raise Test.UnknownParameterType

	def ReadFromConnection(self, connection):
		"""Read data from connection."""
		if isinstance(connection, socket.socket) or isinstance(connection, SSL.Connection):
			return str(connection.recv(1024))
		else:
			raise Test.UnknownParameterType

	def Disconnect(self, connection):
		"""Closes connection."""
		if isinstance(connection, Ftp.PlainConnection) or isinstance(connection, Ftp.SecureConnection):
			connection.Close()
		elif isinstance(connection, socket.socket) or isinstance(connection, SSL.Connection):
			connection.close()
		else:
			raise Test.UnknownParameterType

def Main():
	if len(sys.argv) > 1:
		if sys.argv[1] == 'test-read-from-ftp':
			print 'Testing reading from FTP {0}:{1}...'.format(
				sys.argv[2],
				sys.argv[3])
			test = Test()
			connection = test.ConnectToFtpServer(
				sys.argv[2],
				sys.argv[3],
				sys.argv[4],
				sys.argv[5])
			struct = test.ReadFilesStructureFromFtpServer(connection)
			test.DisconnectFromFtpServer(connection)
			test.DumpFtpServerFilesStructure(struct)
			print 'OK'
			return
		elif sys.argv[1] == 'test-read-from-ftpes':
			print 'Testing reading from FTP-SSL (Explicit FTPS) {0}:{1}...'.format(
				sys.argv[2],
				sys.argv[3])
			test = Test()
			connection = test.ConnectToFtpesServer(
				sys.argv[2],
				sys.argv[3],
				sys.argv[4],
				sys.argv[5])
			struct = test.ReadFilesStructureFromFtpServer(connection)
			test.DisconnectFromFtpServer(connection)
			test.DumpFtpServerFilesStructure(struct)
			print 'OK'
			return
		elif sys.argv[1] == 'test-ssl-client':
			print 'Testing SSL client to {0}:{1}...'.format(sys.argv[2], sys.argv[3])
			test = Test()
			connection = test.ConnectToSslServer(sys.argv[2], sys.argv[3])
			i = 0
			toSend = str()
			while i < 256:
				toSend += str(i)
				toSend += "|"
				i += 1
			print "< " + toSend
			test.SendToConnection(connection, toSend)
			echo = test.ReadFromConnection(connection)
			if (echo == toSend):
				print 'EQUAL'
			else:
				print 'NOT EQUAL'
			print "> " + echo
			print 'OK'
			return
	print 'Help:'
	print '	test-read-from-ftp ${host} ${port} ${user} ${password}'
	print '	test-read-from-ftpes ${host} ${port} ${user} ${password}'
	print '	test-ssl-client ${host} ${port}'

if __name__ == '__main__':
	Main()
