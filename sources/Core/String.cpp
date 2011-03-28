/**************************************************************************
 *   Created: 2007/11/11 19:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * -------------------------------------------------------------------
 *       $Id: String.cpp 1129 2011-02-22 17:28:50Z palchukovsky $
 **************************************************************************/

#include "Prec.h"
#include "String.hpp"

using namespace std;
using namespace boost;
using namespace TunnelEx;
using namespace TunnelEx::Helpers;
namespace tex = TunnelEx;

//////////////////////////////////////////////////////////////////////////

template<class Element>
class TunnelEx::BasicString<Element>::Implementation : private noncopyable {

public:

	typedef basic_string<typename Element> MyString;

	Implementation()
		: noncopyable()
		, m_string() {
		/*...*/
	}

	Implementation(const Element *cString)
		: noncopyable()
		, m_string(cString) {
		/*...*/
	}

	Implementation(const MyString &rhs)
		: noncopyable()
		, m_string(rhs) {
		/*...*/
	}

	template<class Destination>
	void WideCharToMultiByte(const unsigned int codePage, Destination &destination) {
		if (!m_string.size()) {
			destination.clear();
			return;
		}
		const int sourceLen = int(m_string.size());
		const int destintationLen = ::WideCharToMultiByte(
			codePage, 0, m_string.c_str(), sourceLen, NULL, 0, 0, 0);
		vector<typename Destination::value_type> destinationBuf(destintationLen, 0);
		::WideCharToMultiByte(
			codePage, 0, m_string.c_str(), sourceLen,
			reinterpret_cast<char *>(&destinationBuf[0]), destintationLen, 0, 0);
		typename Destination(destinationBuf.begin(), destinationBuf.end()).swap(destination);
	}

	template<class Destination>
	void MultiByteToWideChar(const unsigned int codePage, Destination &destination) {
		if (!m_string.size()) {
			destination.clear();
			return;
		}
		const int sourceLen = int(m_string.size());
		const int destinationLen = ::MultiByteToWideChar(
			codePage, 0, reinterpret_cast<const char *>(m_string.c_str()), sourceLen, NULL, 0);
		vector<wchar_t> destinationBuf(destinationLen, 0);
		::MultiByteToWideChar(
			codePage, 0, reinterpret_cast<const char *>(m_string.c_str()), sourceLen,
			&destinationBuf[0], destinationLen);
		Destination(destinationBuf.begin(), destinationBuf.end()).swap(destination);
	}

public:

	MyString m_string;

};

//////////////////////////////////////////////////////////////////////////

template<class Element>
BasicString<Element>::BasicString()
		: m_pimpl(new Implementation) {
	//...//
}

template<class Element>
BasicString<Element>::~BasicString() {
	delete m_pimpl;
}

template<class Element>
const typename BasicString<Element>::ValueType * BasicString<Element>::GetCStr() const {
	return m_pimpl->m_string.c_str();
}

template<class Element>
typename BasicString<Element>::SizeType BasicString<Element>::GetLength() const {
	return BasicString<Element>::SizeType(m_pimpl->m_string.size());
}

template<class Element>
bool BasicString<Element>::IsEmpty() const {
	return m_pimpl->m_string.empty();
}

template<class Element>
void BasicString<Element>::Clear() {
	m_pimpl->m_string.clear();
}

template<class Element>
BasicString<Element>::BasicString(const ValueType *ch)
		: m_pimpl(new Implementation(ch)) {
	//...//
}

template<class Element>
BasicString<Element>::BasicString(const MyType &rhs)
		: m_pimpl(new Implementation(rhs.m_pimpl->m_string)) {
	//...//
}

template<class Element>
typename BasicString<Element>::MyType & BasicString<Element>::operator =(const MyType &rhs) {
	m_pimpl->m_string = rhs.m_pimpl->m_string;
	return *this;
}

template<class Element>
typename BasicString<Element>::MyType & BasicString<Element>::operator =(const ValueType *pch) {
	m_pimpl->m_string = pch;
	return *this;
}

template<class Element>
typename BasicString<Element>::MyType & BasicString<Element>::operator =(ValueType ch) {
	m_pimpl->m_string = ch;
	return *this;
}

template<class Element>
typename BasicString<Element>::MyType & BasicString<Element>::operator +=(const MyType &rhs) {
	m_pimpl->m_string += rhs.m_pimpl->m_string;
	return *this;
}

template<class Element>
typename BasicString<Element>::MyType & BasicString<Element>::operator +=(const ValueType *pch) {
	m_pimpl->m_string += pch;
	return *this;
}

template<class Element>
typename BasicString<Element>::MyType & BasicString<Element>::operator +=(ValueType ch) {
	m_pimpl->m_string += ch;
	return *this;
}

template<class Element>
const typename BasicString<Element>::ValueType & BasicString<Element>::operator [](SizeType offset) const {
	return m_pimpl->m_string[offset];
}

template<class Element>
typename BasicString<Element>::ValueType & BasicString<Element>::operator [](SizeType offset) {
	return m_pimpl->m_string[offset];
}

template<class Element>
bool BasicString<Element>::operator ==(const MyType &rhs) const  {
	return m_pimpl->m_string == rhs.m_pimpl->m_string;
}

template<class Element>
bool BasicString<Element>::operator !=(const MyType &rhs) const  {
	return m_pimpl->m_string != rhs.m_pimpl->m_string;
}

template<class Element>
bool BasicString<Element>::operator <(const MyType &rhs) const {
	return m_pimpl->m_string < rhs.m_pimpl->m_string;
}

template<class Element>
bool BasicString<Element>::operator >(const MyType &rhs) const {
	return m_pimpl->m_string > rhs.m_pimpl->m_string;
}

template<class Element>
bool BasicString<Element>::operator <=(const MyType &rhs) const {
	return m_pimpl->m_string <= rhs.m_pimpl->m_string;
}

template<class Element>
bool BasicString<Element>::operator >=(const MyType &rhs) const {
	return m_pimpl->m_string >= rhs.m_pimpl->m_string;
}

template<class Element>
void BasicString<Element>::Assign(const MyType &rhs) {
	m_pimpl->m_string.assign(rhs.m_pimpl->m_string);
}

template<class Element>
void BasicString<Element>::SubStr(
			MyType &destination,
			SizeType offset,
			SizeType count)
		const {
	destination.m_pimpl->m_string = m_pimpl->m_string.substr(offset, count == 0 ? Implementation::MyString::npos : count);
}

template<class Element>
typename BasicString<Element>::MyType BasicString<Element>::SubStr(
			SizeType offset,
			SizeType count)
		const {
	MyType result;
	SubStr(result, offset, count);
	return result;
}

template<class Element>
size_t BasicString<Element>::GetHash() const {
	return hash_value(m_pimpl->m_string);
}

template<class Element>
typename BasicString<Element>::SizeType BasicString<Element>::Find(
			ValueType ch)
		const {
	return m_pimpl->m_string.find(ch);
}

template<class Element>
typename BasicString<Element>::SizeType BasicString<Element>::Find(
			const ValueType *str)
		const {
	return m_pimpl->m_string.find(str);
}

template<class Element>
typename BasicString<Element>::SizeType BasicString<Element>::Find(
			const MyType &str)
		const {
	return m_pimpl->m_string.find(str.m_pimpl->m_string);
}


template<class Element>
void BasicString<Element>::Swap(MyType &rhs) throw() {
	m_pimpl->m_string.swap(rhs.m_pimpl->m_string);
}

template<class Element>
typename BasicString<Element>::SizeType BasicString<Element>::GetNPos() {
	return Implementation::MyString::npos;
}

template<class Element>
void BasicString<Element>::EncodeUrl() {
	StringUtil::EncodeUrl(m_pimpl->m_string).swap(m_pimpl->m_string);
}

template<class Element>
typename BasicString<Element>::MyType BasicString<Element>::EncodeUrlClone() const {
	MyType result;
	StringUtil::EncodeUrl(m_pimpl->m_string, result.m_pimpl->m_string);
	return result;
}

//////////////////////////////////////////////////////////////////////////

UString & tex::ConvertString(const WString &source, UString &destination) {
	source.m_pimpl->WideCharToMultiByte(CP_UTF8, destination.m_pimpl->m_string);
	return destination;
}

String & tex::ConvertString(const UString &source, String &destination) {
	//! \todo: reimplement:
	WString wideString;
	return ConvertString(ConvertString(source, wideString), destination);
}

WString & tex::ConvertString(const String &source, WString &destination) {
	source.m_pimpl->MultiByteToWideChar(CP_ACP, destination.m_pimpl->m_string);
	return destination;
}

WString & tex::ConvertString(const UString &source, WString &destination) {
	source.m_pimpl->MultiByteToWideChar(CP_UTF8, destination.m_pimpl->m_string);
	return destination;
}

String & tex::ConvertString(const WString &source, String &destination) {
	source.m_pimpl->WideCharToMultiByte(CP_ACP, destination.m_pimpl->m_string);
	return destination;
}

UString & tex::ConvertString(const String &source, UString &destination) {
	//! \todo: reimplement:
	WString wideString;
	return ConvertString(ConvertString(source, wideString), destination);
}

//////////////////////////////////////////////////////////////////////////

#if TEMPLATES_REQUIRE_SOURCE != 0
	namespace TunnelEx {
		//! Only for template instantiation.
		template<class T>
		struct StringsTemplateInstantiationMaker {
			StringsTemplateInstantiationMaker() {
				T x;
				T::ValueType element = x.GetCStr()[0];
				const T::ValueType *elementPtr = x.GetCStr();
				const T y(elementPtr);
				T z(y);
				z = y;
				z == y;
				z != y;
				z < y;
				z <= y;
				z > y;
				z >= y;
				z = elementPtr;
				z = element;
				z += y;
				z += elementPtr;
				z += element;
				const T::ValueType &elementConstRef = y[0]; 
				elementConstRef;
				z[0] = element;
				z.Assign(x);
				z.GetLength();
				z.IsEmpty();
				z.SubStr(x, 0);
				z.SubStr(0);
				z.Clear();
				z.GetHash();
				z.Swap(x);
				z.Find(element);
				z.Find(&element);
				z.Find(z);
				T::GetNPos();
				z.EncodeUrl();
				z.EncodeUrlClone();
			}
		};
		//! Only for template instantiation.
		void MakeStringsTemplateInstantiation() {
			StringsTemplateInstantiationMaker<String>();
			StringsTemplateInstantiationMaker<UString>();
			StringsTemplateInstantiationMaker<WString>();
		}
	}
#endif // TEMPLATES_REQUIRE_SOURCE


//////////////////////////////////////////////////////////////////////////
