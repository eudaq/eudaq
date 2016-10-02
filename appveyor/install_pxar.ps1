function main(){
   # This package is necessary for the CMS pixel option
   Write-Host("Pxarcore installation started");
   
   Push-Location -StackName entryPath -Path "C:\" ;
   
   Start-Process "git" -ArgumentList "clone https://github.com/simonspa/pxar.git" -Wait;
   
   Write-Host("Checked out pxarcore from https://github.com/simonspa/pxar.git");
   
   cd pxar;
   
   Start-Process "git" -ArgumentList "checkout testbeam-2016" -Wait;
   
   Write-Host("Switched to branch checkout testbeam-2016");
   
   mkdir build;
   
   cd build;
   
   Write-Host("Switched to build folder");   
   
   #Start-Process "cmake" -ArgumentList ".." -Wait;
   
   cmake .. ;
   
   Write-Host("cmake finished");
   
   make install ;
   
   [Environment]::SetEnvironmentVariable("PXARPATH", "C:\pxar", "User");
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("Pxarcore installation finished");
}

main