/**************************************************************************
 *   Created: 2010/11/25 23:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * -------------------------------------------------------------------
 *       $Id: LocalComunicationPolicy.hpp 1072 2010-11-25 20:02:26Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LocalComunicationPolicy_hpp__1011252338
#define INCLUDED_FILE__TUNNELEX__LocalComunicationPolicy_hpp__1011252338

#include <vector>

namespace TunnelEx { namespace Licensing {

	struct LocalComunicationPolicy {

		inline static void GetEncryptingKey(std::vector<unsigned char> &result) {
			std::vector<unsigned char> key;
			/*
				Q<$w'>[(%J;]P*>uL'LMW!RD:OGdo-X[!U-7OvFfs]OXu?NanTCe3.U ]%_+S\u)nlRF1D2?\xp
				length: 75
			 */
			key.resize(37); key[36] = 'O'; key.resize(46); key[45] = '?';
			key[40] = 's'; key.resize(66); key[65] = 'l'; key[7] = '(';
			key[0] = 'Q'; key[15] = 'u'; key[62] = 'u'; key[59] = '+';
			key[12] = 'P'; key.resize(67); key[66] = 'R'; key[11] = ']';
			key[55] = ' '; key[58] = '_'; key[29] = '-'; key[60] = 'S';
			key[44] = 'u'; key[46] = 'N'; key[3] = 'w'; key[4] = '\'';
			key[49] = 'T'; key[30] = 'X'; key[26] = 'G'; key[38] = 'F';
			key[8] = '%'; key[22] = 'R'; key[39] = 'f'; key[27] = 'd';
			key[42] = 'O'; key[48] = 'n'; key[64] = 'n'; key[63] = ')';
			key.resize(70); key[69] = 'D'; key[43] = 'X'; key[28] = 'o';
			key.resize(71); key[70] = '2'; key.resize(73); key[72] = '\\';
			key[41] = ']'; key[20] = 'W'; key[47] = 'a'; key[5] = '>';
			key[18] = 'L'; key[53] = '.'; key[61] = '\\'; key[14] = '>';
			key[13] = '*'; key[16] = 'L'; key[67] = 'F'; key[2] = '$';
			key[57] = '%'; key[56] = ']'; key[51] = 'e'; key[25] = 'O';
			key[32] = '!'; key[24] = ':'; key[19] = 'M'; key[37] = 'v';
			key[17] = '\''; key[68] = '1'; key[52] = '3'; key[54] = 'U';
			key.resize(74); key[73] = 'x'; key.resize(75); key[74] = 'p';
			key[10] = ';'; key[50] = 'C'; key[21] = '!'; key[34] = '-';
			key[31] = '['; key[71] = '?'; key[6] = '['; key[1] = '<';
			key[23] = 'D'; key[9] = 'J'; key[35] = '7'; key[33] = 'U';
			result.swap(key);
		}


	};

} }

#endif // INCLUDED_FILE__TUNNELEX__LocalComunicationPolicy_hpp__1011252338
