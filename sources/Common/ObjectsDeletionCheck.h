/**************************************************************************
 *   Created: 2008/12/11 10:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__ObjectsDeletionCheck_hpp__0812111058
#define INCLUDED_FILE__ObjectsDeletionCheck_hpp__0812111058

#ifdef DEV_VER
#	define TUNNELEX_OBJECTS_DELETION_CHECK
#endif

#if defined(TUNNELEX_OBJECTS_DELETION_CHECK)

#	include "CompileWarningsBoost.h"
#		include <boost/detail/interlocked.hpp>
#		include <boost/noncopyable.hpp>
#		include <boost/assert.hpp>
#	include "CompileWarningsBoost.h"

	namespace TunnelEx { namespace Helpers {
	
		class ObjectsDeletionChecker : private boost::noncopyable {
		public:
			explicit ObjectsDeletionChecker(
						const volatile long &instancesNumber,
						const char *const method,
						const char *const file,
						unsigned long line)
					: m_instancesNumber(instancesNumber),
					m_method(method),
					m_file(file),
					m_line(line) {
				assert(m_instancesNumber == 0);
			}
			~ObjectsDeletionChecker() {
				assert(m_instancesNumber == 0);
			}
		public:
			long GetInstancesNumber() const {
				return m_instancesNumber;
			}
		private:
			const volatile long &m_instancesNumber;
			const char *const m_method;
			const char *const m_file;
			const unsigned long m_line;
		};
	
	} }

#	define TUNNELEX_OBJECTS_DELETION_CHECK_DECLARATION(name) static volatile long name
#	define TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION(className, name) volatile long className::name = 0
#	define TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION_TEMPLATE(className, name, templateClass1) \
		template<class templateClass1> \
		volatile long className<templateClass1>::name = 0

#	define TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(name) assert(BOOST_INTERLOCKED_DECREMENT(&name) >= 0)
	//!todo: move counter to static field
#	define TUNNELEX_OBJECTS_DELETION_CHECK_CTOR(name) \
		do { \
			if (name == 0) { \
				static TunnelEx::Helpers::ObjectsDeletionChecker checker(name, __FUNCTION__, __FILE__, __LINE__); \
			} \
			BOOST_INTERLOCKED_INCREMENT(&name); \
		} while (name == 0)
#	define TUNNELEX_OBJECTS_DELETION_CHECK_ZERO(condition, name) \
		do { \
			if (condition) { \
				assert(name == 0); \
			} \
		} while (name == 0)

#else

#	define TUNNELEX_OBJECTS_DELETION_CHECK_DECLARATION(name)
#	define TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION(className, name)
#	define TUNNELEX_OBJECTS_DELETION_CHECK_DEFINITION_TEMPLATE(className, name, templateClass1)
#	define TUNNELEX_OBJECTS_DELETION_CHECK_CTOR(name)
#	define TUNNELEX_OBJECTS_DELETION_CHECK_DTOR(name)
#	define TUNNELEX_OBJECTS_DELETION_CHECK_ZERO(condition, name)

#endif

#endif // INCLUDED_FILE__ObjectsDeletionCheck_hpp__0812111058
