/**************************************************************************
 *   Created: 2008/02/07 0:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

//////////////////////////////////////////////////////////////////////////
// Customization:

var vendorName	= "Tunnelex Project";
var domain		= "tunnelex.net";
var licenseServiceSubdomain = "licensing";
var productName	= "TunnelEx";
var copyright	= "Copyright 2012 (C) " + vendorName + ". All rights reserved.";

var coreFileName			= "TunnelEx";
var serviceFileName			= "TunnelEx";
var controlCenterFileName	= "TunnelExControlCenter";
var legacyFileName			= "TexLegacy";
var modulePipeFileName		= "TexModPipe";
var moduleInetFileName		= "TexModInet";
var moduleFtpFileName		= "TexModFtp";
var moduleSerialFileName	= "TexModSerial";
var modulePathfinderFileName = "TexModPathfinder";
var moduleUpnpFileName		= "TexModUpnp";

var soapServicePort			= "2901";

//////////////////////////////////////////////////////////////////////////

var shell = new ActiveXObject("WScript.Shell");
var fileSystem = new ActiveXObject("Scripting.FileSystemObject");
var inputFile = "";
var outputDir = "";
var versionGenUniqueId = Math.ceil(Math.random() * 1000000);

//////////////////////////////////////////////////////////////////////////

function GetArgs() {
	for (i = 0; i < WScript.Arguments.length; ++i) {
		var arg = WScript.Arguments(i);
		var opt = arg.substring(0, arg.indexOf("="));
		if (opt.length == 0) {
			return false;
		}
		if (opt == "InputFile") {
			inputFile = arg.substring(opt.length + 1, arg.length);
		} else if (opt == "OutputDir") {
			outputDir = arg.substring(opt.length + 1, arg.length);
		} else {
			return false;
		}
	}
	return inputFile != "" && outputDir != "";
}

function IsExistsInFile(filePath, line) {
	try {
		var f = fileSystem.OpenTextFile(filePath, 1, false);
		while (f.AtEndOfStream != true) {
			if (f.ReadLine() == line) {
				return true;
			}
		}
	} catch (e) {
		//...//
	}
	return false;
}

function GetVersion() {
	var f = fileSystem.OpenTextFile(inputFile, 1, false);
	if (f.AtEndOfStream != true) {
		var expression = /^\s*(\d+)\.(\d+)\.(\d+)\.(\d+)\s*$/;
		var match = expression.exec(f.ReadLine());
		if (match) {
			var version = new Object;
			version.majorHigh	= match[1];
			version.majorLow	= match[2];
			version.minorHigh	= match[3];
			version.minorLow	= match[4];
			return version;
		} else {
			shell.Popup("Version compiling: could not parse version file.");
		}
	} else {
		shell.Popup("Version compiling: could not find version file.");
	}
	return null;
}

function CreateVersionCppHeaderFile() {
	
	var version = GetVersion();
	if (version == null) {
		return;
	}
	
	var versionMajorHighLine	= "#\tdefine TUNNELEX_VERSION_MAJOR_HIGH\t" + version.majorHigh;
	var versionMajorLowLine		= "#\tdefine TUNNELEX_VERSION_MAJOR_LOW\t" + version.majorLow;
	var versionMinorHighLine	= "#\tdefine TUNNELEX_VERSION_MINOR_HIGH\t" + version.minorHigh;
	var versionMinorLowLine		= "#\tdefine TUNNELEX_VERSION_MINOR_LOW\t" + version.minorLow;
	var versionGenUniqueIdLine	= "#\tdefine TUNNELEX_VERSION_GEN_UNIQUE_ID " + versionGenUniqueId + "" ;
	
	var vendorLine				= "#\tdefine TUNNELEX_VENDOR\t\t\"" + vendorName + "\"";
	var vendorLineW				= "#\tdefine TUNNELEX_VENDOR_W\tL\"" + vendorName + "\"";
	
	var domainLine				= "#\tdefine TUNNELEX_DOMAIN\t\t\"" + domain + "\"";
	var domainLineW				= "#\tdefine TUNNELEX_DOMAIN_W\tL\"" + domain + "\"";
	
	var licenseServiceSubdomainLine = "#\tdefine TUNNELEX_LICENSE_SERVICE_SUBDOMAIN\t\"" + licenseServiceSubdomain + "\"";
	var licenseServiceSubdomainLineW = "#\tdefine TUNNELEX_LICENSE_SERVICE_SUBDOMAIN_W\tL\"" + licenseServiceSubdomain + "\"";
	
	var nameLine				= "#\tdefine TUNNELEX_NAME\t\"" + productName + "\"";
	var nameLineW				= "#\tdefine TUNNELEX_NAME_W\tL\"" + productName + "\"";
	
	var copyrightLine			= "#\tdefine TUNNELEX_COPYRIGHT\t\"" + copyright + "\"";
	var copyrightLineW			= "#\tdefine TUNNELEX_COPYRIGHT_W\tL\"" + copyright + "\"";

	var coreFileNameLine		= "#\tdefine TUNNELEX_CORE_FILE_NAME\t\t\t\t\"" + coreFileName + "\"";
	var serviceFileNameLine		= "#\tdefine TUNNELEX_SERVICE_FILE_NAME\t\t\t\"" + serviceFileName + "\"";
	var controlCenterFileNameLine = "#\tdefine TUNNELEX_CONTROL_CENTER_FILE_NAME\t\"" + controlCenterFileName + "\"";
	var legacyFileNameLine		= "#\tdefine TUNNELEX_LEGACY_FILE_NAME\t\t\t\"" + legacyFileName + "\"";
	var modulePipeFileNameLine	= "#\tdefine TUNNELEX_MODULE_PIPE_FILE_NAME\t\t\"" + modulePipeFileName + "\"";
	var moduleInetFileNameLine	= "#\tdefine TUNNELEX_MODULE_INET_FILE_NAME\t\t\"" + moduleInetFileName + "\"";
    var moduleFtpFileNameLine = "#\tdefine TUNNELEX_MODULE_FTP_FILE_NAME\t\t\"" + moduleFtpFileName + "\"";
	var moduleSerialFileNameLine = "#\tdefine TUNNELEX_MODULE_SERIAL_FILE_NAME\t\t\"" + moduleSerialFileName + "\"";
	var modulePathfinderFileNameLine = "#\tdefine TUNNELEX_MODULE_PATHFINDER_FILE_NAME\t\"" + modulePathfinderFileName + "\"";
	var moduleUpnpFileNameLine	= "#\tdefine TUNNELEX_MODULE_UPNP_FILE_NAME\t\t\"" + moduleUpnpFileName + "\"";
	
	var soapServicePortLine	= "#\tdefine TUNNELEX_SOAP_SERVICE_PORT " + soapServicePort + "";

	var fullFileName = outputDir + "Version.h";
	if (	!IsExistsInFile(fullFileName, versionMajorHighLine)
			|| !IsExistsInFile(fullFileName, versionMajorLowLine)
			|| !IsExistsInFile(fullFileName, versionMinorHighLine)
			|| !IsExistsInFile(fullFileName, versionMinorLowLine)
			// || !IsExistsInFile(fullFileName, versionGenUniqueIdLine) - not used for expired file check
			|| !IsExistsInFile(fullFileName, vendorLine)
			|| !IsExistsInFile(fullFileName, domainLine)
			|| !IsExistsInFile(fullFileName, licenseServiceSubdomainLine)
			|| !IsExistsInFile(fullFileName, nameLine)
			|| !IsExistsInFile(fullFileName, copyrightLine)
			|| !IsExistsInFile(fullFileName, coreFileNameLine)
			|| !IsExistsInFile(fullFileName, serviceFileNameLine)
			|| !IsExistsInFile(fullFileName, controlCenterFileNameLine)
			|| !IsExistsInFile(fullFileName, legacyFileNameLine)
			|| !IsExistsInFile(fullFileName, modulePipeFileNameLine)
			|| !IsExistsInFile(fullFileName, moduleInetFileNameLine)
            || !IsExistsInFile(fullFileName, moduleFtpFileNameLine)
			|| !IsExistsInFile(fullFileName, moduleSerialFileNameLine)
			|| !IsExistsInFile(fullFileName, modulePathfinderFileNameLine)
			|| !IsExistsInFile(fullFileName, moduleUpnpFileNameLine)
			|| !IsExistsInFile(fullFileName, soapServicePortLine)) {
	
		var define = "INCLUDED_FILE__TUNNELEX__Version_h__0802010512";			
		var f = fileSystem.CreateTextFile(fullFileName, true);

		f.WriteLine("");
		f.WriteLine("#ifndef " + define);
		f.WriteLine("#define " + define);
		
		f.WriteLine("");
		
		f.WriteLine(versionMajorHighLine);
		f.WriteLine(versionMajorLowLine);
		f.WriteLine(versionMinorHighLine);
		f.WriteLine(versionMinorLowLine);
		f.WriteLine(versionGenUniqueIdLine);
		
		f.WriteLine("");
		
		f.WriteLine(vendorLine);
		f.WriteLine(vendorLineW);
		f.WriteLine("");
		f.WriteLine(domainLine);
		f.WriteLine(domainLineW);
		f.WriteLine("");
		f.WriteLine(licenseServiceSubdomainLine);
		f.WriteLine(licenseServiceSubdomainLineW);
		f.WriteLine("");
		f.WriteLine(nameLine);
		f.WriteLine(nameLineW);
		f.WriteLine("");
		f.WriteLine(copyrightLine);
		f.WriteLine(copyrightLineW);
		
		f.WriteLine("");
		
		f.WriteLine(coreFileNameLine);
		f.WriteLine(serviceFileNameLine);
		f.WriteLine(controlCenterFileNameLine);
		f.WriteLine(legacyFileNameLine);
		f.WriteLine(modulePipeFileNameLine);
		f.WriteLine(moduleInetFileNameLine);
        f.WriteLine(moduleFtpFileNameLine);
		f.WriteLine(moduleSerialFileNameLine);
		f.WriteLine(modulePathfinderFileNameLine);
		f.WriteLine(moduleUpnpFileNameLine);
		
		f.WriteLine("");
		
		f.WriteLine(soapServicePortLine);
		
		f.WriteLine("");
		
		f.WriteLine("#endif // " + define);
	
	}

}

function CreateVersionCmdFile() {
	
	var version = GetVersion();
	if (version == null) {
		return;
	}
	
	var f = fileSystem.CreateTextFile(outputDir + "SetVersion.cmd", true);
	
	f.WriteLine("");
	
	f.WriteLine("set TunnelExVersionMajorHigh=" + version.majorHigh);
	f.WriteLine("set TunnelExVersionMajorLow=" + version.majorLow);
	f.WriteLine("set TunnelExVersionMinorHigh=" + version.minorHigh);
	f.WriteLine("set TunnelExVersionMinorLow=" + version.minorLow);
	f.WriteLine("set TunnelExVersionGenUniqueId=" + versionGenUniqueId);
	
	f.WriteLine("");
	
	f.WriteLine("set TunnelExVersion=%TunnelExVersionMajorHigh%.%TunnelExVersionMajorLow%.%TunnelExVersionMinorHigh%");
	f.WriteLine("set TunnelExVersionFull=%TunnelExVersionMajorHigh%.%TunnelExVersionMajorLow%.%TunnelExVersionMinorHigh%.%TunnelExVersionMinorLow%");
	
	f.WriteLine("");
	
	f.WriteLine("set TunnelExVendor=" + vendorName);
	f.WriteLine("set TunnelExDomain=" + domain);
	f.WriteLine("set TunnelExName=" + productName);

}

function main() {
	if (!GetArgs()) {
		shell.Popup("Version compiling: Wrong arguments!");
		return;
	}
	CreateVersionCppHeaderFile();
	CreateVersionCmdFile();
}

//////////////////////////////////////////////////////////////////////////

try {
	main();
} catch (e) {
	shell.Popup("Version compiling: " + e.message);
}

//////////////////////////////////////////////////////////////////////////
