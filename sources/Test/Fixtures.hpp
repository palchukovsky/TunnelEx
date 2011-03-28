/**************************************************************************
 *   Created: 2008/10/21 14:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Fixtures.hpp 959 2010-06-22 11:36:04Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Fixtures_hpp__0810211416
#define INCLUDED_FILE__TUNNELEX__Fixtures_hpp__0810211416

namespace Test {

	class GlobalFixture : private boost::noncopyable {

	public:
		
		GlobalFixture();
		~GlobalFixture();

	private:

		void StartNetworkFramework();
		void SetLogParams();

	};

}

#endif // INCLUDED_FILE__TUNNELEX__Fixtures_hpp__0810211416