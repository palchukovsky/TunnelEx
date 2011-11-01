/**************************************************************************
 *   Created: 2009/12/02 1:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__FsLocalStorage_hpp__0912020109
#define INCLUDED_FILE__TUNNELEX__FsLocalStorage_hpp__0912020109

#include "License.hpp"
#include "KeyRetrievePolicies.hpp"
#include "CheckPolicies.hpp"
#include "Core/String.hpp"

namespace TunnelEx { namespace Licensing {

	struct FsLocalStorageState {

		FsLocalStorageState()
				: licenseKeyModificationTime(0) {
			//...//
		}

		time_t licenseKeyModificationTime;

	};

	template<class ClientTrait, bool isTestMode>
	struct LocalStoragePolicy {

		typedef typename ClientTrait::License License;
		typedef typename ClientTrait::KeyRetrieve KeyRetrieve;

#		pragma pack(push, 1)
			struct LicenseDbHead {
				
				LicenseDbHead() 
						: version(1),
						modifTime(0),
						privateKeyLen(0),
						licenseKeyLen(0) {
					//...//
				}
				
				unsigned short version;
				time_t modifTime;
				size_t privateKeyLen;
				size_t licenseKeyLen;
				char licenseUuid[36];

			};
#		pragma pack(pop)

		static std::string GetDbDir() {
			std::vector<char> buffer(MAX_PATH + 1, 0);
			if (!SHGetSpecialFolderPathA(NULL, &buffer[0], CSIDL_COMMON_APPDATA, TRUE)) {
				assert(false);
				return std::string();
			}
			std::string result = &buffer[0];
			result += "\\" TUNNELEX_NAME "\\";
			return result;
		}

		inline static std::string GetDbFilePath() {
			return std::string(GetDbDir() + "Service.license");
		}

		inline static time_t GetDbFileModificationTime() {
			try {
				return boost::filesystem::last_write_time(GetDbFilePath());
			} catch (const std::exception &) {
				return 0;
			}
		}
		inline static void GetFileEncryptingKey(std::vector<unsigned char> &result) {
			std::vector<unsigned char> key;
			/*
				GdsH1\[6r>Ua{Zt3cF7O?<N*OX1@6u57o20}mEOzn_'Trz`jOUY#w4}xt^MGv58np*nxB,VN_?*O7?oz'b0$(b?TwJ{H+!L2`5Se+IL''.uhck8s=m,]-F)h*8e@s!eeYw+S[m#y=`3c;;<7?sc?TNg&L$RL`EiuHc`v'<5z"x-;>ElQs"IQG:"k27G4`n^Q]62&6}3<gg#!/#>U4snizCfGhBLi46rOl-wJb+h}$D_U-G#,\^g9>"e6RQ:7,:>0
				length: 256
			*/
			key.resize(174); key[173] = 'E'; key[172] = '>'; key.resize(246);
			key[245] = '"'; key[25] = 'X'; key[55] = 'x'; key[107] = 'h';
			key[37] = 'E'; key[8] = 'r'; key[94] = 'L'; key[244] = '>';
			key[20] = '?'; key[220] = '4'; key[211] = 'i'; key[44] = 'r';
			key[114] = ','; key[90] = '{'; key[39] = 'z'; key[84] = '(';
			key[202] = '#'; key[174] = 'l'; key[100] = '+'; key[0] = 'G';
			key[148] = 'T'; key[230] = 'h'; key[129] = 'w'; key[87] = 'T';
			key[146] = 'c'; key[33] = '2'; key[108] = 'c'; key[47] = 'j';
			key[179] = 'Q'; key[222] = 'r'; key[80] = '\''; key[135] = 'y';
			key[233] = 'D'; key[185] = '7'; key[56] = 't'; key[164] = '\'';
			key[168] = '"'; key[204] = '/'; key[183] = 'k'; key[24] = 'O';
			key[92] = '+'; key[134] = '#'; key[214] = 'f'; key[187] = '4';
			key.resize(253); key[252] = ','; key[18] = '7'; key[223] = 'O';
			key[34] = '0'; key[112] = '='; key[58] = 'M'; key[159] = 'u';
			key[119] = 'h'; key[149] = 'N'; key[184] = '2'; key[137] = '`';
			key[166] = '5'; key[109] = 'k'; key[1] = 'd'; key[31] = '7';
			key[188] = '`'; key[51] = '#'; key[26] = '1'; key[106] = 'u';
			key[124] = 's'; key[59] = 'G'; key[38] = 'O'; key[104] = '\'';
			key[13] = 'Z'; key[196] = '6'; key[4] = '1'; key[102] = 'L';
			key[118] = ')'; key[139] = 'c'; key[16] = 'c'; key[241] = '^';
			key[15] = '3'; key[157] = 'E'; key[243] = '9'; key[181] = ':';
			key[203] = '!'; key[218] = 'L'; key[23] = '*'; key[36] = 'm';
			key[42] = '\''; key[32] = 'o'; key[93] = '!'; key[228] = 'b';
			key[65] = '*'; key[212] = 'z'; key[151] = '&'; key[175] = 'Q';
			key[197] = '}'; key[49] = 'U'; key[48] = 'O'; key[61] = '5';
			key[67] = 'x'; key[95] = '2'; key[205] = '#'; key[121] = '8';
			key.resize(255); key[254] = '>'; key[250] = ':'; key[155] = 'L';
			key[160] = 'H'; key[120] = '*'; key[224] = 'l'; key[150] = 'g';
			key[170] = '-'; key[231] = '}'; key[123] = '@'; key[145] = 's';
			key[103] = '\''; key[248] = 'R'; key[200] = 'g'; key[45] = 'z';
			key[177] = '"'; key[216] = 'h'; key[97] = '5'; key[207] = 'U';
			key[192] = ']'; key[21] = '<'; key[127] = 'e'; key[249] = 'Q';
			key[130] = '+'; key[105] = '.'; key[69] = ','; key[161] = 'c';
			key[198] = '3'; key[35] = '}'; key[50] = 'Y'; key[162] = '`';
			key[246] = 'e'; key[71] = 'N'; key[11] = 'a'; key[215] = 'G';
			key[82] = '0'; key[144] = '?'; key[133] = 'm'; key[180] = 'G';
			key[41] = '_'; key[210] = 'n'; key[63] = 'n'; key.resize(256);
			key[255] = '0'; key[253] = ':'; key[40] = 'n'; key[83] = '$';
			key[229] = '+'; key[73] = '?'; key[237] = 'G'; key[5] = '\\';
			key[10] = 'U'; key[117] = 'F'; key[115] = ']'; key[199] = '<';
			key[242] = 'g'; key[57] = '^'; key[206] = '>'; key[191] = 'Q';
			key[2] = 's'; key[110] = '8'; key[176] = 's'; key[189] = 'n';
			key[226] = 'w'; key[152] = 'L'; key[221] = '6'; key[14] = 't';
			key[68] = 'B'; key[30] = '5'; key[12] = '{'; key[128] = 'Y';
			key[132] = '['; key[236] = '-'; key[29] = 'u'; key[167] = 'z';
			key[54] = '}'; key[138] = '3'; key[163] = 'v'; key[17] = 'F';
			key[76] = '7'; key[158] = 'i'; key[251] = '7'; key[99] = 'e';
			key[209] = 's'; key[70] = 'V'; key[46] = '`'; key[126] = 'e';
			key[66] = 'n'; key[111] = 's'; key[60] = 'v'; key[122] = 'e';
			key[27] = '@'; key[178] = 'I'; key[147] = '?'; key[88] = 'w';
			key[101] = 'I'; key[85] = 'b'; key[3] = 'H'; key[131] = 'S';
			key[81] = 'b'; key[125] = '!'; key[116] = '-'; key[165] = '<';
			key[28] = '6'; key[79] = 'z'; key[235] = 'U'; key[232] = '$';
			key[74] = '*'; key[9] = '>'; key[169] = 'x'; key[186] = 'G';
			key[201] = 'g'; key[193] = '6'; key[62] = '8'; key[53] = '4';
			key[227] = 'J'; key[143] = '7'; key[77] = '?'; key[156] = '`';
			key[225] = '-'; key[86] = '?'; key[195] = '&'; key[153] = '$';
			key[91] = 'H'; key[72] = '_'; key[98] = 'S'; key[194] = '2';
			key[6] = '['; key[89] = 'J'; key[19] = 'O'; key[136] = '=';
			key[238] = '#'; key[43] = 'T'; key[182] = '"'; key[240] = '\\';
			key[7] = '6'; key[234] = '_'; key[96] = '`'; key[113] = 'm';
			key[22] = 'N'; key[217] = 'B'; key[142] = '<'; key[171] = ';';
			key[213] = 'C'; key[154] = 'R'; key[219] = 'i'; key[78] = 'o';
			key[52] = 'w'; key[239] = ','; key[208] = '4'; key[190] = '^';
			key[247] = '6'; key[140] = ';'; key[64] = 'p'; key[141] = ';';
			key[75] = 'O'; 
			result.swap(key);
		}

		inline static void SetFileContent(
					const LicenseDbHead &head,
					const std::vector<unsigned char> &varData) {
			assert(varData.size() == head.licenseKeyLen + head.privateKeyLen);
			std::vector<unsigned char> fileKey;
			GetFileEncryptingKey(fileKey);
			std::ofstream f(GetDbFilePath().c_str(), std::ios::binary | std::ios::trunc);
			size_t token = 0;
			foreach (char ch, varData) {
				ch ^= fileKey[token++ % fileKey.size()];
				f << ch;
			}
			for (size_t i = 0; i < sizeof(LicenseDbHead); ++i) {
				char ch = *(reinterpret_cast<const char *>(&head) + i);
				ch ^= fileKey[token++ % fileKey.size()];
				f << ch;
			}
		}

		inline static bool GetFileContent(
					LicenseDbHead &head,
					const boost::any &clientParam) {
			std::vector<unsigned char> tmpVarData;
			return GetFileContent(head, tmpVarData, clientParam);
		}

		inline static bool GetFileContent(
					LicenseDbHead &head,
					std::vector<unsigned char> &varData,
					const boost::any &clientParam) {
			assert(varData.size() == head.licenseKeyLen + head.privateKeyLen);
			std::vector<unsigned char> fileKey;
			GetFileEncryptingKey(fileKey);
			std::ifstream f(GetDbFilePath().c_str(), std::ios::binary);
			f.unsetf(std::ios_base::skipws);
			typedef std::istreambuf_iterator<char> Iter; 
			const Iter end;
			size_t token = 0;
			std::vector<unsigned char> decrypted;
			for (Iter i = Iter(f); i != end; ++i) {
				unsigned char ch = *i;
				ch ^= fileKey[token++ % fileKey.size()];
				decrypted.push_back(ch);
			}
			if (decrypted.size() < sizeof LicenseDbHead) {
				License::RegisterError(
					"C6F8BD5B-B50C-4060-A4E5-B155DAEC0EEB",
					decrypted.size(),
					clientParam);
				return false;
			}
			LicenseDbHead headTmp;
			memcpy(
				&headTmp,
				&*(decrypted.rbegin() + sizeof(LicenseDbHead) - 1),
				sizeof(LicenseDbHead));
			decrypted.resize(decrypted.size() - sizeof(LicenseDbHead));
			head = headTmp;
			decrypted.swap(varData);
			return true;
		}

		inline static std::string GetTrialLicense(const boost::any &clientParam) {
			std::string result;
			LicenseDbHead head;
			std::vector<unsigned char> varData;
			if (!GetFileContent(head, varData, clientParam)) {
				result = ConvertString<String>(Helpers::Uuid().GetAsString().c_str()).GetCStr();
				memcpy(head.licenseUuid, result.c_str(), sizeof(head.licenseUuid));
				SetFileContent(head, varData);
			} else {
				std::string(head.licenseUuid, head.licenseUuid + sizeof(head.licenseUuid))
					.swap(result);
			}
			assert(result.size() == 36);
			return result;
		}

		inline static std::string GetLicenseKey(const boost::any &clientParam) {
			LicenseDbHead head;
			std::vector<unsigned char> varData;
			for (size_t i = 1; i <= 2; ++i) {
				GetFileContent(head, varData, clientParam);
				break;
				/* if (all.size()) {
					break;
				}
				using namespace Crypto;
				const Rsa rsa;
				std::vector<unsigned char> key;
				Seale seale(key, rsa.GetPublicKey());
				std::string keyEncrypted(seale.GetSealed().begin(), seale.GetSealed().end());
				copy(seale.GetEnvKey().begin(), seale.GetEnvKey().end(), back_inserter(keyEncrypted));
				format keyFormated("%1%%2% %3% %4% %5%%1%\r\n%6%\r\n%1%%7% %3% %4% %5%%1%\r\n");
				keyFormated
					% "-----"
					% "BEGIN"
					% "TUNNELEX"
					% "LICENSE"
					% "KEY"
					% keyEncrypted
					% "END";
				StoreLicenseKey(keyFormated.str(), rsa.GetPrivateKey().Export()); */
			}
			if (varData.size() < head.licenseKeyLen) {
				License::RegisterError(
					"1CE91D56-F2D9-4A5D-8C1B-3863C7206E66",
					varData.size(),
					clientParam);
				return std::string();
			}
			return std::string(
				varData.begin(),
				varData.begin() + head.licenseKeyLen);
		}

		inline static std::string GetLocalAsymmetricPrivateKey(
					const boost::any &clientParam) {
			LicenseDbHead head;
			std::vector<unsigned char> varData;
			GetFileContent(head, varData, clientParam);
			if (varData.size() < head.licenseKeyLen + head.privateKeyLen) {
				License::RegisterError(
					"723181DF-962C-42B5-8E0B-0637AC722CDC",
					varData.size(),
					clientParam);
				return std::string();
			}
			return std::string(
				varData.begin() + head.licenseKeyLen,
				varData.begin() + head.licenseKeyLen + head.privateKeyLen);
		}
		
		inline static void StoreLicenseKey(
					const std::string &licenseKey,
					const std::string &privateKey,
					const boost::any &clientParam) {
			LicenseDbHead head;
			GetFileContent(head, clientParam);
			std::vector<unsigned char> varData(licenseKey.begin(), licenseKey.end());
			copy(privateKey.begin(), privateKey.end(), back_inserter(varData));
			head.licenseKeyLen = licenseKey.size();
			head.privateKeyLen = privateKey.size();
			const std::string license
				= License::GetLicense(KeyRetrieve::Import(licenseKey, privateKey));
			memcpy(head.licenseUuid, license.c_str(), sizeof(head.licenseUuid));
			SetFileContent(head, varData);
		}

		inline static bool IsLicenseKeyChanged(const boost::any &clientParam) {
			if (clientParam.empty()) {
				return false;
			}
			FsLocalStorageState &state = *boost::any_cast<FsLocalStorageState *>(clientParam);
			return state.licenseKeyModificationTime != GetDbFileModificationTime();
		}

		inline static void ResetLicenseKeyUpdateState(const boost::any &clientParam) {
			if (clientParam.empty()) {
				return;
			}
			FsLocalStorageState &state = *boost::any_cast<FsLocalStorageState *>(clientParam);
			state.licenseKeyModificationTime = GetDbFileModificationTime();
		}

	};

} }

#endif // INCLUDED_FILE__TUNNELEX__FsLocalStorage_hpp__0912020109
