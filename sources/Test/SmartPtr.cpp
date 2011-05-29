/**************************************************************************
 *   Created: 2008/03/21 11:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#include "Prec.h"

#include "TestUtils/DtorTester.hpp"

#include "Core/SharedPtr.hpp"
#include "Core/UniquePtr.hpp"

namespace tex = TunnelEx;

namespace Test {

	TEST(SmartPtr, Share) {
		
		std::string lastDelObj;
		unsigned int delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
			foo.Reset();
			EXPECT_TRUE(delObjNumb == 1);
		}
		
		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
			foo.Reset(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 1);
		}
		EXPECT_TRUE(delObjNumb == 2);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
			foo1 = foo2;
			EXPECT_TRUE(delObjNumb == 1);
		}
		EXPECT_TRUE(delObjNumb == 2);
		
		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1;
			{
				tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj));
				EXPECT_TRUE(delObjNumb == 0);
				foo1 = foo2;
				EXPECT_TRUE(delObjNumb == 0);
			}
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1;
			{
				tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj));
				EXPECT_TRUE(delObjNumb == 0);
				foo1 = foo2;
				EXPECT_TRUE(delObjNumb == 0);
			}
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
			{
				tex::SharedPtr<DtorTester> foo2(foo1);
				EXPECT_TRUE(delObjNumb == 0);
				{
					tex::SharedPtr<DtorTester> foo3(foo2);
					EXPECT_TRUE(delObjNumb == 0);
					{
						tex::SharedPtr<DtorTester> foo4(foo3);
						EXPECT_TRUE(delObjNumb == 0);
						{
							tex::SharedPtr<DtorTester> foo5(foo4);
							EXPECT_TRUE(delObjNumb == 0);
						}
						EXPECT_TRUE(delObjNumb == 0);
					}
					EXPECT_TRUE(delObjNumb == 0);
				}
				EXPECT_TRUE(delObjNumb == 0);
			}
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj, "obj 1"));
			EXPECT_TRUE(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj, "obj 2"));
			EXPECT_TRUE(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo3(new DtorTester(delObjNumb, lastDelObj, "obj 3"));
			EXPECT_TRUE(delObjNumb == 0);
			tex::SharedPtr<DtorTester> foo4(new DtorTester(delObjNumb, lastDelObj, "obj 4"));
			EXPECT_TRUE(delObjNumb == 0);
			foo1 = foo2;
			EXPECT_TRUE(delObjNumb == 1);
			EXPECT_TRUE(lastDelObj == "obj 1");
			foo1 = foo3;
			EXPECT_TRUE(delObjNumb == 1);
			foo1 = foo3 = foo4;
			EXPECT_TRUE(delObjNumb == 2);
			EXPECT_TRUE(lastDelObj == "obj 3");
			foo2 = foo4;
			EXPECT_TRUE(delObjNumb == 3);
			EXPECT_TRUE(lastDelObj == "obj 2");
		}
		EXPECT_TRUE(delObjNumb == 4);
		EXPECT_TRUE(lastDelObj == "obj 4");

	}

	TEST(SmartPtr, Auto) {

		std::string lastDelObj;
		unsigned int delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
			foo.Reset();
			EXPECT_TRUE(delObjNumb == 1);
		}

		delObjNumb = 0;
		{
			DtorTester* tester = new DtorTester(delObjNumb, lastDelObj);
			{
				tex::UniquePtr<DtorTester> foo(tester);
				EXPECT_TRUE(delObjNumb == 0);
				foo.Release();
			}
			EXPECT_TRUE(delObjNumb == 0);
			delete tester;
		}

		delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj));
			EXPECT_TRUE(delObjNumb == 0);
			tex::UniquePtr<DtorTester> foo2(foo1);
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::UniquePtr<DtorTester> foo1(new DtorTester(delObjNumb, lastDelObj, "obj 1"));
			EXPECT_TRUE(delObjNumb == 0);
			tex::UniquePtr<DtorTester> foo2(new DtorTester(delObjNumb, lastDelObj, "obj 2"));
			EXPECT_TRUE(delObjNumb == 0);
			foo1 = foo2;
			EXPECT_TRUE(delObjNumb == 1);
			EXPECT_TRUE(lastDelObj == "obj 1");
		}
		EXPECT_TRUE(delObjNumb == 2);
		EXPECT_TRUE(lastDelObj == "obj 2");

	}

	TEST(SmartPtr, Cross) {

		std::string lastDelObj;
		unsigned int delObjNumb = 0;

		{
			tex::UniquePtr<DtorTester> autoPtr(new DtorTester(delObjNumb, lastDelObj));
			tex::SharedPtr<DtorTester> sharedPtr(autoPtr);
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

		delObjNumb = 0;
		{
			tex::SharedPtr<DtorTester> sharedPtr;
			{
				tex::UniquePtr<DtorTester> autoPtr(new DtorTester(delObjNumb, lastDelObj));
				sharedPtr = autoPtr;
			}
			EXPECT_TRUE(delObjNumb == 0);
		}
		EXPECT_TRUE(delObjNumb == 1);

	}

}

