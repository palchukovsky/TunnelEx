/**************************************************************************
 *   Created: 2008/06/12 18:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: Dll.hpp 1129 2011-02-22 17:28:50Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Dll_hpp__0806121823
#define INCLUDED_FILE__TUNNELEX__Dll_hpp__0806121823

#include "Core/Exceptions.hpp"
#include "Core/String.hpp"
#include "Core/Error.hpp"
#include "Format.hpp"
#include <boost/noncopyable.hpp>
#include <Windows.h>

namespace TunnelEx { namespace Helpers {

	//////////////////////////////////////////////////////////////////////////

	class Dll : private boost::noncopyable {

	public:

		//! Could load DLL.
		class DllLoadException : public TunnelEx::DllException {
		public:
			explicit DllLoadException(
						const char *const dllFile,
						const ::TunnelEx::Error &error)
					: DllException(
						(WFormat(L"Failed to load DLL file \"%1%\" (%2%, %3%).")
								% TunnelEx::ConvertString<TunnelEx::WString>(dllFile).GetCStr()
								% error.GetString().GetCStr()
								% error.GetErrorNo())
							.str().c_str()) {
				//...//
			}
		};

		//! Could find required function in DLL.
		class DllFuncException : public TunnelEx::DllException {
		public:
			explicit DllFuncException(
						const char *const dllFile,
						const char *const funcName,
						const ::TunnelEx::Error &error)
					: DllException(
						(WFormat(L"Failed to find function \"%2%\" in DLL \"%1%\" (%3%, %4%).")
								% TunnelEx::ConvertString<TunnelEx::WString>(dllFile).GetCStr()
								% TunnelEx::ConvertString<TunnelEx::WString>(funcName).GetCStr()
								% error.GetString().GetCStr()
								% error.GetErrorNo())
							.str().c_str()) {
				//...//
			}
		};

	public:

		explicit Dll(const char *const dllFile)
				: m_file(dllFile),
				m_handle(LoadLibraryA(m_file.c_str())) {
			if (m_handle == NULL) {
				throw DllLoadException(
					m_file.c_str(),
					TunnelEx::Error(::GetLastError()));
			}
		}

		~Dll() throw() {
			FreeLibrary(m_handle);
		}

	public:

		const std::string & GetFile() const {
			return m_file;
		}

	public:

		template<class Func>
		typename Func * GetFunction(const char *const funcName) const {
			FARPROC procAddr = GetProcAddress(m_handle, funcName);
			if (procAddr == NULL) {
				throw DllFuncException(
					m_file.c_str(),
					funcName,
					TunnelEx::Error(::GetLastError()));
			}
			return reinterpret_cast<typename Func *>(procAddr);
		}

	private:

		const std::string m_file;
		HMODULE m_handle;

	};

	//////////////////////////////////////////////////////////////////////////

	//! Holder for object pointer, that was received from a DLL.
	/** Closes dll only after object will be destroyed.
	  * @sa: ::TunnelEx::Helpers::Dll;
	  */
	template<class T>
	class DllObjectPtr {
	public:
		explicit DllObjectPtr(
					boost::shared_ptr<const Dll> dll,
					boost::shared_ptr<typename T> objFormDll)
				: m_dll(dll),
				m_objFormDll(objFormDll) {
			//...//
		}
		~DllObjectPtr() {
			//...//
		}
	public:
		T & Get() {
			return *m_objFormDll;
		}
		const T & Get() const {
			return (const_cast<DllObject *>(this))->Get();
		}
	private:
		boost::shared_ptr<const Dll> m_dll;
		boost::shared_ptr<typename T> m_objFormDll;
	};

	//////////////////////////////////////////////////////////////////////////

} }

#endif // INCLUDED_FILE__TUNNELEX__Dll_hpp__0806121823
