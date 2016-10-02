function main(){
   # This package is necessary for the CMS pixel option
   Write-Host("Pxarcore installation started");
   
   Push-Location -StackName entryPath -Path C: ;
   
   Start-Process "git" -ArgumentList "clone https://github.com/simonspa/pxar.git" -Wait;
   
   cd pxar;
   
   Start-Process "git" -ArgumentList "checkout testbeam-2016" -Wait;
   
   mkdir build;
   
   cd build;
   
   Start-Process "cmake" -ArgumentList ".." -Wait;
   
   Start-Process "make" -ArgumentList "install" -Wait;
   
   [Environment]::SetEnvironmentVariable("PXARPATH", "C:\pxar", "User");
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("Pxarcore installation finished");
}

main