
from TunnelEx.Test import Ftp

class Test:

	def ReadStructureFromFtpServer(self, host, login, password):
		"""Connects to the FTP server, reads files structure and returns as object."""
		d = Ftp.Struct(host, login, password)
		return d

	def CompareFtpServersStructures(self, server1, server2):
		"""Compares two FTP servers files structures and returns True if they are identical."""
		return server1.Compare(server2)

	def DumpFtpServerStructure(self, server):
		"""Dumps the FTP server files structure into a string."""
		return server.Dump()
