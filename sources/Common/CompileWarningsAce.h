/**************************************************************************
 *   Created: 2009/04/13 21:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: CompileWarningsAce.h 1046 2010-11-02 12:07:07Z palchukovsky $
 **************************************************************************/

#ifndef _DEBUG
#	ifndef TEX_ACE_COMPILE_WARNINGS_DISABLED
#		define TEX_ACE_COMPILE_WARNINGS_DISABLED
#		pragma warning(push, 3)
#	else // #ifndef TEX_ACE_COMPILE_WARNINGS_DISABLED
#		undef TEX_ACE_COMPILE_WARNINGS_DISABLED
#		pragma warning(pop)
#	endif  // #ifndef TEX_ACE_COMPILE_WARNINGS_DISABLED
#endif // #ifndef _DEBUG
