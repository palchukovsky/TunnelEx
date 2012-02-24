
import sys
import socket
from M2Crypto import SSL
from TunnelEx.Test import Ftp

class Test:

	class UnknownParameterType(Exception):
		pass

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
		result.settimeout(5)
		result.connect((str('localhost'), int(101)))
		return result

	def ConnectToSslServer(self, host, port):
		"""Creates SSL connection to server and returns as object."""
		context = SSL.Context()
		context.set_verify(SSL.verify_peer | SSL.verify_fail_if_no_peer_cert, depth = 9)
		if context.load_verify_locations('ca.pem') != 1:
			raise Exception('No CA certificate')
		result = SSL.Connection(context)
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
		if isinstance(connection, socket.socket):
			connection.send(data)
		else:
			raise Test.UnknownParameterType

	def ReadFromConnection(self, connection):
		"""Read data from connection."""
		if isinstance(connection, socket.socket):
			return connection.recv(1024)
		else:
			raise Test.UnknownParameterType

	def Disconnect(self, connection):
		"""Closes connection."""
		if isinstance(connection, Ftp.PlainConnection) or isinstance(connection, Ftp.SecureConnection):
			connection.Close()
		elif isinstance(connection, socket.socket):
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
	print 'Help:'
	print '	test-read-from-ftp ${host} ${port} ${user} ${password}'
	print '	test-read-from-ftpes ${host} ${port} ${user} ${password}'

if __name__ == '__main__':
	Main()
