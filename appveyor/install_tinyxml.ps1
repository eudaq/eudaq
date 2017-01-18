function main(){
   # This package is necessary for palpidefs
   Write-Host("Tinyxml installation started");
   
   Push-Location -StackName entryPath -Path "C:\projects\eudaq\extern" ;
   
   If (Test-Path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\tinyxml")) {
	Write-Host ("Reusing cached tinyxml " + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\tinyxml" + " instead of downloading from sourceforge")
	} 
	Else {
	Write-Host "Downloading tinyxml from sourceforge"; 
	C:\\projects\eudaq\appveyor\download.ps1 -downloadLocation 'http://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip' -storageLocation 'C:\\tinyxml_2_6_2.zip'; 
	$zipargument = ("-o" + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\"); 
	7z -y x C:\\tinyxml_2_6_2.zip $zipargument;  
	}
   
   cd tinyxml
   
   MSBUILD.exe tinyxml_lib.vcxproj
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("Tinyxml installation finished");
}

main