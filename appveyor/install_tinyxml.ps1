function main(){
   # This package is necessary for palpidefs
   Write-Host("Tinyxml installation started");
   
   Push-Location -StackName entryPath -Path "C:\projects\eudaq\extern" ;
   
   If (Test-Path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b")) {
	Write-Host ("Reusing cached alice-its-alpide-software-master-latest " + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b" + " instead of downloading from cernbox")
	} 
	Else {
	Write-Host "Downloading tinyxml from sourceforge"; 
	C:\\projects\eudaq\appveyor\download.ps1 -downloadLocation 'http://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip' -storageLocation 'C:\\tinyxml_2_6_2.zip'; 
	$zipargument = ("-o" + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\"); 
	7z -y x C:\\tinyxml_2_6_2.zip $zipargument;  
	#Rename-Item -path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\tinyxml") -newName ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-latest")
	}
   
   cd tinyxml
   
   MSBUILD.exe tinyxml_lib.vcxproj
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("Tinyxml installation finished");
}

main