/**************************************************************************
 *   Created: 2008/08/28 11:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__IoHandle_h__0808281149
#define INCLUDED_FILE__TUNNELEX__IoHandle_h__0808281149

namespace TunnelEx {

	struct IoHandleInfo {

		enum Type {
			TYPE_SOCKET,
			TYPE_OTHER
		};

		typedef void * Handle;

		explicit IoHandleInfo(Handle handle, Type type)
				: handle(handle),
				type(type) {
			//...//
		}

		Handle handle;
		Type type;

	};

}

#endif // INCLUDED_FILE__TUNNELEX__IoHandle_h__0808281149
