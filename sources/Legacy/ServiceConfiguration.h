/**************************************************************************
 *   Created: 2008/06/13 1:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: ServiceConfiguration.h 991 2010-09-08 01:41:14Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__ServiceConfiguration_h__0806130124
#define INCLUDED_FILE__TUNNELEX__ServiceConfiguration_h__0806130124

class ServiceConfiguration;

//! Migrates service configuration.
boost::shared_ptr<ServiceConfiguration> MigrateCurrentServiceConfiguration(void);


#endif // INCLUDED_FILE__TUNNELEX__ServiceConfiguration_h__0806130124
