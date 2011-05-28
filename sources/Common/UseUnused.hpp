
/**************************************************************************
 *   Created: 2011/05/02 17:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__UseUnused_hpp__1105021748
#define INCLUDED_FILE__TUNNELEX__UseUnused_hpp__1105021748

template<typename T>
inline void UseUnused(const T &) {
	//...//
}

template<typename T1, typename T2>
inline void UseUnused(const T1 &, const T2 &) {
	//...//
}

template<typename T1, typename T2, typename T3>
inline void UseUnused(const T1 &, const T2 &, const T3 &) {
	//...//
}

#endif // INCLUDED_FILE__TUNNELEX__UseUnused_hpp__1105021748
