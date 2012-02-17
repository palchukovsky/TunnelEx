'''
	Module: TunnelEx.Test.Ftp
	Created: Feb 12, 2012 7:44:50 PM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

import md5
from ftplib import FTP
from M2Crypto.ftpslib import FTP_TLS

class Connection:

	class Error:
		def __init__(self, cmd, value):
			self.cmd = cmd
			self.result = value
		def __str__(self):
			return repr('Failed to "{0}": "{1}"'.format(self.cmd, self.result))

	def __init__(self, ftp, isPassive = None):
		self._ftp = ftp
		if isPassive is not None:
			self._ftp.set_pasv(isPassive)

	def Close(self):
		self._ftp.quit()

	def ReadFilesStruct(self):
		db = dict()
		db['welcome'] = self._ftp.getwelcome()
		db['content'] = dict()
		self._GetContent([], db['content'])
		return db

	def _GetContent(self, path, db):

		cd = self._ftp.pwd()
		db[cd] = dict()
		dirDb = db[cd]

		lines = list()
		result = self._ftp.retrlines('LIST', lines.append)
		if result.split()[0] != '226':
			raise Connection.Error('LIST', result)

		dirDb['struct'] = list()
		for l in lines:
			dirDb['struct'].append(l)

		dirDb['files'] = dict()
		for l in lines:
			if l[0] != 'd':
				fileName = ' '.join(l.split()[8:])
				fileHash = md5.new()
				cmd = 'RETR %s' % fileName
				result = self._ftp.retrbinary(cmd, fileHash.update)
				if result.split()[0] != '226':
					raise Connection.Error(cmd, result)
				dirDb['files'][fileName] = fileHash.hexdigest()

		for l in lines:
			if l[0] == 'd':
				subPath = list(path)
				subPath.append(' '.join(l.split()[8:]))
				fullPath = '/' + '/'.join(subPath)
				self._ftp.cwd(fullPath)
				self._GetContent(subPath, db)

class PlainConnection(Connection):

	def __init__(self, host, port, user, password, isPassive = None):
		ftp = FTP()
		ftp.connect(host, port)
		ftp.login(user, password)
		Connection.__init__(self, ftp, isPassive)

class SecureConnection(Connection):

	def __init__(
			self
			, host
			, port
			, user
			, password
			, isPassive = None
			, isTransferConnectionProtected = True):
		ftp = FTP_TLS()
		ftp.connect(host, port)
		ftp.auth_tls()
		ftp.login(user, password)
		if isTransferConnectionProtected == True:
			ftp.prot_p()
		Connection.__init__(self, ftp, isPassive)

class Struct:

	def __init__(self, connection):
		self._struct = connection.ReadFilesStruct()

	def Compare(self, rhs):
		if not 'welcome' in self._struct:
			return False
		elif len(self._struct['welcome']) == 0:
			return False
		elif not 'content' in self._struct:
			return False
		elif len(self._struct['content']) == 0:
			return False
		else:
			return self._struct == rhs._struct

	def Dump(self):
		result = self._struct['welcome'] + ":\n"
		for dirPath, dirContent in self._struct['content'].iteritems():
			result += '\n{0}\n'.format(dirPath)
			result += "\tstruct:\n"
			for l in dirContent['struct']: result += '\t\t{0}\n'.format(l)
			result += "\tfiles:\n"
			for name, hexdigest in dirContent['files'].iteritems():
				result += "\t\t{0:<32}{1}\n".format(name + ":", hexdigest)
		return result
