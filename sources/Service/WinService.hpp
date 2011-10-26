/**************************************************************************
 *   Created: 2007/12/27 0:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2009 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Service_h__0712270036
#define INCLUDED_FILE__TUNNELEX__Service_h__0712270036

#include "Core/Log.hpp"

class TexServiceImplementation;

void RunAsTexService();
bool InstallTexService();
bool UninstallTexService();

class TexWinService : private boost::noncopyable {
public:
	explicit TexWinService(
			boost::optional<TunnelEx::LogLevel> forcedLogLevel = boost::optional<TunnelEx::LogLevel>());
	~TexWinService();
private:
	mutable soap m_soap;
	boost::thread_group m_adminThreads;
	typedef std::set<soap*> Connections;
	Connections m_connections;
	boost::mutex m_connectionRemoveMutex;
public:
	void Run(HANDLE stopEvent);
	static boost::shared_ptr<TexServiceImplementation> GetTexServiceInstance();
	static boost::shared_ptr<TexServiceImplementation> m_texService;
protected:
	void RunSoapServer(HANDLE stopEvent);
private:
	void HandleSoapRequest();
	void SoapServeThread(soap *);
	void LogSoapError() const ;
	void LogUnexpectedEvent(const DWORD) const;
};

#endif // INCLUDED_FILE__TUNNELEX__Service_h__0712270036
