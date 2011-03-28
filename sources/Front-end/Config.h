/**************************************************************************
 *   Created: 2008/01/03 4:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.com
 * Copyright: 2007 - 2008 Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *       $Id: Config.h 958 2010-06-21 15:53:18Z palchukovsky $
 **************************************************************************/

#ifndef INCLUDED_FILE__TUNNELEX__Config_h__0801030455
#define INCLUDED_FILE__TUNNELEX__Config_h__0801030455

class Config : public wxFileConfig {

public:

	Config();
	virtual ~Config();


public:

#	ifdef _WINDOWS
		static const wxString& SetConfFilePath();
#	endif // _WINDOWS


private:

#	ifdef _WINDOWS
		static wxString m_confFilePathCache;
#	endif // _WINDOWS
	
	DECLARE_NO_COPY_CLASS(Config)

};

#endif // INCLUDED_FILE__TUNNELEX__Config_h__0801030455