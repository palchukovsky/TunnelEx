/**************************************************************************
*                                                                         *
*   Created: 2007/06/12 15:28                                             *
*     Author: Eugene V. Palchukovsky                                      *
*     E-mail: eugene@palchukovsky.com                                     *
* -------------------------------------------------------------------     *
*    Project: TunnelEx                                                    *
*                                                                         *
**************************************************************************/

#include "Prec.h"

#include "Endpoint.hpp"
#include "EndpointAddress.hpp"
#include "ModulesFactory.hpp"

//////////////////////////////////////////////////////////////////////////

using namespace boost;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;

//////////////////////////////////////////////////////////////////////////

class Endpoint::Implementation {

public:

	Implementation()
			: m_acceptor() {
		//...//
	}

	explicit Implementation(const WString &combinedResourceIdentifier, bool isAcceptor)
			: m_cachedCombinedOrReadResourceIdentifier(combinedResourceIdentifier),
			m_acceptor(isAcceptor ? Endpoint::ACCEPTOR_READER : Endpoint::ACCEPTOR_NONE) {
		//...//
	}

	explicit Implementation(
				const WString &readResourceIdentifier,
				const WString &writeResourceIdentifier,
				Endpoint::Acceptor acceptor)
			: m_cachedCombinedOrReadResourceIdentifier(readResourceIdentifier),
			m_cachedWriteResourceIdentifier(writeResourceIdentifier),
			m_acceptor(acceptor) {
		BOOST_ASSERT(
			!m_cachedCombinedOrReadResourceIdentifier.IsEmpty()
			&& !m_cachedWriteResourceIdentifier.IsEmpty());
	}

	explicit Implementation(UniquePtr<EndpointAddress> combinedAddress, bool isAcceptor) throw()
			: m_combinedOrReadAddress(combinedAddress),
			m_acceptor(isAcceptor ? Endpoint::ACCEPTOR_READER : Endpoint::ACCEPTOR_NONE) {
		//...//
	}

	explicit Implementation(
				UniquePtr<EndpointAddress> readAddress,
				UniquePtr<EndpointAddress> writeAddress,
				Endpoint::Acceptor acceptor)
			throw()
			: m_combinedOrReadAddress(readAddress),
			m_writeAddress(writeAddress),
			m_acceptor(acceptor) {
		BOOST_ASSERT(m_combinedOrReadAddress && m_writeAddress);
	}

	Implementation(const Implementation &rhs)
			: m_cachedCombinedOrReadResourceIdentifier(
				rhs.m_cachedCombinedOrReadResourceIdentifier),
			m_cachedWriteResourceIdentifier(
				rhs.m_cachedWriteResourceIdentifier),
			m_acceptor(rhs.m_acceptor) {
		if (rhs.m_combinedOrReadAddress) {
			UniquePtr<EndpointAddress> clone(rhs.m_combinedOrReadAddress->Clone());
			m_combinedOrReadAddress = clone;
		}
		if (rhs.m_writeAddress) {
			UniquePtr<EndpointAddress> clone(rhs.m_writeAddress->Clone());
			m_writeAddress = clone;
		}
	}

public:

	template<bool isCombined>
	void CheckType() const throw(EndpointAddressTypeMismatchException) {
		BOOST_ASSERT(x);
	}
	template<>
	void CheckType<false>() const throw(EndpointAddressTypeMismatchException) {
		BOOST_ASSERT(!IsCombined());
		if (IsCombined()) {
			throw EndpointAddressTypeMismatchException(
				L"Wrong endpoint type requested (not combined).");
		}
	}
	template<>
	void CheckType<true>() const throw(EndpointAddressTypeMismatchException) {
		BOOST_ASSERT(IsCombined());
		if (!IsCombined()) {
			throw EndpointAddressTypeMismatchException(
				L"Wrong endpoint type requested (combined).");
		}
	}

	bool IsCombined() const {
		return m_cachedWriteResourceIdentifier.IsEmpty() && !m_writeAddress;
	}

private:

	const Implementation & operator =(const Implementation &);

public:

	SharedPtr<EndpointAddress> m_combinedOrReadAddress;
	SharedPtr<EndpointAddress> m_writeAddress;

	WString m_cachedCombinedOrReadResourceIdentifier;
	WString m_cachedWriteResourceIdentifier;

	const Endpoint::Acceptor m_acceptor;

};

//////////////////////////////////////////////////////////////////////////

Endpoint::Endpoint()
		: m_pimpl(new Implementation) {
	//...//
}

Endpoint::Endpoint(const WString &combinedResourceIdentifier, bool isAcceptor)
		: m_pimpl(new Implementation(combinedResourceIdentifier, isAcceptor)) {
	//...//
}

Endpoint::Endpoint(
			const WString &readResourceIdentifier,
			const WString &writeResourceIdentifier,
			Acceptor acceptor)
		: m_pimpl(
			new Implementation(
				readResourceIdentifier,
				writeResourceIdentifier,
				acceptor)) {
	//...//
}

Endpoint::Endpoint(UniquePtr<EndpointAddress> combinedAddress, bool isAcceptor)
		: m_pimpl(new Implementation(combinedAddress, isAcceptor)) {
	//...//
}

Endpoint::Endpoint(
			UniquePtr<EndpointAddress> readAddress,
			UniquePtr<EndpointAddress> writeAddress,
			Acceptor acceptor)
		: m_pimpl(new Implementation(readAddress, writeAddress, acceptor)) {
	//...//
}

Endpoint::~Endpoint() throw() {
	delete m_pimpl;
}

Endpoint::Endpoint(const Endpoint &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

const Endpoint & Endpoint::operator =(const Endpoint &rhs) {
	Endpoint newObj(rhs);
	Swap(newObj);
	return *this;
}

void Endpoint::Swap(Endpoint &rhs) throw() {
	Implementation *oldImpl(m_pimpl);
	m_pimpl = rhs.m_pimpl;
	rhs.m_pimpl = oldImpl;
}

bool Endpoint::IsCombined() const {
	return m_pimpl->IsCombined();
}

SharedPtr<const EndpointAddress> Endpoint::GetCombinedAddress()
		const
		throw(
			InvalidLinkException,
			EndpointAddressTypeMismatchException) {
	return (const_cast<Endpoint *>(this))->GetCombinedAddress();
}

SharedPtr<EndpointAddress> Endpoint::GetCombinedAddress()
		throw(
			InvalidLinkException,
			EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<true>();
	if (	!m_pimpl->m_combinedOrReadAddress
			&& !m_pimpl->m_cachedCombinedOrReadResourceIdentifier.IsEmpty()) {
		SharedPtr<EndpointAddress> address(
			ModulesFactory::GetInstance()
				.CreateEndpointAddress(m_pimpl->m_cachedCombinedOrReadResourceIdentifier)
					.Release());
		m_pimpl->m_cachedCombinedOrReadResourceIdentifier.Clear();
		m_pimpl->m_combinedOrReadAddress = address;
	}
	return m_pimpl->m_combinedOrReadAddress;
}

SharedPtr<const EndpointAddress> Endpoint::GetReadAddress()
		const
		throw(
			InvalidLinkException,
			EndpointAddressTypeMismatchException) {
	return (const_cast<Endpoint *>(this))->GetReadAddress();
}

SharedPtr<EndpointAddress> Endpoint::GetReadAddress()
		throw(
			InvalidLinkException,
			EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<false>();
	if (	!m_pimpl->m_combinedOrReadAddress
			&& !m_pimpl->m_cachedCombinedOrReadResourceIdentifier.IsEmpty()) {
		SharedPtr<EndpointAddress> address(
			ModulesFactory::GetInstance()
				.CreateEndpointAddress(m_pimpl->m_cachedCombinedOrReadResourceIdentifier)
					.Release());
		m_pimpl->m_cachedCombinedOrReadResourceIdentifier.Clear();
		m_pimpl->m_combinedOrReadAddress = address;
	}
	return m_pimpl->m_combinedOrReadAddress;
}

SharedPtr<const EndpointAddress> Endpoint::GetWriteAddress()
		const
		throw(
			InvalidLinkException,
			EndpointAddressTypeMismatchException) {
	return (const_cast<Endpoint *>(this))->GetWriteAddress();
}

SharedPtr<EndpointAddress> Endpoint::GetWriteAddress()
		throw(
			InvalidLinkException,
			EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<false>();
	if (	!m_pimpl->m_writeAddress
			&& !m_pimpl->m_cachedWriteResourceIdentifier.IsEmpty()) {
		SharedPtr<EndpointAddress> address(
			ModulesFactory::GetInstance()
				.CreateEndpointAddress(m_pimpl->m_cachedWriteResourceIdentifier)
					.Release());
		m_pimpl->m_cachedWriteResourceIdentifier.Clear();
		m_pimpl->m_writeAddress = address;
	}
	return m_pimpl->m_writeAddress;
}

void Endpoint::SetCombinedAddress(UniquePtr<EndpointAddress> address, bool isAcceptor) {
	Endpoint(address, isAcceptor).Swap(*this);
}

void Endpoint::SetCombinedResourceIdentifier(const WString &ri, bool isAcceptor) {
	Endpoint(ri, isAcceptor).Swap(*this);
}

const WString & Endpoint::GetCombinedResourceIdentifier()
		const
		throw(::TunnelEx::EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<true>();
	return m_pimpl->m_combinedOrReadAddress
		?	m_pimpl->m_combinedOrReadAddress->GetResourceIdentifier()
		:	m_pimpl->m_cachedCombinedOrReadResourceIdentifier;
}

const WString & Endpoint::GetReadResourceIdentifier()
		const
		throw(::TunnelEx::EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<false>();
	return m_pimpl->m_combinedOrReadAddress
		?	m_pimpl->m_combinedOrReadAddress->GetResourceIdentifier()
		:	m_pimpl->m_cachedCombinedOrReadResourceIdentifier;
}

const WString & Endpoint::GetWriteResourceIdentifier()
		const
		throw(::TunnelEx::EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<false>();
	return m_pimpl->m_writeAddress
		?	m_pimpl->m_writeAddress->GetResourceIdentifier()
		:	m_pimpl->m_cachedWriteResourceIdentifier;
}

void Endpoint::SetReadWriteAddresses(
			::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> ra,
			::TunnelEx::UniquePtr<::TunnelEx::EndpointAddress> wa,
			Acceptor acceptor) {
	Endpoint(ra, wa, acceptor).Swap(*this);
}

void Endpoint::SetReadWriteResourceIdentifiers(
			const WString &rri,
			const WString &wri,
			Acceptor acceptor) {
	Endpoint(rri, wri, acceptor).Swap(*this);
}

bool Endpoint::IsCombinedAcceptor()
		const
		throw(::TunnelEx::EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<true>();
	return m_pimpl->m_acceptor != ACCEPTOR_NONE;
}

Endpoint::Acceptor Endpoint::GetReadWriteAcceptor()
		const 
		throw(::TunnelEx::EndpointAddressTypeMismatchException) {
	m_pimpl->CheckType<false>();
	return m_pimpl->m_acceptor;
}

//////////////////////////////////////////////////////////////////////////

class RuleEndpoint::Implementation {

public:

	Implementation(const WString *uuidStr = 0)
			: m_uuid(uuidStr ? *uuidStr : Uuid().GetAsString().c_str()) {
		//...//
	}

private:

	const Implementation & operator =(const Implementation &);

public:

	RuleEndpoint::Listeners m_preListeners;
	RuleEndpoint::Listeners m_postListeners;
	WString m_uuid;

};

//////////////////////////////////////////////////////////////////////////

RuleEndpoint::RuleEndpoint(const WString *uuid)
		: m_pimpl(new Implementation(uuid)) {
	//...//
}

RuleEndpoint::RuleEndpoint(
			const WString &resourceIdentifier,
			bool isAcceptor,
			const WString *uuid)
		: Endpoint(resourceIdentifier, isAcceptor),
		m_pimpl(new Implementation(uuid)) {
	//...//
}

RuleEndpoint::RuleEndpoint(
			const WString &ri,
			const WString &wi,
			Endpoint::Acceptor acceptor,
			const WString *uuid)
		: Endpoint(ri, wi, acceptor),
		m_pimpl(new Implementation(uuid)) {
	//...//
}

RuleEndpoint::RuleEndpoint(
			UniquePtr<EndpointAddress> r,
			UniquePtr<EndpointAddress> w,
			Endpoint::Acceptor acceptor,
			const WString *uuid)
		: Endpoint(r, w, acceptor),
		m_pimpl(new Implementation(uuid)) {
	//...//
}

RuleEndpoint::RuleEndpoint(
			UniquePtr<EndpointAddress> address,
			bool isAcceptor,
			const WString *uuid)
		: Endpoint(address, isAcceptor),
		m_pimpl(new Implementation(uuid)) {
	//...//
}

RuleEndpoint::~RuleEndpoint() throw() {
	delete m_pimpl;
}

RuleEndpoint::RuleEndpoint(const RuleEndpoint &rhs)
		: Endpoint(rhs),
		m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

const RuleEndpoint & RuleEndpoint::operator =(const RuleEndpoint &rhs) {
	RuleEndpoint newObj(rhs);
	Swap(newObj);
	return *this;
}

void RuleEndpoint::Swap(RuleEndpoint &rhs) throw() {
	Implementation *oldImpl(m_pimpl);
	Endpoint::Swap(rhs);
	m_pimpl = rhs.m_pimpl;
	rhs.m_pimpl = oldImpl;
}

const RuleEndpoint::Listeners & RuleEndpoint::GetPreListeners() const {
	return m_pimpl->m_preListeners;
}

RuleEndpoint::Listeners & RuleEndpoint::GetPreListeners() {
	return m_pimpl->m_preListeners;
}

const RuleEndpoint::Listeners & RuleEndpoint::GetPostListeners() const {
	return m_pimpl->m_postListeners;
}

RuleEndpoint::Listeners & RuleEndpoint::GetPostListeners() {
	return m_pimpl->m_postListeners;
}

const WString & RuleEndpoint::GetUuid() const {
	return m_pimpl->m_uuid;
}

TimeSeconds RuleEndpoint::GetOpenTimeout() const {
	return 30;
}

RuleEndpoint RuleEndpoint::MakeCopy() const {
	RuleEndpoint result(*this);
	result.m_pimpl->m_uuid = Helpers::Uuid().GetAsString().c_str();
	return result;
}

//////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
#	include "Collection.cpp"
	namespace {
		//! Only for template instances.
		void MakeTemplateInstantiation() {
			MakeCollectionTemplateInstantiation<EndpointCollection>();
			MakeCollectionTemplateInstantiation<RuleEndpointCollection>();
			MakeCollectionTemplateInstantiation<RuleEndpoint::Listeners>();
		}
	}
#endif // TEMPLATES_REQUIRE_SOURCE

//////////////////////////////////////////////////////////////////////////
