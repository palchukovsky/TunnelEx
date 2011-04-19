/**************************************************************************
 *   Created: 2009/10/25 10:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2009 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__StoragePolicies_hpp__0910251050
#define INCLUDED_FILE__TUNNELEX__StoragePolicies_hpp__0910251050

namespace TunnelEx { namespace Licensing {

	//////////////////////////////////////////////////////////////////////////

	//! Storage policy.
	/** Should has methods:
	  *   - inline static std::string GetLicenseKey():
	  *      return encrypted license key
	  *   - inline static std::string GetLocalAsymmetricPrivateKey():
	  *      return current local private key
	  *   - inline static void StoreLicenseKey(
	  *				const std::string &licenseKey,
	  *				const std::string &privateKey,
	  *				const boost::any &clientParam):
	  *      stores local private key and license key
	  *   - inline static std::string GetLicense():
	  *      return current license 
	  *   - inline static bool IsLicenseKeyChanged(const boost::any &clientParam);
	  *   - inline static void ResetLicenseKeyUpdateState(const boost::any &clientParam);
	  */
	template<class ClientTrait, bool isTestMode>
	struct LocalStoragePolicy;

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait, bool isTestMode>
	struct ConstantStoragePolicy {

		inline static void GetLocalSymmetricKey(std::vector<unsigned char> &result) {
			std::vector<unsigned char> key;
			/*
				qJkf37t)xg-QGgUM)xLz<h0G7jbrtm5(YHDd9(V<OnvE-Ch5H>D4riMxe57KfVaz9kcNZsw[VtBwSnPKM6q73SEkVPB2SW ibxv1HR N4WW-cbbE>3EcAYHZex-(U9{3}qTayAdzpVlcrnN>qYfjDbUZXX_JoWPqO7AITAwW5k68sFnWBtCHCHJvu_lLb50Tx233k5mUee{eT5GccfqNR2 CPMNU}QzJ8Dp(V4)(DZzWLxWh}5S4O3ic4J0iXvt}
				length: 256
			 */
			key.resize(241); key[240] = '}'; key[154] = '_'; key[8] = 'x';
			key[9] = 'g'; key[56] = 'e'; key[4] = '3'; key[64] = '9';
			key[49] = '>'; key[176] = 'B'; key[208] = 'c'; key[59] = 'K';
			key[38] = 'V'; key[130] = 'T'; key[112] = '>'; key[60] = 'f';
			key[128] = '}'; key[205] = '5'; key[151] = 'Z'; key.resize(252);
			key[251] = 'i'; key[142] = 'N'; key[33] = 'H'; key[94] = ' ';
			key[169] = 'k'; key[231] = '('; key[186] = 'l'; key[127] = '3';
			key[90] = 'B'; key[238] = 'W'; key[138] = 'l'; key[50] = 'D';
			key[98] = 'v'; key[156] = 'o'; key[113] = '3'; key[41] = 'n';
			key[232] = 'D'; key[107] = '-'; key[15] = 'M'; key[69] = 's';
			key[10] = '-'; key[65] = 'k'; key[237] = 'x'; key[83] = '7';
			key[226] = 'p'; key[18] = 'L'; key[227] = '('; key[152] = 'X';
			key[228] = 'V'; key[179] = 'H'; key[218] = 'N'; key[175] = 'W';
			key[80] = 'M'; key[30] = '5'; key[3] = 'f'; key[180] = 'C';
			key[31] = '('; key[184] = 'u'; key[19] = 'z'; key[196] = 'k';
			key[108] = 'c'; key[109] = 'b'; key[162] = 'A'; key[173] = 'F';
			key[250] = '0'; key[23] = 'G'; key[13] = 'g'; key[134] = 'd';
			key[129] = 'q'; key[105] = 'W'; key[199] = 'U'; key[167] = 'W';
			key[11] = 'Q'; key[168] = '5'; key[220] = '}'; key[48] = 'H';
			key[37] = '('; key[204] = 'T'; key[210] = 'q'; key[73] = 't';
			key[55] = 'x'; key[22] = '0'; key[74] = 'B'; key[6] = 't';
			key[119] = 'Z'; key[106] = 'W'; key[123] = '('; key[1] = 'J';
			key[100] = 'H'; key[75] = 'w'; key[40] = 'O'; key[189] = '5';
			key[187] = 'L'; key[236] = 'L'; key[188] = 'b'; key[217] = 'M';
			key[93] = 'W'; key[32] = 'Y'; key[89] = 'P'; key[153] = 'X';
			key[243] = '4'; key[16] = ')'; key[137] = 'V'; key[200] = 'e';
			key[77] = 'n'; key[249] = 'J'; key[214] = ' '; key[58] = '7';
			key[61] = 'V'; key[202] = '{'; key[219] = 'U'; key[157] = 'W';
			key[241] = '5'; key[201] = 'e'; key[163] = 'I'; key[117] = 'Y';
			key[52] = 'r'; key[53] = 'i'; key[150] = 'U'; key[0] = 'q';
			key[139] = 'c'; key.resize(256); key[255] = '}'; key[87] = 'k';
			key[209] = 'f'; key[155] = 'J'; key[133] = 'A'; key[44] = '-';
			key[206] = 'G'; key[207] = 'c'; key[97] = 'x'; key[88] = 'V';
			key[254] = 't'; key[197] = '5'; key[57] = '5'; key[96] = 'b';
			key[135] = 'z'; key[212] = 'R'; key[27] = 'r'; key[71] = '[';
			key[132] = 'y'; key[45] = 'C'; key[211] = 'N'; key[223] = 'J';
			key[36] = '9'; key[192] = 'x'; key[216] = 'P'; key[149] = 'b';
			key[222] = 'z'; key[67] = 'N'; key[194] = '3'; key[42] = 'v';
			key[245] = '3'; key[145] = 'Y'; key[160] = 'O'; key[165] = 'A';
			key[62] = 'a'; key[91] = '2'; key[92] = 'S'; key[104] = '4';
			key[215] = 'C'; key[54] = 'M'; key[39] = '<'; key[76] = 'S';
			key[34] = 'D'; key[81] = '6'; key[86] = 'E'; key[198] = 'm';
			key[233] = 'Z'; key[146] = 'f'; key[253] = 'v'; key[17] = 'x';
			key[12] = 'G'; key[20] = '<'; key[95] = 'i'; key[190] = '0';
			key[203] = 'e'; key[224] = '8'; key[230] = ')'; key[185] = '_';
			key[7] = ')'; key[159] = 'q'; key[140] = 'r'; key[35] = 'd';
			key[101] = 'R'; key[166] = 'w'; key[116] = 'A'; key[244] = 'O';
			key[29] = 'm'; key[46] = 'h'; key[78] = 'P'; key[181] = 'H';
			key[24] = '7'; key[252] = 'X'; key[182] = 'J'; key[103] = 'N';
			key[102] = ' '; key[234] = 'z'; key[195] = '3'; key[171] = '8';
			key[246] = 'i'; key[111] = 'E'; key[110] = 'b'; key[51] = '4';
			key[235] = 'W'; key[144] = 'q'; key[221] = 'Q'; key[21] = 'h';
			key[229] = '4'; key[143] = '>'; key[79] = 'K'; key[177] = 't';
			key[70] = 'w'; key[183] = 'v'; key[118] = 'H'; key[174] = 'n';
			key[161] = '7'; key[82] = 'q'; key[239] = 'h'; key[242] = 'S';
			key[26] = 'b'; key[148] = 'D'; key[2] = 'k'; key[248] = '4';
			key[247] = 'c'; key[66] = 'c'; key[120] = 'e'; key[147] = 'j';
			key[191] = 'T'; key[99] = '1'; key[72] = 'V'; key[125] = '9';
			key[164] = 'T'; key[193] = '2'; key[5] = '7'; key[172] = 's';
			key[213] = '2'; key[126] = '{'; key[170] = '6'; key[225] = 'D';
			key[84] = '3'; key[122] = '-'; key[68] = 'Z'; key[124] = 'U';
			key[28] = 't'; key[43] = 'E'; key[121] = 'x'; key[136] = 'p';
			key[63] = 'z'; key[158] = 'P'; key[47] = '5'; key[131] = 'a';
			key[115] = 'c'; key[178] = 'C'; key[14] = 'U'; key[141] = 'n';
			key[114] = 'E'; key[25] = 'j'; key[85] = 'S';
			key.swap(result);
		}
	
		inline static void GetLicenseServerAsymmetricPublicKey(
					std::vector<unsigned char> &result) {
			std::vector<unsigned char> key;
			/*
				-----BEGIN PUBLIC KEY-----
				MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCVP26uAiI3edw651pN7mhJkJyY
				/HXAUhJGa0OgNa3uSsdgMwhz6rRWZW0jOcEAuys5fbl36IQxWubnNbn51I0WAo51
				Sj4746MXxdO8rGGEsoEmfkXXFo7AQrgSQDDjtnsdHmkmhaSm+/tfY9WDMRID0g1q
				dKHNVyqL/n+7QVI3hwIDAQAB
				-----END PUBLIC KEY-----

				length: 272
			 */
			key.resize(111); key[110] = 'd'; key[107] = 'u'; key[100] = 'a';
			key.resize(119); key[118] = 'R'; key.resize(266); key[265] = 'Y';
			key[164] = 'X'; key[143] = 'n'; key[168] = '8'; key[198] = 'm';
			key[262] = ' '; key[201] = 'h'; key[20] = 'Y'; key[32] = 'A';
			key[98] = 'J'; key[249] = '-'; key[161] = '4'; key[8] = 'I';
			key[220] = 'q'; key[224] = 'H'; key[22] = '-'; key[175] = 'E';
			key[148] = '1'; key[214] = 'R'; key[142] = 'b'; key[128] = 'u';
			key[36] = 'S'; key[206] = '/'; key[158] = 'j'; key[248] = '-';
			key[40] = 'I'; key[130] = 's'; key[131] = '5'; key[203] = 'S';
			key[35] = 'C'; key[106] = '3'; key[181] = 'F'; key[1] = '-';
			key[162] = '6'; key[235] = 'V'; key[227] = 'y'; key[177] = 'f';
			key[6] = 'E'; key[115] = 'z'; key[132] = 'f'; key[95] = 'A';
			key[191] = 'D'; key[70] = 'u'; key[133] = 'b'; key[129] = 'y';
			key[79] = '5'; key[147] = '5'; key[219] = '1'; key[14] = 'L';
			key[59] = 'i'; key[138] = 'Q'; key[182] = 'o'; key[50] = 'A';
			key[150] = '0'; key[167] = 'O'; key[116] = '6'; key[192] = 'j';
			key[85] = 'h'; key[173] = 's'; key[195] = 's'; key[154] = '5';
			key[171] = 'G'; key[245] = 'B'; key[229] = 'L'; key.resize(269);
			key[268] = '-'; key[252] = 'E'; key[244] = 'A'; key[76] = 'd';
			key[231] = 'n'; key[12] = 'U'; key[208] = 'f'; key[62] = 'B';
			key[267] = '-'; key[56] = 'D'; key[5] = 'B'; key[124] = 'O';
			key[225] = 'N'; key[174] = 'o'; key[212] = 'D'; key[232] = '+';
			key[155] = '1'; key[122] = '0'; key[81] = 'p'; key[205] = '+';
			key[146] = 'n'; key[63] = 'g'; key[134] = 'l'; key[39] = 'S';
			key[185] = 'Q'; key[102] = 'O'; key.resize(272); key[188] = 'S';
			key[52] = '4'; key[90] = 'Y'; key[187] = 'g'; key[55] = 'A';
			key[255] = ' '; key[72] = 'i'; key[193] = 't'; key[256] = 'P';
			key[57] = 'C'; key[139] = 'x'; key[190] = 'D'; key[44] = 'Q';
			key[264] = 'E'; key[270] = '-'; key[74] = '3'; key[170] = 'G';
			key[237] = '3'; key[93] = 'H'; key[19] = 'E'; key[202] = 'a';
			key[68] = '2'; key[27] = 'M'; key[71] = 'A'; key[88] = 'J';
			key[135] = '3'; key[236] = 'I'; key[103] = 'g'; key[28] = 'I';
			key[25] = '-'; key[169] = 'r'; key[157] = 'S'; key[145] = 'b';
			key[144] = 'N'; key[211] = 'W'; key[189] = 'Q'; key[21] = '-';
			key[46] = 'B'; key[140] = 'W'; key[13] = 'B'; key[105] = 'a';
			key[242] = 'A'; key[18] = 'K'; key[261] = 'C'; key[266] = '-';
			key[223] = 'K'; key[196] = 'd'; key[33] = '0'; key[137] = 'I';
			key[136] = '6'; key[126] = 'E'; key[127] = 'A'; key[10] = ' ';
			key[64] = 'Q'; key[199] = 'k'; key[45] = 'E'; key[165] = 'x';
			key[213] = 'M'; key[38] = 'G'; key[186] = 'r'; key[176] = 'm';
			key[96] = 'U'; key[111] = 'g'; key[251] = '-'; key[226] = 'V';
			key[37] = 'q'; key[3] = '-'; key[209] = 'Y'; key[97] = 'h';
			key[238] = 'h'; key[163] = 'M'; key[179] = 'X'; key[152] = 'A';
			key[259] = 'L'; key[210] = '9'; key[108] = 'S'; key[112] = 'M';
			key[43] = 'D'; key[67] = 'P'; key[16] = 'C'; key[151] = 'W';
			key[253] = 'N'; key[53] = 'G'; key[141] = 'u'; key[58] = 'B';
			key[254] = 'D'; key[9] = 'N'; key[29] = 'G'; key[149] = 'I';
			key[120] = 'Z'; key[218] = 'g'; key[243] = 'Q'; key[80] = '1';
			key[215] = 'I'; key[0] = '-'; key[17] = ' '; key[125] = 'c';
			key[123] = 'j'; key[207] = 't'; key[24] = '-'; key[99] = 'G';
			key[240] = 'I'; key[65] = 'C'; key[54] = 'N'; key[222] = 'd';
			key[180] = 'X'; key[49] = 'U'; key[257] = 'U'; key[233] = '7';
			key[216] = 'D'; key[166] = 'd'; key[15] = 'I'; key[23] = '-';
			key[204] = 'm'; key[153] = 'o'; key[34] = 'G'; key[114] = 'h';
			key[89] = 'y'; key[197] = 'H'; key[87] = 'k'; key[47] = 'A';
			key[172] = 'E'; key[86] = 'J'; key[73] = 'I'; key[228] = 'q';
			key[83] = '7'; key[217] = '0'; key[117] = 'r'; key[247] = '-';
			key[160] = '7'; key[183] = '7'; key[260] = 'I'; key[119] = 'W';
			key[101] = '0'; key[7] = 'G'; key[82] = 'N'; key[113] = 'w';
			key[69] = '6'; key[178] = 'k'; key[78] = '6'; key[269] = '-';
			key[200] = 'm'; key[84] = 'm'; key[230] = '/'; key[104] = 'N';
			key[75] = 'e'; key[61] = 'K'; key[31] = 'M'; key[258] = 'B';
			key[30] = 'f'; key[2] = '-'; key[241] = 'D'; key[4] = '-';
			key[92] = '/'; key[77] = 'w'; key[60] = 'Q'; key[66] = 'V';
			key[184] = 'A'; key[234] = 'Q'; key[48] = 'Q'; key[94] = 'X';
			key[51] = 'A'; key[42] = '3'; key[250] = '-'; key[194] = 'n';
			key[159] = '4'; key[41] = 'b'; key[121] = 'W'; key[263] = 'K';
			key[239] = 'w'; key[109] = 's'; key[11] = 'P'; key[91] = '\n';
			key[271] = '\n'; key[156] = '\n'; key[26] = '\n'; key[221] = '\n';
			key[246] = '\n';
			key.swap(result);
		}
	
	};

	//////////////////////////////////////////////////////////////////////////

} }

#endif // INCLUDED_FILE__TUNNELEX__StoragePolicies_hpp__0910251050
