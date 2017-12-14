function main(){
   # This package is necessary for the CMS pixel option
   Write-Host("Pxarcore installation started");
   
   Push-Location -StackName entryPath -Path "C:\projects\eudaq\extern" ;
   
   Start-Process "git" -ArgumentList "clone https://github.com/simonspa/pxar.git" -Wait;
   
   Write-Host("Checked out pxarcore from https://github.com/simonspa/pxar.git");
   
   cd pxar;
   
   Start-Process "git" -ArgumentList "checkout testbeam-2016" -Wait;
   
   Write-Host("Switched to branch checkout testbeam-2016");
   
   Write-Host("Installing pxarcore dependency: ftd2xx");
   
    . "C:\\projects\eudaq\appveyor\install_ftd2xx.ps1";   
   
   mkdir build;
   
   cd build;
   
   Write-Host("Switched to build folder");   
   
   Write-Host("Building pxar with pxarui=OFF as not necessary for this purpose and there is a problem with timespec redefinition (probably pthreads probably)");
   
   cmake -DBUILD_pxarui=OFF .. ;
   
   Write-Host("cmake finished");
   
   MSBUILD.exe INSTALL.vcxproj
   
   #Variables put into the User context are not visible in this process, process variables are only temporary but fortunately we are running here in one process
   [Environment]::SetEnvironmentVariable("PXARPATH", "C:\projects\eudaq\extern\pxar", "User");
   [Environment]::SetEnvironmentVariable("PXARPATH", "C:\projects\eudaq\extern\pxar", "Process");
   
   Pop-Location -StackName entryPath -PassThru ;
   
   Write-Host("Pxarcore installation finished");
}

main