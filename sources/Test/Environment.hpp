/**************************************************************************
 *   Created: 2008/10/21 14:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Fixtures_hpp__0810211416
#define INCLUDED_FILE__TUNNELEX__Fixtures_hpp__0810211416

////////////////////////////////////////////////////////////////////////////////

class Environment : public testing::Environment {

public:
		
	virtual ~Environment();

public:

	virtual void SetUp();
	virtual void TearDown();

private:

	void SetLogParams();

private:

	static bool m_isGlobalInited;

};

////////////////////////////////////////////////////////////////////////////////

class LocalEnvironment : public testing::Environment {

public:
		
	virtual ~LocalEnvironment();

public:

	virtual void SetUp();
	virtual void TearDown();

};
	
////////////////////////////////////////////////////////////////////////////////

class ServerEnvironment : public testing::Environment {

public:
		
	virtual ~ServerEnvironment();

public:

	virtual void SetUp();
	virtual void TearDown();

};

////////////////////////////////////////////////////////////////////////////////

class ClientEnvironment : public testing::Environment {

public:
		
	virtual ~ClientEnvironment();

public:

	virtual void SetUp();
	virtual void TearDown();

};

////////////////////////////////////////////////////////////////////////////////

#endif // INCLUDED_FILE__TUNNELEX__Fixtures_hpp__0810211416
