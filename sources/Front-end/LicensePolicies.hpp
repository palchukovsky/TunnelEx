/**************************************************************************
 *   Created: 2009/12/06 6:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: 2007 - 2009 Eugene V. Palchukovsky
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__LicensePolicies_hpp__0912060624
#define INCLUDED_FILE__TUNNELEX__LicensePolicies_hpp__0912060624

#include "ServiceAdapter.hpp"

#include "Licensing/WinInetCommPolicy.hpp"
#include "Licensing/License.hpp"
#include "Licensing/LocalComunicationPolicy.hpp"

//////////////////////////////////////////////////////////////////////////

struct LicenseState {

	//! Gets license key from local DB only if it changed.
	explicit LicenseState(ServiceAdapter &service, time_t &licenseKeyModificationTime)
			: service(&service),
			licenseKeyModificationTime(&licenseKeyModificationTime) {
		//...//
	}

	//! Always gets license key from local DB.
	explicit LicenseState(ServiceAdapter &service)
			: service(&service),
			licenseKeyModificationTime(0) {
		//...//
	}

	ServiceAdapter *service;
	time_t *licenseKeyModificationTime;

	template<class License>
	static wxString GetAsString(
				const License &license,
				const wxString &noActivationText) {

		namespace pt = boost::posix_time;

		const wxString ownerName = license.IsTrial()
			?	wxT("Trail license")
			:	wxString::FromAscii(license.GetOwner().c_str());
	
		wxString state;
		if (ownerName.IsEmpty()) {
			if (license.GetUnactivityReason() == UR_NOT_EXISTS) {
				state = noActivationText;
			} else {
				state = wxT("Error at license info getting");
			}
		} else if (!license.IsTrial()) {
			state = wxT("Licensed to ") + ownerName;
		} else {
			state = ownerName;
		}

		{
		
			const boost::optional<pt::ptime> timeFromLimit
				= license.GetLimitationTimeFrom();
			const boost::optional<pt::ptime> timeToLimit
				= license.GetLimitationTimeTo();

			if (timeFromLimit || timeToLimit) {
	
				const pt::time_duration zoneDiff
					= pt::second_clock::local_time() - pt::second_clock::universal_time();

				std::auto_ptr<pt::wtime_facet> facet(new pt::wtime_facet(L"%d %B %Y"));
				std::locale locFrom(std::cout.getloc(), facet.get());
				facet.release();
				facet.reset(new pt::wtime_facet(L"%B, %d %Y"));
				std::locale locTo(std::cout.getloc(), facet.get());
				facet.release();

				std::wstringstream ss;
				ss << L"\nCurrent activation is valid ";
				if (timeFromLimit && !timeToLimit) {
					ss.imbue(locFrom);
					ss << L"from " << (*timeFromLimit + zoneDiff);
				} else if (timeToLimit && !timeFromLimit) {
					ss.imbue(locTo);
					ss << L"until " << (*timeToLimit + zoneDiff);
				} else {
					ss.imbue(locFrom);
					ss << L"from ";
					ss << (*timeFromLimit + zoneDiff);
					ss.imbue(locTo);
					ss << L" to " << (*timeToLimit + zoneDiff);
				}
				state += ss.str();

			}
	
		}

		return state;

	}

};

//////////////////////////////////////////////////////////////////////////

struct LicenseDataEncryption {


	inline static void GetOfflineActivationPrivateKeyEncryptingKey(
				std::vector<unsigned char> &result) {
		std::vector<unsigned char> key;
		/*
			Jy sf!X^SWLAul]YIXQZAHdQ)BZHK=; \ITXlQr+n_RSqMT#kiU}tnrON!2&@_jp\KGZE.A\h=)VK>Wu;`/LKp,ISf[)i$-y]*W_u0z>oW,5aS_F4ULON&1Nw)Xj,^57S')u dcQS-e+iA{+p5rq6>0L?zl89SBIWAtb^j5e5+d5b*6{=@B17y&P0UG?A"v_>)X;/,vz:+q.zoS44% 6j3 V@S1WS9+KY(q07`5"32s!wUc6x"_=Z) {6p._p1Ub
			length: 256
		*/
		key.resize(195); key[194] = 'X'; key[67] = 'Z'; key[32] = '\\';
		key[82] = '/'; key[155] = '8'; key[105] = 'W'; key[5] = '!';
		key[45] = 'M'; key.resize(196); key[195] = ';'; key.resize(201);
		key[200] = ':'; key[35] = 'X'; key[19] = 'Z'; key.resize(205);
		key[204] = 'z'; key.resize(232); key[231] = '"'; key[117] = '&';
		key[226] = 'q'; key[52] = 't'; key[198] = 'v'; key[221] = '9';
		key.resize(247); key[246] = ' '; key[2] = ' '; key[91] = ')';
		key[201] = '+'; key[182] = '&'; key[224] = 'Y'; key[102] = 'z';
		key[120] = 'w'; key[37] = 'Q'; key[158] = 'B'; key[207] = '4';
		key[50] = 'U'; key[215] = 'V'; key[241] = '"'; key[114] = 'L';
		key[147] = 'q'; key[106] = ','; key[97] = '*'; key[39] = '+';
		key[66] = 'G'; key[22] = 'd'; key[99] = '_'; key[128] = 'S';
		key[166] = '5'; key[189] = '"'; key[218] = '1'; key[53] = 'n';
		key[211] = '6'; key[25] = 'B'; key[108] = 'a'; key[167] = 'e';
		key[113] = 'U'; key[70] = 'A'; key[69] = '.'; key[153] = 'z';
		key[173] = '*'; key[163] = 'b'; key[7] = '^'; key[191] = '_';
		key[133] = 'd'; key[27] = 'H'; key[94] = '-'; key[131] = 'u';
		key[227] = '0'; key[236] = 'w'; key[116] = 'N'; key[112] = '4';
		key[130] = ')'; key[192] = '>'; key[230] = '5'; key[212] = 'j';
		key[103] = '>'; key[185] = 'U'; key[13] = 'l'; key[197] = ',';
		key[48] = 'k'; key[142] = '{'; key[11] = 'A'; key[40] = 'n';
		key[125] = '^'; key.resize(248); key[247] = '{'; key[89] = 'f';
		key[109] = 'S'; key[186] = 'G'; key[121] = ')'; key[38] = 'r';
		key[137] = '-'; key[46] = 'T'; key[232] = '3'; key[149] = '>';
		key[21] = 'H'; key[229] = '`'; key[209] = '%'; key[92] = 'i';
		key[183] = 'P'; key[88] = 'S'; key[81] = '`'; key[152] = '?';
		key[216] = '@'; key[172] = 'b'; key[9] = 'W'; key[101] = '0';
		key[23] = 'Q'; key[62] = 'j'; key[222] = '+'; key[118] = '1';
		key[151] = 'L'; key[107] = '5'; key[165] = 'j'; key.resize(250);
		key[249] = 'p'; key[28] = 'K'; key[12] = 'u'; key[190] = 'v';
		key[180] = '7'; key[78] = 'W'; key[60] = '@'; key[119] = 'N';
		key[86] = ','; key[34] = 'T'; key[157] = 'S'; key[90] = '[';
		key[29] = '='; key[124] = ','; key[31] = ' '; key[18] = 'Q';
		key[243] = '='; key[47] = '#'; key[139] = '+'; key[115] = 'O';
		key[41] = '_'; key[76] = 'K'; key[64] = '\\'; key[175] = '{';
		key[217] = 'S'; key[205] = 'o'; key[30] = ';'; key[234] = 's';
		key.resize(251); key[250] = '.'; key[58] = '2'; key[188] = 'A';
		key[233] = '2'; key[51] = '}'; key[193] = ')'; key[98] = 'W';
		key[15] = 'Y'; key[138] = 'e'; key[176] = '='; key[26] = 'Z';
		key[196] = '/'; key[160] = 'W'; key[83] = 'L'; key[74] = ')';
		key[220] = 'S'; key[0] = 'J'; key[136] = 'S'; key[225] = '(';
		key[20] = 'A'; key[235] = '!'; key[71] = '\\'; key[245] = ')';
		key[159] = 'I'; key[6] = 'X'; key[3] = 's'; key[244] = 'Z';
		key[178] = 'B'; key[95] = 'y'; key[132] = ' '; key[75] = 'V';
		key[237] = 'U'; key[187] = '?'; key[240] = 'x'; key[57] = '!';
		key[43] = 'S'; key[80] = ';'; key[79] = 'u'; key[223] = 'K';
		key[171] = '5'; key[214] = ' '; key[181] = 'y'; key[87] = 'I';
		key[170] = 'd'; key[210] = ' '; key[110] = '_'; key[179] = '1';
		key[219] = 'W'; key[154] = 'l'; key[123] = 'j'; key.resize(255);
		key[254] = 'U'; key[10] = 'L'; key[77] = '>'; key[73] = '=';
		key[169] = '+'; key[248] = '6'; key[85] = 'p'; key[59] = '&';
		key[16] = 'I'; key[140] = 'i'; key[228] = '7'; key[61] = '_';
		key[104] = 'o'; key.resize(256); key[255] = 'b'; key[135] = 'Q';
		key[177] = '@'; key[238] = 'c'; key[206] = 'S'; key[251] = '_';
		key[63] = 'p'; key[100] = 'u'; key[49] = 'i'; key[14] = ']';
		key[184] = '0'; key[203] = '.'; key[143] = '+'; key[253] = '1';
		key[68] = 'E'; key[33] = 'I'; key[168] = '5'; key[54] = 'r';
		key[72] = 'h'; key[36] = 'l'; key[199] = 'z'; key[208] = '4';
		key[129] = '\''; key[239] = '6'; key[162] = 't'; key[127] = '7';
		key[174] = '6'; key[44] = 'q'; key[213] = '3'; key[145] = '5';
		key[148] = '6'; key[111] = 'F'; key[42] = 'R'; key[141] = 'A';
		key[252] = 'p'; key[1] = 'y'; key[8] = 'S'; key[56] = 'N';
		key[144] = 'p'; key[65] = 'K'; key[93] = '$'; key[24] = ')';
		key[156] = '9'; key[126] = '5'; key[242] = '_'; key[17] = 'X';
		key[202] = 'q'; key[150] = '0'; key[84] = 'K'; key[96] = ']';
		key[146] = 'r'; key[122] = 'X'; key[55] = 'O'; key[4] = 'f';
		key[164] = '^'; key[134] = 'c'; key[161] = 'A';
		key.swap(result);
	}

};

//////////////////////////////////////////////////////////////////////////

class OnlineActivation : public wxThread {

public:

	explicit OnlineActivation()
			: wxThread(wxTHREAD_JOINABLE),
			m_result(false) {
		//...//
	}

	virtual ~OnlineActivation() {
		//...//
	}

public:

	bool Activate(const std::string &license, ServiceAdapter &service) {
		BOOST_ASSERT(!m_request.get());
		m_request.reset(
			new TunnelEx::Licensing::OnlineKeyRequest(
				license,
				LicenseState(service)));
		m_result = false;
		Create();
		Run();
		wxProgressDialog progress(
			wxT("Activation..."),
			wxT("Processing online activation, please wait..."),
			100,
			NULL,
		wxPD_APP_MODAL | wxPD_SMOOTH);
		for ( ; IsRunning(); progress.Pulse(), wxMilliSleep(25));
		if (!GetActivationResult()) {
			wxLogError(
				wxT("Unknown error at license activation.")
				wxT(" Please check an Internet connection and Internet Explorer proxy server settings")
				wxT(" or try offline activation."));
			return false;
		}
		AcceptActivation();
		return true;
	}

public:

	bool GetActivationResult() const {
		return m_result;
	}

	void AcceptActivation() {
		m_request->Accept();
		m_request.reset();
	}

public:

	virtual ExitCode Entry() {
		static time_t lastOperationTime = 0;
		if (lastOperationTime > 0) {
			const time_t timeFromLastOperation = time(0) - lastOperationTime;
			time_t sleepTime = 0;
			if (timeFromLastOperation < 10) {
				sleepTime = 120;
			} else if (timeFromLastOperation < 30) {
				sleepTime = 30;
			} else if (timeFromLastOperation < 60) {
				sleepTime = 20;
			}
			if (sleepTime > 0) {
				wxSleep(sleepTime);
			}
		}
		m_request->Send();
		m_result = m_request->TestKey<TunnelEx::Licensing::InfoDlgLicense>();
		lastOperationTime = time(0);
		return 0;
	}

private:

	std::auto_ptr<TunnelEx::Licensing::OnlineKeyRequest> m_request;
	bool m_result;

};

///////////////////////////////////////////////////////////////////////////

namespace TunnelEx { namespace Licensing {

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait, bool isTestMode>
	struct LocalStoragePolicy {

		inline static std::string GetLicenseKey(const boost::any &clientParam) {
			return boost::any_cast<LicenseState>(clientParam)
				.service
				->GetLicenseKey();
		}

		inline static std::string GetLocalAsymmetricPrivateKey(
					const boost::any &clientParam) {
			return boost::any_cast<LicenseState>(clientParam)
				.service
				->GetLicenseKeyLocalAsymmetricPrivateKey();
		}

		inline static void StoreLicenseKey(
					const std::string &licenseKey,
					const std::string &privateKey,
					const boost::any &clientParam) {
			boost::any_cast<LicenseState>(clientParam)
				.service
				->SetLicenseKey(
					licenseKey,
					privateKey);
		}

		inline static bool IsLicenseKeyChanged(const boost::any &clientParam) {
			const LicenseState state = boost::any_cast<LicenseState>(clientParam);
			return
				!state.licenseKeyModificationTime
				|| *state.licenseKeyModificationTime != state.service->GetLastLicenseKeyModificatiomTime();
		}

		inline static void ResetLicenseKeyUpdateState(const boost::any &clientParam) {
			const LicenseState state = boost::any_cast<LicenseState>(clientParam);
			if (state.licenseKeyModificationTime) {
				*state.licenseKeyModificationTime
					= state.service->GetLastLicenseKeyModificatiomTime();
			}
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait, bool isTestMode>
	struct WorkstationPropertiesLocalPolicy  {
	
		inline static bool Get(
					WorkstationPropertyValues &result,
					const boost::any &clientParam) {
			const LicenseState state = boost::any_cast<LicenseState>(clientParam);
			return state.service->GetProperties(result);
		}

	};

	//////////////////////////////////////////////////////////////////////////

	template<class ClientTrait, bool isTestMode>
	struct RequestGenerationPolicy {

		inline static void Generate(
					const std::string &license,
					std::string &requestResult,
					std::string &privateKeyResult,
					const boost::any &clientParam) {
			boost::any_cast<LicenseState>(clientParam)
				.service
				->GenerateLicenseKeyRequest(
					license,
					requestResult,
					privateKeyResult);
		}

	};

	//////////////////////////////////////////////////////////////////////////

} }

#endif // INCLUDED_FILE__TUNNELEX__LicensePolicies_hpp__0912060624
