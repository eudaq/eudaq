function main(){
   # This package is necessary for palpidefs
   Write-Host("Palpidefs driver installation started");
     
   Push-Location -StackName entryPath -Path "C:\projects\eudaq\extern" ;
   
   If (Test-Path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-TestBeamStable_ALPIDE_2016-56948c980cccf12408059628d758d86a39f27454")) {
	Write-Host ("Reusing cached alice-its-alpide-software-master-latest " + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-TestBeamStable_ALPIDE_2016-56948c980cccf12408059628d758d86a39f27454" + " instead of downloading from cernbox")
	} 
	Else {
	Write-Host "Downloading alice-its-alpide-software-master from cernbox"; 
	C:\\projects\eudaq\appveyor\download.ps1 -downloadLocation 'https://cernbox.cern.ch/index.php/s/WpOQOjrUXihXD98/download' -storageLocation 'C:\\alice-its-alpide-software-master-latest.zip';
	$zipargument = ("-o" + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\"); 
	7z -y x C:\\alice-its-alpide-software-master-latest.zip $zipargument;  
	#Rename-Item -path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-TestBeamStable_ALPIDE_2016-56948c980cccf12408059628d758d86a39f27454") -newName ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-latest")
	}
   
   cd alice-its-alpide-software-TestBeamStable_ALPIDE_2016-56948c980cccf12408059628d758d86a39f27454;
   
   cd pALPIDEfs-software
   
   nmake lib
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("Palpidefs driver installation finished");
}

main