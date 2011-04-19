/**************************************************************************
 *   Created: 2007/02/11 5:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
  **************************************************************************/

#ifndef INCLUDED_FILE_Log__07_02110515
#define INCLUDED_FILE_Log__07_02110515

#include "Singleton.hpp"
#include "Api.h"
#include "String.hpp"
#include "SharedPtr.hpp"
#include "Time.h"

//! \todo: remove this headers before API will be released.
#include "Format.hpp"
#include <string>

#ifdef _DEBUG
#	define LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE
#endif // #ifdef _DEBUG

namespace TunnelEx {

	//////////////////////////////////////////////////////////////////////////

	enum LogLevel {
		LOG_LEVEL_UNKNOWN = 0,
		LOG_LEVEL_TRACK,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_INFO,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_SYSTEM_ERROR,
		LOG_LEVEL_FATAL_ERROR,
		LOG_LEVEL_LEVELS_COUNT
	};

	//////////////////////////////////////////////////////////////////////////

	namespace Singletons {

		//! A log real class.
		/* To get instance please use the Log-singleton. */
		class TUNNELEX_CORE_API LogPolicy {

			template<typename T, template<class> class L, template<class> class Th>
			friend class Holder;

		private:

			LogPolicy();
			LogPolicy(const LogPolicy &);
			~LogPolicy();
			const LogPolicy & operator =(const LogPolicy &);
			LogPolicy * operator *();

		public:

#			ifdef LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE
				//! Debug tracking, available only in a debug-build.
				void AppendTracking(
							const char *className,
							const char *methodName,
							const char *file,
							const unsigned int line)
						throw();
#			endif // LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE

			//! Debug information level, for developers version.
			void AppendDebug(const std::string &) throw();
			void AppendDebug(const wchar_t *const message) throw() {
				if (!IsDebugRegistrationOn()) {
					return;
				}
				try {
					AppendDebug(ConvertString<String>(message).GetCStr());
				} catch (...) {
					BOOST_ASSERT(false);
				}
			}
			//! Format and add debug message.
			template<typename T1>
			void AppendDebug(const char *str, const T1 &insert1) throw() {
				if (IsDebugRegistrationOn()) {
					try {
						AppendDebug((TunnelEx::Format(str) % insert1).str());
					} catch (...) {
						AppendWarn((TunnelEx::Format("Format-error for the string \"%1%\".") % str).str());
					}
				}
			}
			//! Format and add debug message.
			template<typename T1, typename T2>
			void AppendDebug(const char *str, const T1 &insert1, const T2 &insert2) throw() {
				if (IsDebugRegistrationOn()) {
					try {
						AppendDebug((TunnelEx::Format(str) % insert1 % insert2).str());
					} catch (...) {
						AppendWarn((TunnelEx::Format("Format-error for the string \"%1%\".") % str).str());
					}
				}
			}
			//! Format and add debug message.
			template<typename T1, typename T2, typename T3>
			void AppendDebug(
						const char *str,
						const T1 &insert1,
						const T2 &insert2,
						const T3 &insert3)
					throw() {
				if (IsDebugRegistrationOn()) {
					try {
						AppendDebug((TunnelEx::Format(str) % insert1 % insert2 % insert3).str());
					} catch (...) {
						AppendWarn((TunnelEx::Format("Format-error for the string \"%1%\".") % str).str());
					}
				}
			}
			//! Format and add debug message.
			template<typename T1, typename T2, typename T3, typename T4>
			void AppendDebug(
						const char *str,
						const T1 &insert1,
						const T2 &insert2,
						const T3 &insert3,
						const T4 &insert4)
					throw() {
				if (IsDebugRegistrationOn()) {
					try {
						AppendDebug((TunnelEx::Format(str) % insert1 % insert2 % insert3 % insert4).str());
					} catch (...) {
						AppendWarn((TunnelEx::Format("Format-error for the string \"%1%\".") % str).str());
					}
				}
			}
			//! Format and add debug message.
			template<typename T1, typename T2, typename T3, typename T4, typename T5>
			void AppendDebug(
						const char *str,
						const T1 &insert1,
						const T2 &insert2,
						const T3 &insert3,
						const T4 &insert4,
						const T5 &insert5)
					throw() {
				if (IsDebugRegistrationOn()) {
					try {
						AppendDebug((TunnelEx::Format(str) % insert1 % insert2 % insert3 % insert4 % insert5).str());
					} catch (...) {
						AppendWarn((TunnelEx::Format("Format-error for the string \"%1%\".") % str).str());
					}
				}
			}
			//! Information level, says user about completed actions and so on.
			void AppendInfo(const std::string &) throw();
			//! Waring information level (debug version). 
			/** In that level we say that some action can't be completed
				correctly or notice some users wrong actions. */
			void AppendWarn(const std::string &) throw();
			//! Error information level - one or more important (or complex)
			//! actions failed or some data lost (debug version).
			void AppendError(const std::string &) throw();
			//! System error (debug version).
			/** We will catch such errors, but we don't know what to do
				with it. It can be the same as LEVEL_ERROR, but
				point to system error.*/
			void AppendSystemError(const std::string &) throw();
			//! A fatal-error information level (debug version).
			/** After such error program can't work and will be terminated. */
			void AppendFatalError(const std::string &) throw();
			
			void SetLevelRegistrationState(LogLevel, bool isRegistrationOn) throw();
			void SetMinimumRegistrationLevel(LogLevel) throw();

			bool IsDebugRegistrationOn() const throw();
			bool IsInfoRegistrationOn() const throw();
			bool IsWarnsRegistrationOn() const throw();
			bool IsCommonErrorsRegistrationOn() const throw();
			bool IsSystemErrorsRegistrationOn() const throw();
			bool IsFatalErrorsRegistrationOn() const throw();

			bool AttachFile(const std::wstring &filePath) throw();
			bool DetachFile(const std::wstring &filePath) throw();

			bool AttachStdoutStream() throw();
			bool DetachStdoutStream() throw();

			bool AttachStderrStream() throw();
			bool DetachStderrStream() throw();

			LogLevel ResolveLevel(const char *levelName) const throw();

			unsigned long long GetSize() const;

			::TunnelEx::TimeT GetLastWarnTime() const;
			::TunnelEx::TimeT GetLastErrorTime() const;

		private:
			
			class Implementation;
			Implementation *m_pimpl;

		};

	}

	//////////////////////////////////////////////////////////////////////////

	//! The system log. Is a singleton.
	/* It's available at any time of runtime. */
	typedef Singletons::Holder<Singletons::LogPolicy, Singletons::PhoenixLifetime> Log;

#	ifdef LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE
		inline void LogTracking(
					const char *className,
					const char *methodName,
					const char *file,
					unsigned int line)
				throw() {
			Log::GetInstance().AppendTracking(className, methodName, file, line);
		}
#	else // #ifdef LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE
		inline void LogTracking(
					const char *,
					const char *,
					const char *,
					unsigned int)
				throw() {
			/*...*/
		}
#	endif // #ifdef LOG_LEVEL_TRACK_REGISTRATION_AVAILABLE

	//////////////////////////////////////////////////////////////////////////

}

#endif // INCLUDED_FILE_Log__07_02110515
