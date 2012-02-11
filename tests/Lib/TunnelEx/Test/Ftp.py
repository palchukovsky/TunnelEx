'''
	Module: TunnelEx.Test.Ftp
	Created: Feb 7, 2012 12:48:37 PM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

import md5
from ftplib import FTP


class _Client:

	class Error:
		def __init__(self, cmd, value):
			self.cmd = cmd
			self.result = value
		def __str__(self):
			return repr('Failed to "{0}": "{1}"'.format(self.cmd, self.result))

	def __init__(self, host, login, password):
		self._ftp = FTP()
		self._ftp.connect(host)
		self._ftp.login(login, password)

	def GetDump(self):
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
		if result.split()[0] != '226': raise _Client.Error('LIST', result)

		dirDb['struct'] = list()
		for l in lines: dirDb['struct'].append(l)

		dirDb['files'] = dict()
		for l in lines:
			if l[0] != 'd':
				fileName = ' '.join(l.split()[8:])
				fileHash = md5.new()
				cmd = 'RETR %s' % fileName
				result = self._ftp.retrbinary(cmd, fileHash.update)
				if result.split()[0] != '226': raise _Client.Error(cmd, result)
				dirDb['files'][fileName] = fileHash.hexdigest()

		for l in lines:
			if l[0] == 'd':
				subPath = list(path)
				subPath.append(' '.join(l.split()[8:]))
				fullPath = '/' + '/'.join(subPath)
				self._ftp.cwd(fullPath)
				self._GetContent(subPath, db)


class Struct:

	def __init__(self, host, login, password):
		self._struct = _Client(host, login, password).GetDump()

	def Compare(self, rhs):
		if self._struct['welcome'] != rhs._struct['welcome']: return False;
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
