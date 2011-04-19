/**************************************************************************
 *   Created: 2009/04/12 17:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Wait_h__0904121731
#define INCLUDED_FILE__TUNNELEX__Wait_h__0904121731

#define TEST_WAIT_DEFAULT_SLEEP 50
#define TEST_WAIT_DEFAULT_COUNT 500

//////////////////////////////////////////////////////////////////////////

#define TEST_WAIT_AND_CHECK_EX(expr, count, sleep) \
	for (unsigned int i = 0; i < count && !(expr);  ++i) { \
		Sleep(sleep); \
	} \
	BOOST_CHECK(expr)

#define TEST_WAIT_AND_CHECK(expr) \
	TEST_WAIT_AND_CHECK_EX(expr, TEST_WAIT_DEFAULT_SLEEP, TEST_WAIT_DEFAULT_COUNT);

//////////////////////////////////////////////////////////////////////////

#define TEST_WAIT_AND_CHECK_MESSAGE_EX(expr, count, sleep, message) \
	for (unsigned int i = 0; i < count && !(expr);  ++i) { \
		Sleep(sleep); \
	} \
	BOOST_CHECK_MESSAGE(expr, message)

#define TEST_WAIT_AND_CHECK_MESSAGE(expr, message) \
	TEST_WAIT_AND_CHECK_MESSAGE_EX(expr, TEST_WAIT_DEFAULT_COUNT, TEST_WAIT_DEFAULT_SLEEP, message)

//////////////////////////////////////////////////////////////////////////

#define TEST_WAIT_AND_REQUIRE_EX(expr, count, sleep) \
	for (unsigned int i = 0; i < count && !(expr);  ++i) { \
		Sleep(sleep); \
	} \
	BOOST_REQUIRE(expr)

#define TEST_WAIT_AND_REQUIRE(expr) \
	TEST_WAIT_AND_REQUIRE_EX(expr, TEST_WAIT_DEFAULT_COUNT, TEST_WAIT_DEFAULT_SLEEP)

//////////////////////////////////////////////////////////////////////////

#define TEST_WAIT_AND_REQUIRE_MESSAGE_EX(expr, count, sleep, message) \
	for (unsigned int i = 0; i < count && !(expr);  ++i) { \
		Sleep(sleep); \
	} \
	BOOST_REQUIRE_MESSAGE(expr, message)

#define TEST_WAIT_AND_REQUIRE_MESSAGE(expr, message) \
	TEST_WAIT_AND_REQUIRE_MESSAGE_EX(expr, TEST_WAIT_DEFAULT_COUNT, TEST_WAIT_DEFAULT_SLEEP, message)

//////////////////////////////////////////////////////////////////////////

#endif // #ifndef INCLUDED_FILE__TUNNELEX__Wait_h__0904121731