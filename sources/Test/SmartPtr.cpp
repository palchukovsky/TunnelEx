/**************************************************************************
 *   Created: 2008/03/21 11:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: SmartPtr.cpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#include "Prec.h"

#include "DtorTester.hpp"

#include <TunnelEx/SharedPtr.hpp>
#include <TunnelEx/UniquePtr.hpp>

namespace ut = boost::unit_test;
namespace tex = TunnelEx;
using namespace std;
using namespace boost;

namespace Test { BOOST_AUTO_TEST_SUITE(SmartPtr)

	BOOST_AUTO_TEST_CASE(Share) {
		
		string lastDelObj;
		unsigned int delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
			foo.Reset();
			BOOST_CHECK(delObjNumb == 1);
		}
		
		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
			foo.Reset(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 1);
		}
		BOOST_CHECK(delObjNumb == 2);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
			foo1 = foo2;
			BOOST_CHECK(delObjNumb == 1);
		}
		BOOST_CHECK(delObjNumb == 2);
		
		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1;
			{
				tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj));
				BOOST_CHECK(delObjNumb == 0);
				foo1 = foo2;
				BOOST_CHECK(delObjNumb == 0);
			}
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1;
			{
				tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj));
				BOOST_CHECK(delObjNumb == 0);
				foo1 = foo2;
				BOOST_CHECK(delObjNumb == 0);
			}
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
			{
				tex::SharedPtr<DtorTester> foo2(foo1);
				BOOST_CHECK(delObjNumb == 0);
				{
					tex::SharedPtr<DtorTester> foo3(foo2);
					BOOST_CHECK(delObjNumb == 0);
					{
						tex::SharedPtr<DtorTester> foo4(foo3);
						BOOST_CHECK(delObjNumb == 0);
						{
							tex::SharedPtr<DtorTester> foo5(foo4);
							BOOST_CHECK(delObjNumb == 0);
						}
						BOOST_CHECK(delObjNumb == 0);
					}
					BOOST_CHECK(delObjNumb == 0);
				}
				BOOST_CHECK(delObjNumb == 0);
			}
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj, "obj 1"));
			BOOST_CHECK(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj, "obj 2"));
			BOOST_CHECK(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo3(new DtorTester(delObjNumb, lastDelObj, "obj 3"));
			BOOST_CHECK(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo4(new DtorTester(delObjNumb, lastDelObj, "obj 4"));
			BOOST_CHECK(delObjNumb == 0);
			foo1 = foo2;
			BOOST_CHECK(delObjNumb == 1);
			BOOST_CHECK(lastDelObj == "obj 1");
			foo1 = foo3;
			BOOST_CHECK(delObjNumb == 1);
			foo1 = foo3 = foo4;
			BOOST_CHECK(delObjNumb == 2);
			BOOST_CHECK(lastDelObj == "obj 3");
			foo2 = foo4;
			BOOST_CHECK(delObjNumb == 3);
			BOOST_CHECK(lastDelObj == "obj 2");
		}
		BOOST_CHECK(delObjNumb == 4);
		BOOST_CHECK(lastDelObj == "obj 4");

	}

	BOOST_AUTO_TEST_CASE(Auto) {

		string lastDelObj;
		unsigned int delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
			foo.Reset();
			BOOST_CHECK(delObjNumb == 1);
		}

		delObjNumb = 0;
		{
			DtorTester* tester = new DtorTester(delObjNumb, lastDelObj);
			{
				tex::UniquePtr<DtorTester> foo(tester);
				BOOST_CHECK(delObjNumb == 0);
				foo.Release();
			}
			BOOST_CHECK(delObjNumb == 0);
			delete tester;
		}

		delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj));
			BOOST_CHECK(delObjNumb == 0);
			tex::UniquePtr<DtorTester> foo2(foo1);
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj, "obj 1"));
			BOOST_CHECK(delObjNumb == 0);
			tex::UniquePtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj, "obj 2"));
			BOOST_CHECK(delObjNumb == 0);
			foo1 = foo2;
			BOOST_CHECK(delObjNumb == 1);
			BOOST_CHECK(lastDelObj == "obj 1");
		}
		BOOST_CHECK(delObjNumb == 2);
		BOOST_CHECK(lastDelObj == "obj 2");

	}

	BOOST_AUTO_TEST_CASE(SmartPtr_Cross) {

		string lastDelObj;
		unsigned int delObjNumb = 0;

		{
			tex::UniquePtr<DtorTester> autoPtr(new DtorTester(delObjNumb, lastDelObj));
			tex::SharedPtr<DtorTester> sharedPtr(autoPtr);
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> sharedPtr;
			{
				tex::UniquePtr<DtorTester> autoPtr(new DtorTester(delObjNumb, lastDelObj));
				sharedPtr = autoPtr;
			}
			BOOST_CHECK(delObjNumb == 0);
		}
		BOOST_CHECK(delObjNumb == 1);

	}

BOOST_AUTO_TEST_SUITE_END() }

