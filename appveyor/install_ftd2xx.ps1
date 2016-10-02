function main(){
   # This package is necessary for the CMS pixel option
   Write-Host("FTD2XX installation started");
   
   Push-Location -StackName entryPath -Path "C:\projects\eudaq\extern" ;
   
   If (Test-Path ("${env:APPVEYOR_BUILD_FOLDER}" + "\extern\ftd2xx")) {Write-Host ("Reusing cached ftd2xx " + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\ftd2xx" + " instead of downloading from ftdichip.com")} Else {Write-Host "Downloading ftdi2xx"; download.ps1 -downloadLocation 'http://www.ftdichip.com/Drivers/CDM/CDM v2.12.18 WHQL Certified.zip' -storageLocation 'C:\\ftd2xx.zip'; $zipargument = ("-o" + "${env:APPVEYOR_BUILD_FOLDER}" + "\extern\"); 7z -y x C:\\ftd2xx.zip $zipargument; }   
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("ftd2xx installation finished");
}

main