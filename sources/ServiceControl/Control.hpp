/**************************************************************************
 *   Created: 2008/01/05 13:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Control_h__0801051317
#define INCLUDED_FILE__TUNNELEX__Control_h__0801051317

class ServiceControl : private boost::noncopyable {

public:

	class ServiceControlException {
	public:
		ServiceControlException(unsigned long error) : m_error(error) {/*...*/}
		unsigned long GetError() const {return m_error;}
	private:
		unsigned long m_error;
	};
	class ServiceControlAccessDeniedException : public ServiceControlException {
	public:
		ServiceControlAccessDeniedException(unsigned long error) : ServiceControlException(error) {/*...*/}
	};
	class ServiceControlServiceDoesntExistException : public ServiceControlException {
	public:
		ServiceControlServiceDoesntExistException(unsigned long error) : ServiceControlException(error) {/*...*/}
	};

public:
	
	ServiceControl(); /* throw(ServiceControlAccessDeniedException, ServiceControlException)*/
	~ServiceControl(); /* throw() */

public:

	bool Start(); /* throw(ServiceControlAccessDeniedException, ServiceControlException)*/
	bool Stop(); /* throw(ServiceControlAccessDeniedException, ServiceControlException)*/
	
	bool IsStarted() const; /* throw(ServiceControlAccessDeniedException, ServiceControlException)*/
	bool IsStopped() const; /* throw(ServiceControlAccessDeniedException, ServiceControlException)*/

	bool WaitStartPending() const; /* throw(ServiceControlAccessDeniedException, ServiceControlException)*/
	bool WaitStopPending() const; /* throw(ServiceControlAccessDeniedException, ServiceControlException)*/

private:

	class Implementation;

};

#endif // INCLUDED_FILE__TUNNELEX__Control_h__0801051317