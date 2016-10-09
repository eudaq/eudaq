function main(){
   # This package is necessary for palpidefs
   Write-Host("Palpidefs driver installation started");
     
   Push-Location -StackName entryPath -Path "C:\projects\eudaq\extern" ;
   
   If (Test-Path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b")) {
	Write-Host ("Reusing cached alice-its-alpide-software-master-latest " + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b" + " instead of downloading from cernbox")
	} 
	Else {
	Write-Host "Downloading alice-its-alpide-software-master from cernbox"; 
	C:\\projects\eudaq\appveyor\download.ps1 -downloadLocation 'https://cernbox.cern.ch/index.php/s/QIRPTV84XziyQ3q/download' -storageLocation 'C:\\alice-its-alpide-software-master-latest.zip'; 
	$zipargument = ("-o" + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\"); 
	7z -y x C:\\alice-its-alpide-software-master-latest.zip $zipargument;  
	#Rename-Item -path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b") -newName ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\alice-its-alpide-software-master-latest")
	}
   
   cd alice-its-alpide-software-master-3189f00d7515733d46a61a5ab2606e436df4955b;
   
   cd pALPIDEfs-software
   
   nmake lib
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("Palpidefs driver installation finished");
}

main