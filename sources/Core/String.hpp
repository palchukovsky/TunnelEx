/**************************************************************************
 *   Created: 2007/11/11 18:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__String_h__0711111848
#define INCLUDED_FILE__TUNNELEX__String_h__0711111848

#include "StringFwd.hpp"
#include "Api.h"

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	//! Template for std::string wrappers.
	template<class Element>
	class TUNNELEX_CORE_API BasicString {

	public:

		typedef unsigned int SizeType;
		typedef SizeType size_type;
		typedef typename Element ValueType;
		typedef ValueType value_type;
		typedef BasicString<typename Element> MyType;

		friend TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
				const ::TunnelEx::WString &,
				::TunnelEx::UString &);
		friend TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
				const wchar_t *,
				::TunnelEx::UString &);

		friend TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
				const ::TunnelEx::WString &,
				::TunnelEx::String &);
		friend TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
				const wchar_t *,
				::TunnelEx::String &);

		friend TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
				const ::TunnelEx::UString &,
				::TunnelEx::WString &);
		friend TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
				const ::TunnelEx::Utf8Char *,
				::TunnelEx::WString &);

		friend TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
				const ::TunnelEx::UString &,
				::TunnelEx::String &);
		friend TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
				const ::TunnelEx::Utf8Char *,
				::TunnelEx::String &);

		friend TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
				const ::TunnelEx::String &,
				::TunnelEx::WString &);
		friend TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
				const char *,
				::TunnelEx::WString &);
		
		friend TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
				const ::TunnelEx::String &,
				::TunnelEx::UString &);
		friend TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
				const char *,
				::TunnelEx::UString &);

	public:
		
		BasicString();
		BasicString(const ValueType *);
		BasicString(const MyType &);
		~BasicString() throw();
		
		MyType & operator =(const MyType &);
		MyType & operator =(const ValueType *);
		MyType & operator =(ValueType);
		MyType & operator +=(const MyType &);
		MyType & operator +=(const ValueType *);
		MyType & operator +=(ValueType);

		const ValueType & operator[](SizeType) const;
		ValueType & operator[](SizeType);

		bool operator ==(const MyType &) const;
		bool operator !=(const MyType &) const;
		bool operator <(const MyType &) const;
		bool operator >(const MyType &) const;
		bool operator <=(const MyType &) const;
		bool operator >=(const MyType &) const;

		void Assign(const MyType &);

		//! Returns substring.
		/** @param	destination		the destination object for substring;
		  * @param	offset			substaing start in parent string;
		  * @param	count			substring end in parent string (zero - to parent end);
		  */
		void SubStr(
					MyType &destination,
					SizeType offset,
					SizeType count = 0)
				const;

		MyType SubStr(SizeType offset, SizeType count = 0) const;

		size_t GetHash() const;

	public:
		
		const ValueType * GetCStr() const;
		SizeType GetLength() const;
		bool IsEmpty() const;
		void Clear();

		SizeType Find(ValueType) const;
		SizeType Find(const ValueType *) const;
		SizeType Find(const MyType &) const;

		void EncodeUrl();
		MyType EncodeUrlClone() const;

		void Swap(MyType &) throw();

	public:

		static SizeType GetNPos();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
			const ::TunnelEx::WString &source,
			::TunnelEx::UString &destination);
	TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
			const ::TunnelEx::WString::ValueType *source,
			::TunnelEx::UString &destination);

	TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
			const ::TunnelEx::WString &source,
			::TunnelEx::String &destination);
	TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
			const ::TunnelEx::WString::ValueType *source,
			::TunnelEx::String &destination);

	TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
			const ::TunnelEx::UString &source,
			::TunnelEx::WString &destination);
	TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
			const ::TunnelEx::UString::ValueType *source,
			::TunnelEx::WString &destination);

	TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
			const ::TunnelEx::UString &source,
			::TunnelEx::String &destination);
	TUNNELEX_CORE_API ::TunnelEx::String & ConvertString(
			const ::TunnelEx::UString::ValueType *source,
			::TunnelEx::String &destination);

	TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
			const ::TunnelEx::String &source,
			::TunnelEx::WString &destination);
	TUNNELEX_CORE_API ::TunnelEx::WString & ConvertString(
			const ::TunnelEx::String::ValueType *source,
			::TunnelEx::WString &destination);

	TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
			const ::TunnelEx::String &source,
			::TunnelEx::UString &destination);
	TUNNELEX_CORE_API ::TunnelEx::UString & ConvertString(
			const ::TunnelEx::String::ValueType *source,
			::TunnelEx::UString &destination);

	template<typename Destination, typename Source>
	Destination ConvertString(const Source &source) {
		typename Destination buffer;
		return ConvertString(source, buffer);
	}

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE__TUNNELEX__String_h__0711111848
