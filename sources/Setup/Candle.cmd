
candle.exe ^
	-nologo ^
	-pedantic ^
	-trace ^
	-wx ^
	-v ^
	-dProductName="%ProductName%" ^
	-dProductVendor="%TunnelExVendor%" ^
	-dProductDomain="%TunnelExDomain%" ^
	-dProductFullName="%ProductFullName%" ^
	-dProductVersionFull=%TunnelExVersionFull% ^
	-dProductVersionMajorHigh=%TunnelExVersionMajorHigh% ^
	-dProductVersionMajorLow=%TunnelExVersionMajorLow% ^
	-dProductVersionMinorHigh=%TunnelExVersionMinorHigh% ^
	-dProductVersionMinorLow=%TunnelExVersionMinorLow% ^
	-dCompressionLevel=%CompressionLevel% ^
	-dUpgradeCode=%UpgradeCode% ^
	-dSolutionDir=%~s1 ^
	-dBinDir=%~s2 ^
	-dTexFileNameSuffix=%TexFileNameSuffix% ^
	-dTexExternalsFileNameSuffix=%TexExternalsFileNameSuffix% ^
	-dMsFrameworkDir=%~s3 ^
	-dMsFileNameSuffix=%MsFileNameSuffix% ^
	-dMsManifestNameSuffix=%MsManifestNameSuffix% ^
	-out %~s4 ^
	"%~5"
