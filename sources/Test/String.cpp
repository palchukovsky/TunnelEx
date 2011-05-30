/**************************************************************************
 *   Created: 2007/12/15 15:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 **************************************************************************/

#include "Prec.h"
#include "Core/String.hpp"

namespace tex = TunnelEx;
using namespace std;
using namespace boost;


namespace {

	template<class String>
	void OperatorsTestImplementation(
				const typename String::value_type* testStr1,
				const typename String::value_type* testStr2,
				const typename String::value_type* testStr3,
				const typename String::value_type* testStr4) {
		String str1(testStr1);
		EXPECT_TRUE(str1 == testStr1);
		{
			std::basic_string<String::value_type> stdStr = str1.GetCStr();
			EXPECT_TRUE(stdStr == testStr1);
		}
		str1 = testStr2;
		EXPECT_TRUE(str1 == testStr2);
		std::basic_string<String::value_type> conTestString(testStr1);
		conTestString += testStr2;
		str1 = testStr1;
		EXPECT_TRUE(str1 == testStr1);
		str1 += testStr2;
		EXPECT_TRUE(str1 == conTestString.c_str());
		EXPECT_TRUE(str1 != testStr3);
		const String str2(conTestString.c_str());
		EXPECT_TRUE(str1 == str2);
		EXPECT_TRUE(str2 != testStr4);
		String str5(testStr1);
		EXPECT_TRUE(str5 == testStr1);
		str5.Clear();
		EXPECT_TRUE(str5.IsEmpty());
		EXPECT_TRUE(str5 == String());
		String str6(testStr1);
		String str7(testStr4);
		str6.Swap(str7);
		EXPECT_TRUE(str6 == testStr4);
		EXPECT_TRUE(str7 == testStr1);
	}

}

namespace {

	TEST(Strings, Operators) {

		OperatorsTestImplementation<tex::String>(
			"Multibyte first test string",
			"Multibyte second test string",
			"Multibyte 3-rd test string",
			"Multibyte 4-th test string");
		OperatorsTestImplementation<tex::WString>(
			L"Wide first test string",
			L"Wide second test string - на этот раз на русском €зыке (й€хзщўшЎэёя€э)",
			L"Wide 3-rd test string",
			L"Wide 4-th test string - тоже на русском €зыке (й€хзщўшЎэёя€э)");
	}

	TEST(Strings, WideToMultibyte) {

		const wchar_t *const from = L"Test string - “естова€ строка - ЁёяўЎэю€щш12345 ;є%эє;.";
		const char *const to = "Test string - “естова€ строка - ЁёяўЎэю€щш12345 ;є%эє;.";
		tex::String cache;
		
		EXPECT_TRUE(tex::ConvertString(from, cache) == to);

		cache.Clear();
		EXPECT_TRUE(tex::ConvertString(tex::WString(from), cache) == to);

	}

	TEST(Strings, MultibyteToWide) {

		const char *const from = "Test string - “естова€ строка - ЁёяўЎэю€щш12345 ;є%эє;.";
		const wchar_t *const to = L"Test string - “естова€ строка - ЁёяўЎэю€щш12345 ;є%эє;.";
		tex::WString cache;
		
		EXPECT_TRUE(tex::ConvertString(from, cache) == to);

		cache.Clear();
		EXPECT_TRUE(tex::ConvertString(tex::String(from), cache) == to);

	}

	TEST(Strings, WideToUtf8) {

		const wchar_t *const from = L"Test string in UTF-8 - “естова€ строка в UTF-8.";
		tex::UString cache;
		const tex::UString::value_type to[] = {
			0x54, 0x65, 0x73, 0x74, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67,
			0x20, 0x69, 0x6e, 0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x20, 0x2d,
			0x20, 0xd0, 0xa2, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbe,
			0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x8f, 0x20, 0xd1, 0x81, 0xd1, 0x82,
			0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xba, 0xd0, 0xb0, 0x20, 0xd0, 0xb2,
			0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x2e, 0x00
		};

		EXPECT_TRUE(tex::ConvertString(from, cache) == to);

		cache.Clear();
		EXPECT_TRUE(tex::ConvertString(tex::WString(from), cache) == to);

	}

	TEST(Strings, Utf8ToWide) {
		
		const tex::UString::value_type from[] = {
			0x54, 0x65, 0x73, 0x74, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67,
			0x20, 0x69, 0x6e, 0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x20, 0x2d,
			0x20, 0xd0, 0xa2, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbe,
			0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x8f, 0x20, 0xd1, 0x81, 0xd1, 0x82,
			0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xba, 0xd0, 0xb0, 0x20, 0xd0, 0xb2,
			0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x2e, 0x00
		};
		tex::WString cache;
		const wchar_t *const to = L"Test string in UTF-8 - “естова€ строка в UTF-8.";

		EXPECT_TRUE(tex::ConvertString(from, cache) == to);

		cache.Clear();
		EXPECT_TRUE(tex::ConvertString(tex::UString(from), cache) == to);
	
	}

	TEST(Strings, Utf8ToMultibyte) {
		
		const char *const to = "Test string in UTF-8 - “естова€ строка в UTF-8.";
		const tex::UString::value_type from[] = {
			0x54, 0x65, 0x73, 0x74, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67,
			0x20, 0x69, 0x6e, 0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x20, 0x2d,
			0x20, 0xd0, 0xa2, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbe,
			0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x8f, 0x20, 0xd1, 0x81, 0xd1, 0x82,
			0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xba, 0xd0, 0xb0, 0x20, 0xd0, 0xb2,
			0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x2e, 0x00
		};
		tex::String cache;

		EXPECT_TRUE(tex::ConvertString(from, cache) == to);
		EXPECT_TRUE(tex::ConvertString(tex::UString(from), cache) == to);

	}

	TEST(Strings, MultibyteToUtf8) {

		const char *const from = "Test string in UTF-8 - “естова€ строка в UTF-8.";
		const tex::UString::value_type to[] = {
			0x54, 0x65, 0x73, 0x74, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67,
			0x20, 0x69, 0x6e, 0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x20, 0x2d,
			0x20, 0xd0, 0xa2, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbe,
			0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x8f, 0x20, 0xd1, 0x81, 0xd1, 0x82,
			0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xba, 0xd0, 0xb0, 0x20, 0xd0, 0xb2,
			0x20, 0x55, 0x54, 0x46, 0x2d, 0x38, 0x2e, 0x00
		};
		tex::UString cache;

		EXPECT_TRUE(tex::ConvertString(from, cache) == to);

		cache.Clear();
		EXPECT_TRUE(tex::ConvertString(tex::String(from), cache) == to);
	
	}

}
