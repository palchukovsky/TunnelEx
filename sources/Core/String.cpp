/**************************************************************************
 *   Created: 2007/11/11 19:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#include "Prec.h"
#include "String.hpp"
#include "Error.hpp"
#include "Exceptions.hpp"

using namespace TunnelEx;
using namespace TunnelEx::Helpers;
namespace tex = TunnelEx;

//////////////////////////////////////////////////////////////////////////

namespace {

	template<typename Destination>
	void WideCharToMultiByte(
				const unsigned int codePage,
				const wchar_t *const source,
				const size_t sourceLen,
				Destination &destination) {
		if (sourceLen == 0) {
			destination.clear();
			return;
		}
		const int destintationLen = ::WideCharToMultiByte(
			codePage,
			0,
			source,
			sourceLen,
			NULL,
			0,
			0,
			0);
		if (destintationLen == 0) {
			const Error error(GetLastError());
			throw SystemException(error.GetString().GetCStr());
		}
		std::vector<typename Destination::value_type> destinationBuf(destintationLen, 0);
		::WideCharToMultiByte(
			codePage,
			0,
			source,
			sourceLen,
			reinterpret_cast<char *>(&destinationBuf[0]),
			destintationLen,
			0,
			0);
		typename Destination(destinationBuf.begin(), destinationBuf.end()).swap(destination);
	}

	template<class Source, class Destination>
	void MultiByteToWideChar(
				const unsigned int codePage,
				const Source *const source,
				const unsigned int sourceLen,
				Destination &destination) {
		std::vector<wchar_t> buffer;
		MultiByteToWideCharBuffer(codePage, source, sourceLen, buffer);
		Destination(buffer.begin(), buffer.end()).swap(destination);
	}

	template<class Source>
	void MultiByteToWideCharBuffer(
				const unsigned int codePage,
				const Source *const source,
				const unsigned int sourceLen,
				std::vector<wchar_t> &destination) {
		if (!sourceLen) {
			destination.clear();
			return;
		}
		const int destinationLen = ::MultiByteToWideChar(
			codePage,
			0,
			reinterpret_cast<const char *>(source),
			sourceLen,
			NULL,
			0);
		if (destinationLen == 0) {
			const Error error(GetLastError());
			throw SystemException(error.GetString().GetCStr());
		}
		std::vector<wchar_t> destinationTmp(destinationLen, 0);
		::MultiByteToWideChar(
			codePage,
			0,
			reinterpret_cast<const char *>(source),
			sourceLen,
			&destinationTmp[0],
			destinationLen);
		destinationTmp.swap(destination);
	}

}

//////////////////////////////////////////////////////////////////////////

template<class Element>
class TunnelEx::BasicString<Element>::Implementation : private boost::noncopyable {

public:

	typedef std::basic_string<typename Element> MyString;

	Implementation() {
		//...//
	}

	Implementation(const Element *cString)
			: m_string(cString) {
		//...//
	}

	Implementation(const MyString &rhs)
			: m_string(rhs) {
		//...//
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
	destination.m_pimpl->m_string
		= m_pimpl->m_string.substr(
			offset,
			count == 0 ? Implementation::MyString::npos : count);
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
	return boost::hash_value(m_pimpl->m_string);
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
	std::swap(m_pimpl, rhs.m_pimpl);
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
	WideCharToMultiByte(
		CP_UTF8,
		source.m_pimpl->m_string.c_str(),
		source.m_pimpl->m_string.size(),
		destination.m_pimpl->m_string);
	return destination;
}

UString & tex::ConvertString(const WString::ValueType *source, UString &destination) {
	WideCharToMultiByte(CP_UTF8, source, wcslen(source), destination.m_pimpl->m_string);
	return destination;
}

String & tex::ConvertString(const UString &source, String &destination) {
	if (source.IsEmpty()) {
		destination.Clear();
		return destination;
	}
	std::vector<wchar_t> buffer;
	MultiByteToWideCharBuffer(
		CP_UTF8,
		source.m_pimpl->m_string.c_str(),
		source.m_pimpl->m_string.size(),
		buffer);
	WideCharToMultiByte(CP_ACP, &buffer[0], buffer.size(), destination.m_pimpl->m_string);
	return destination;
}

String & tex::ConvertString(const UString::ValueType *source, String &destination) {
	const auto sourceLen = _mbslen(source);
	if (sourceLen == 0) {
		destination.Clear();
		return destination;
	}
	std::vector<wchar_t> buffer;
	MultiByteToWideCharBuffer(CP_UTF8, source, sourceLen, buffer);
	WideCharToMultiByte(CP_ACP, &buffer[0], buffer.size(), destination.m_pimpl->m_string);
	return destination;
}

WString & tex::ConvertString(const String &source, WString &destination) {
	MultiByteToWideChar(
		CP_ACP,
		source.m_pimpl->m_string.c_str(),
		source.m_pimpl->m_string.size(),
		destination.m_pimpl->m_string);
	return destination;
}

WString & tex::ConvertString(const String::ValueType *source, WString &destination) {
	MultiByteToWideChar(CP_ACP, source, strlen(source), destination.m_pimpl->m_string);
	return destination;
}

WString & tex::ConvertString(const UString &source, WString &destination) {
	MultiByteToWideChar(
		CP_UTF8,
		source.m_pimpl->m_string.c_str(),
		source.m_pimpl->m_string.size(),
		destination.m_pimpl->m_string);
	return destination;
}

WString & tex::ConvertString(const UString::ValueType *source, WString &destination) {
	MultiByteToWideChar(CP_UTF8, source, _mbslen(source), destination.m_pimpl->m_string);
	return destination;
}

String & tex::ConvertString(const WString &source, String &destination) {
	WideCharToMultiByte(
		CP_ACP,
		source.m_pimpl->m_string.c_str(),
		source.m_pimpl->m_string.size(),
		destination.m_pimpl->m_string);
	return destination;
}

String & tex::ConvertString(const WString::ValueType *source, String &destination) {
	WideCharToMultiByte(CP_ACP, source, wcslen(source), destination.m_pimpl->m_string);
	return destination;
}

UString & tex::ConvertString(const String &source, UString &destination) {
	if (source.IsEmpty()) {
		destination.Clear();
		return destination;
	}
	std::vector<wchar_t> buffer;
	MultiByteToWideCharBuffer(
 		CP_ACP,
 		source.m_pimpl->m_string.c_str(),
 		source.m_pimpl->m_string.size(),
 		buffer);
	WideCharToMultiByte(CP_UTF8, &buffer[0], buffer.size(), destination.m_pimpl->m_string);
	return destination;
}

UString & tex::ConvertString(const String::ValueType *source, UString &destination) {
	const auto sourceLen = strlen(source);
	if (sourceLen == 0) {
		destination.Clear();
		return destination;
	}
	std::vector<wchar_t> buffer;
	MultiByteToWideChar(CP_ACP, source, sourceLen, buffer);
	WideCharToMultiByte(CP_UTF8, &buffer[0], buffer.size(), destination.m_pimpl->m_string);
	return destination;
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
