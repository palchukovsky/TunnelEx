'''
	Module: TunnelEx.Test.Ftp
	Created: Feb 7, 2012 12:48:37 PM
	Author: Eugene V. Palchukovsky
	E-mail: eugene@palchukovsky.com
	-------------------------------------------------------------------
	Project: TunnelEx
	URL: http://tunnelex.net
'''

import os
import md5
from ftplib import FTP


class Client:

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

	def StoreWelcome(self, filePath):
		f = open(filePath, 'w+')
		f.write(self._ftp.getwelcome())

	def StoreStruct(self, file_path):
		f = open(file_path, 'w+')
		self._StorePathStruct(f, [])

	def _StorePathStruct(self, db, path):

		cd = self._ftp.pwd()
		db.write("\ndir: %s\n" % cd)

		lines = []
		result = self._ftp.retrlines('LIST', lines.append)
		if result.split()[0] != '226': raise Client.Error('LIST', result)

		db.write("\tstruct:\n")
		for l in lines: db.write("\t\t%s\n" % l)

		db.write("\tfiles:\n")
		for l in lines:
			if l[0] != 'd':
				fileName = ' '.join(l.split()[8:])
				fileHash = md5.new()
				cmd = 'RETR %s' % fileName
				result = self._ftp.retrbinary(cmd, fileHash.update)
				if result.split()[0] != '226': raise Client.Error(cmd, result)
				db.write('\t\t{0}\t{1}\n'.format(fileName, fileHash.hexdigest()))

		for l in lines:
			if l[0] == 'd':
				subPath = list(path)
				subPath.append(' '.join(l.split()[8:]))
				fullPath = '/' + '/'.join(subPath)
				self._ftp.cwd(fullPath)
				self._StorePathStruct(db, subPath)


class Struct:

	def __init__(self, homePath):
		self.homePath = homePath
		self.structFilePath = os.path.join(self.homePath, 'struct.txt')
		self.welcomeFilePath = os.path.join(self.homePath, 'welcome.txt')

	def Create(self, host, login, password):

		if os.access(self.homePath, os.F_OK) == False:
			os.makedirs(self.homePath)

		ftp = Client(host, login, password)
		ftp.StoreWelcome(self.welcomeFilePath)
		ftp.StoreStruct(self.structFilePath)


def CreateMasterCopy(host, login, password):
	struct = Struct(os.path.join('.', 'Temp', 'Ftp', 'Standard'))
	struct.create(host, login, password)

def ShowHelp():
	print '%s:' % __name__
	print '	create_master_copy ${host} ${user} ${password}'

def Execute(argv):
	actions = {
		'create_master_copy': CreateMasterCopy,
		'help': ShowHelp
	}
	try:
		action = actions[argv[1]]
	except IndexError:
		print "Error: Invalid command."
		ShowHelp()
		return
	action(*argv[2:])
