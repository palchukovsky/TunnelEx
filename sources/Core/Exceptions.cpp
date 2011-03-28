/**************************************************************************
 *   Created: 2007/02/13 3:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Exceptions.cpp 1049 2010-11-07 16:40:27Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "Exceptions.hpp"

using namespace TunnelEx;

//////////////////////////////////////////////////////////////////////////

LocalException::LocalException(const wchar_t *what) throw()
		: m_doFree(false) {
	const size_t buffSize = (wcslen(what) + 1) * sizeof(wchar_t);
	m_what = static_cast<wchar_t *>(malloc(buffSize));
	if (m_what != NULL) {
		memcpy(const_cast<wchar_t *>(m_what), what, buffSize);
		m_doFree = true;
	} else {
		m_what = L"Memory allocation for exception description has been failed.";
	}
}

LocalException::~LocalException() throw() {
	if (m_doFree) {
		free(const_cast<wchar_t *>(m_what));
	}
}

LocalException::LocalException(const LocalException &rhs) throw() {
	m_doFree = rhs.m_doFree;
	if (m_doFree) {
		const size_t buffSize = (wcslen(rhs.m_what) + 1) * sizeof(wchar_t);
		m_what = static_cast<wchar_t *>(malloc(buffSize));
		if (m_what != NULL) {
			memcpy(const_cast<wchar_t *>(m_what), rhs.m_what, buffSize);
		} else {
			m_what = L"Memory allocation for exception description has been failed.";
		}
	} else {
		m_what = rhs.m_what;
	}
}

LocalException & LocalException::operator =(const LocalException &rhs) throw() {
	if (this != &rhs) {
		return *this;
	}
	m_doFree = rhs.m_doFree;
	if (m_doFree) {
		const size_t buffSize = (wcslen(rhs.m_what) + 1) * sizeof(wchar_t);
		m_what = static_cast<wchar_t *>(malloc(buffSize));
		if (m_what != NULL) {
			memcpy(const_cast<wchar_t *>(m_what), rhs.m_what, buffSize);
		} else {
			m_what = L"Memory allocation for exception description has been failed.";
		}
	} else {
		m_what = rhs.m_what;
	}
    return *this;
}


const wchar_t * LocalException::GetWhat() const throw() {
	return m_what;
}

UniquePtr<LocalException> LocalException::Clone() const {
	return UniquePtr<LocalException>(new LocalException(*this));
}

//////////////////////////////////////////////////////////////////////////

LogicalException::LogicalException(const wchar_t *what) throw()
		: LocalException(what) {
	//...//
}

LogicalException::LogicalException(const LogicalException &rhs) throw()
		: LocalException(rhs) {
	//...//
}

LogicalException::~LogicalException() throw() {
	//...//
}

LogicalException & LogicalException::operator =(
			const LogicalException &rhs)
		throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> LogicalException::Clone() const {
	return UniquePtr<LocalException>(new LogicalException(*this));
}

//////////////////////////////////////////////////////////////////////////

NotFoundException::NotFoundException(const wchar_t *what) throw()
		: LogicalException(what) {
	//...//
}

NotFoundException::NotFoundException(const NotFoundException &rhs) throw()
		: LogicalException(rhs) {
	//...//
}

NotFoundException::~NotFoundException() throw() {
	//...//
}

NotFoundException & NotFoundException::operator =(
			const NotFoundException &rhs)
		throw() {
	LogicalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> NotFoundException::Clone() const {
	return UniquePtr<LocalException>(new NotFoundException(*this));
}

//////////////////////////////////////////////////////////////////////////

ConnectionException::ConnectionException(const wchar_t *what) throw()
		: LocalException(what) {
	//...//
}

ConnectionException::ConnectionException(
			const ConnectionException &rhs)
		throw()
		: LocalException(rhs) {
	//...//
}

ConnectionException::~ConnectionException() throw() {
	//...//
}

ConnectionException & ConnectionException::operator =(
			const ConnectionException &rhs)
		throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> ConnectionException::Clone() const {
	return UniquePtr<LocalException>(new ConnectionException(*this));
}

//////////////////////////////////////////////////////////////////////////
ConnectionOpeningException::ConnectionOpeningException(
			const wchar_t *what)
		throw()
		: ConnectionException(what) {
	//...//
}

ConnectionOpeningException::ConnectionOpeningException(
			const ConnectionOpeningException &rhs)
		throw()
		: ConnectionException(rhs) {
	//...//
}

ConnectionOpeningException::~ConnectionOpeningException() throw() {
	//...//
}

ConnectionOpeningException & ConnectionOpeningException::operator =(
			const ConnectionOpeningException &rhs)
		throw() {
	ConnectionException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> ConnectionOpeningException::Clone() const {
	return UniquePtr<LocalException>(new ConnectionOpeningException(*this));
}

//////////////////////////////////////////////////////////////////////////

SourceConnectionOpeningException::SourceConnectionOpeningException(
			const wchar_t *what) throw()
		: ConnectionOpeningException(what) {
	//...//
}

SourceConnectionOpeningException::SourceConnectionOpeningException(
			const SourceConnectionOpeningException &rhs)
		throw()
		: ConnectionOpeningException(rhs) {
	//...//
}

SourceConnectionOpeningException::~SourceConnectionOpeningException() throw() {
	//...//
}

SourceConnectionOpeningException & SourceConnectionOpeningException::operator =(
			const SourceConnectionOpeningException &rhs)
		throw() {
	ConnectionOpeningException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> SourceConnectionOpeningException::Clone() const {
	return UniquePtr<LocalException>(new SourceConnectionOpeningException(*this));
}

//////////////////////////////////////////////////////////////////////////

DestinationConnectionOpeningException::DestinationConnectionOpeningException(
			const wchar_t *what) throw()
		: ConnectionOpeningException(what) {
	//...//
}

DestinationConnectionOpeningException::DestinationConnectionOpeningException(
			const DestinationConnectionOpeningException &rhs)
		throw()
		: ConnectionOpeningException(rhs) {
	//...//
}

DestinationConnectionOpeningException::~DestinationConnectionOpeningException() throw() {
	//...//
}

DestinationConnectionOpeningException & DestinationConnectionOpeningException::operator =(
			const DestinationConnectionOpeningException &rhs)
		throw() {
	ConnectionOpeningException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> DestinationConnectionOpeningException::Clone() const {
	return UniquePtr<LocalException>(
		new DestinationConnectionOpeningException(*this));
}

//////////////////////////////////////////////////////////////////////////

SystemException::SystemException(const wchar_t *what) throw()
		: LocalException(what) {
	//...//
}

SystemException::SystemException(const SystemException &rhs) throw()
		: LocalException(rhs) {
	//...//
}

SystemException::~SystemException() throw() {
	//...//
}

SystemException & SystemException::operator =(
			const SystemException &rhs)
		throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> SystemException::Clone() const {
	return UniquePtr<LocalException>(new SystemException(*this));
}

//////////////////////////////////////////////////////////////////////////

InsufficientMemoryException::InsufficientMemoryException(
			const wchar_t *what)
		throw()
		: SystemException(what) {
	//...//
}

InsufficientMemoryException::InsufficientMemoryException(
			const InsufficientMemoryException &rhs)
		throw()
		: SystemException(rhs) {
	//...//
}

InsufficientMemoryException::~InsufficientMemoryException() throw() {
	//...//
}

InsufficientMemoryException & InsufficientMemoryException::operator =(
			const InsufficientMemoryException &rhs)
		throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> InsufficientMemoryException::Clone() const {
	return UniquePtr<LocalException>(new InsufficientMemoryException(*this));
}

//////////////////////////////////////////////////////////////////////////

InvalidXmlException::InvalidXmlException(const wchar_t *what) throw()
		: LocalException(what) {
	//...//
}

InvalidXmlException::InvalidXmlException(const InvalidXmlException &rhs) throw()
		: LocalException(rhs) {
	//...//
}

InvalidXmlException::~InvalidXmlException() throw() {
	//...//
}

InvalidXmlException & InvalidXmlException::operator =(
			const InvalidXmlException &rhs)
		throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> InvalidXmlException::Clone() const {
	return UniquePtr<LocalException>(new InvalidXmlException(*this));
}

//////////////////////////////////////////////////////////////////////////

XmlDoesNotMatchException::XmlDoesNotMatchException(const wchar_t *what) throw()
		: InvalidXmlException(what) {
	//...//
}

XmlDoesNotMatchException::XmlDoesNotMatchException(
			const XmlDoesNotMatchException &rhs)
		throw()
		: InvalidXmlException(rhs) {
	//...//
}

XmlDoesNotMatchException::~XmlDoesNotMatchException() throw() {
	//...//
}

XmlDoesNotMatchException & XmlDoesNotMatchException::operator =(
			const XmlDoesNotMatchException &rhs)
		throw() {
	InvalidXmlException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> XmlDoesNotMatchException::Clone() const {
	return UniquePtr<LocalException>(new XmlDoesNotMatchException(*this));
}

//////////////////////////////////////////////////////////////////////////

EndpointException::EndpointException(const wchar_t *what) throw()
		: LocalException(what) {
	//...//
}

EndpointException::EndpointException(const EndpointException &rhs) throw()
		: LocalException(rhs) {
	//...//
}

EndpointException::~EndpointException() throw() {
	//...//
}

EndpointException & EndpointException::operator =(
			const EndpointException &rhs)
		throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> EndpointException::Clone() const {
	return UniquePtr<LocalException>(new EndpointException(*this));
}

//////////////////////////////////////////////////////////////////////////

InvalidLinkException::InvalidLinkException(
			const wchar_t *what)
		throw()
		: EndpointException(what) {
	//...//
}

InvalidLinkException::InvalidLinkException(
			const InvalidLinkException &rhs)
		throw()
		: EndpointException(rhs) {
	//...//
}

InvalidLinkException::~InvalidLinkException() throw() {
	//...//
}

InvalidLinkException & InvalidLinkException::operator =(
			const InvalidLinkException &rhs)
		throw() {
	EndpointException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> InvalidLinkException::Clone() const {
	return UniquePtr<LocalException>(new InvalidLinkException(*this));
}

//////////////////////////////////////////////////////////////////////////

EndpointAddressTypeMismatchException::EndpointAddressTypeMismatchException(
			const wchar_t *what)
		throw()
		: EndpointException(what) {
	//...//
}

EndpointAddressTypeMismatchException::EndpointAddressTypeMismatchException(
			const EndpointAddressTypeMismatchException &rhs)
		throw()
		: EndpointException(rhs) {
	//...//
}

EndpointAddressTypeMismatchException::~EndpointAddressTypeMismatchException() throw() {
	//...//
}

EndpointAddressTypeMismatchException & EndpointAddressTypeMismatchException::operator =(
			const EndpointAddressTypeMismatchException &rhs)
		throw() {
	EndpointException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> EndpointAddressTypeMismatchException::Clone() const {
	return UniquePtr<LocalException>(new EndpointAddressTypeMismatchException(*this));
}

//////////////////////////////////////////////////////////////////////////


EndpointHasNotMultiClientsTypeException::EndpointHasNotMultiClientsTypeException(
			const wchar_t *what)
		throw()
		: EndpointException(what) {
	//...//
}

EndpointHasNotMultiClientsTypeException::EndpointHasNotMultiClientsTypeException(
			const EndpointHasNotMultiClientsTypeException &rhs)
		throw()
		: EndpointException(rhs) {
	//...//
}

EndpointHasNotMultiClientsTypeException::~EndpointHasNotMultiClientsTypeException()
		throw() {
	//...//
}

EndpointHasNotMultiClientsTypeException & EndpointHasNotMultiClientsTypeException::operator =(
			const EndpointHasNotMultiClientsTypeException &rhs)
		throw() {
	EndpointException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> EndpointHasNotMultiClientsTypeException::Clone()
		const {
	return UniquePtr<LocalException>(
		new EndpointHasNotMultiClientsTypeException(*this));
}

//////////////////////////////////////////////////////////////////////////

DllException::DllException(const wchar_t *what) throw()
		: LocalException(what) {
	//...//
}

DllException::DllException(const DllException &rhs) throw()
		: LocalException(rhs) {
	//...//
}

DllException::~DllException() throw() {
	//...//
}

DllException & DllException::operator =(const DllException &rhs) throw() {
	LocalException::operator =(rhs);
	return *this;
}

UniquePtr<LocalException> DllException::Clone() const {
	return UniquePtr<LocalException>(new DllException(*this));
}

//////////////////////////////////////////////////////////////////////////
