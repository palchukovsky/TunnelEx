
import sys
from TunnelEx.Test import Ftp

class Test:

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

	def DisconnectFromFtpServer(self, connection):
		"""Closes FTP server connect."""
		return connection.Close()

	def ReadFilesStructureFromFtpServer(self, connection):
		"""Reads files structure and returns as object."""
		return Ftp.Struct(connection)

	def CompareFtpServersFilesStructures(self, server1, server2):
		"""Compares two FTP servers files structures and returns True if they are identical."""
		return server1.Compare(server2)

	def DumpFtpServerFilesStructure(self, server):
		"""Dumps the FTP server files structure into a string."""
		return server.Dump()

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
