/**************************************************************************
 *   Created: 2007/02/09 23:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Exceptions.hpp 1049 2010-11-07 16:40:27Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE_Exception_h__0702110530
#define INCLUDED_FILE_Exception_h__0702110530

#include "UniquePtr.hpp"
#include "Api.h"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	//! General application exception.
	class TUNNELEX_CORE_API LocalException {
	public:
		explicit LocalException(const wchar_t *what) throw();
		LocalException(const LocalException &) throw();
		virtual ~LocalException() throw();
		LocalException & operator =(const LocalException &) throw();
	private:
		wchar_t const *m_what;
		bool m_doFree;
	public:
		virtual const wchar_t * GetWhat() const throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Logical exception.
	class TUNNELEX_CORE_API LogicalException : public LocalException {
	public:
		explicit LogicalException(const wchar_t *what) throw();
		LogicalException(const LogicalException &) throw();
		virtual ~LogicalException() throw();
		LogicalException & operator =(const LogicalException &) throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Object not found.
	class TUNNELEX_CORE_API NotFoundException : public LogicalException {
	public:
		explicit NotFoundException(const wchar_t *what) throw();
		NotFoundException(const NotFoundException &) throw();
		virtual ~NotFoundException() throw();
		NotFoundException & operator =(const NotFoundException &) throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Basic exception in connection working.
	class TUNNELEX_CORE_API ConnectionException : public LocalException {
	public:
		explicit ConnectionException(const wchar_t *what) throw();
		ConnectionException(const ConnectionException &) throw();
		virtual ~ConnectionException() throw();
		ConnectionException & operator =(
				const ConnectionException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//! Throws when connection opening has failed.
	class TUNNELEX_CORE_API ConnectionOpeningException : public ConnectionException {
	public:
		explicit ConnectionOpeningException(const wchar_t *what) throw();
		ConnectionOpeningException(
				const ConnectionOpeningException &)
			throw();
		virtual ~ConnectionOpeningException() throw();
		ConnectionOpeningException & operator =(
				const ConnectionOpeningException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	class SourceConnectionOpeningException
			: public ::TunnelEx::ConnectionOpeningException {
	public:
		explicit SourceConnectionOpeningException(
				const wchar_t *what)
			throw();
		SourceConnectionOpeningException(
				const SourceConnectionOpeningException &)
			throw();
		virtual ~SourceConnectionOpeningException() throw();
		SourceConnectionOpeningException & operator =(
				const SourceConnectionOpeningException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	class DestinationConnectionOpeningException
			: public ::TunnelEx::ConnectionOpeningException {
	public:
		explicit DestinationConnectionOpeningException(
				const wchar_t *what)
			throw();
		DestinationConnectionOpeningException(
				const DestinationConnectionOpeningException &)
			throw();
		virtual ~DestinationConnectionOpeningException() throw();
		DestinationConnectionOpeningException & operator =(
				const DestinationConnectionOpeningException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone()const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Exception after system error (usual critical).
	/** It can be OS error, TunnelEx installations error, data corruptions and so on. */
	class TUNNELEX_CORE_API SystemException : public LocalException {
	public:
		explicit SystemException(const wchar_t *what) throw();
		SystemException(const SystemException &) throw();
		virtual ~SystemException() throw();
		SystemException & operator =(const SystemException &) throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};
	
	//! Exception after insufficient memory error.
	class TUNNELEX_CORE_API InsufficientMemoryException : public SystemException {
	public:
		explicit InsufficientMemoryException(const wchar_t *what) throw();
		InsufficientMemoryException(const InsufficientMemoryException &) throw();
		virtual ~InsufficientMemoryException() throw();
		InsufficientMemoryException & operator =(const InsufficientMemoryException &) throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Throws when xml-string or xml-file is invalid or has invalid format.
	class TUNNELEX_CORE_API InvalidXmlException : public LocalException {
	public:
		explicit InvalidXmlException(const wchar_t *what) throw();
		InvalidXmlException(const InvalidXmlException &) throw();
		virtual ~InvalidXmlException() throw();
		InvalidXmlException & operator =(
				const InvalidXmlException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Throws when xml-file or xml-string does not match.
	class TUNNELEX_CORE_API XmlDoesNotMatchException : public InvalidXmlException {
	public:
		explicit XmlDoesNotMatchException(const wchar_t *what) throw();
		XmlDoesNotMatchException(const XmlDoesNotMatchException &) throw();
		virtual ~XmlDoesNotMatchException() throw();
		XmlDoesNotMatchException & operator =(
				const XmlDoesNotMatchException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! General exeption for ::TunnelEx::Endpoint end ::TunnelEx::Service.
	/** @sa	::TunnelEx::Endpoint 
		@sa	::TunnelEx::Service 
	  */
	class TUNNELEX_CORE_API EndpointException : public LocalException {
	public:
		explicit EndpointException(const wchar_t *what) throw();
		EndpointException(const EndpointException &) throw();
		virtual ~EndpointException() throw();
		EndpointException & operator =(const EndpointException &) throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Invalid endpoint link format, could not be parsed.
	/** @sa	::TunnelEx::Endpoint 
		@sa	::TunnelEx::Service 
	  */
	class TUNNELEX_CORE_API InvalidLinkException : public EndpointException {
	public:
		explicit InvalidLinkException(const wchar_t *what) throw();
		InvalidLinkException(const InvalidLinkException &) throw();
		virtual ~InvalidLinkException() throw();
		InvalidLinkException & operator =(const InvalidLinkException &) throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Requested and real type mismatch.
	/** @sa	::TunnelEx::Endpoint */
	class TUNNELEX_CORE_API EndpointAddressTypeMismatchException
		: public EndpointException {
	public:
		explicit EndpointAddressTypeMismatchException(
				const wchar_t *what)
			throw();
		EndpointAddressTypeMismatchException(
				const EndpointAddressTypeMismatchException &)
			throw();
		virtual ~EndpointAddressTypeMismatchException() throw();
		EndpointAddressTypeMismatchException & operator =(
				const EndpointAddressTypeMismatchException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Endpoint has not a Multi-Clients type.
	class TUNNELEX_CORE_API EndpointHasNotMultiClientsTypeException
		: public EndpointException {
	public:
		explicit EndpointHasNotMultiClientsTypeException(
				const wchar_t *what)
			throw();
		EndpointHasNotMultiClientsTypeException(
				const EndpointHasNotMultiClientsTypeException &)
			throw();
		virtual ~EndpointHasNotMultiClientsTypeException() throw();
		EndpointHasNotMultiClientsTypeException & operator =(
				const EndpointHasNotMultiClientsTypeException &)
			throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

	//! Error with DLL.
	class TUNNELEX_CORE_API DllException : public LocalException {
	public:
		explicit DllException(const wchar_t *what) throw();
		DllException(const DllException &) throw();
		virtual ~DllException() throw();
		DllException & operator =(const DllException &) throw();
		virtual ::TunnelEx::UniquePtr<::TunnelEx::LocalException> Clone() const;
	};

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE_Exception_h__0702110530
