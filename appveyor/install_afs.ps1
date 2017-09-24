function main () {
    Write-Host("AFS installation started");

    $clnt = new-object System.Net.WebClient; 

    try{
       $clnt.DownloadFile("https://www.secure-endpoints.com/binaries/heimdal/Heimdal-AMD64-full-7-4-0-40.msi","C:\\Heimdal-AMD64-full-7-4-0-40.msi");
    }
    catch {}
    
    $clnt.DownloadFile("http://dl.openafs.org/dl/openafs/1.7.31/winxp/openafs-en_US-64bit-1-7-3100.msi","C:\\openafs-en_US-64bit-1-7-3100.msi");
	$clnt.DownloadFile("http://dl.openafs.org/dl/openafs/1.7.31/winxp/openafs-32bit-tools-en_US-1-7-3100.msi","C:\\openafs-32bit-tools-en_US-1-7-3100.msi");
   
    If( Test-Path("C:\\Heimdal-AMD64-full-7-4-0-40.msi") ) {
	Start-Process "C:\\Heimdal-AMD64-full-7-4-0-40.msi" -ArgumentList "/quiet /qn /norestart /L*V install_heimdal.log ADDLOCAL=ALL" -Wait;
	Start-Process msiexec -ArgumentList "/i C:\\openafs-en_US-64bit-1-7-3100.msi /quiet /qn /norestart /L*V install_openafs.log ADDLOCAL=ALL InstallMode=Typical AFSCELLNAME=desy.de IAgree=yes" -Wait -verb runas;
	Start-Process "C:\\openafs-32bit-tools-en_US-1-7-3100.msi" -ArgumentList "/quiet /qn /norestart /L*V install_openafs_tools.log ADDLOCAL=ALL" -Wait;
    }  Else {
	Write-Host("It seems that not all files for afs installation were downloaded properly - trying alternative.");	$clnt.DownloadFile("https://www.auristor.com/downloads/openafs/windows/x64/yfs-openafs-en_US-AMD64-1_7_3301.msi","C:\\yfs-openafs-en_US-AMD64-1_7_3301.msi");
	Start-Process "C:\\yfs-openafs-en_US-AMD64-1_7_3301.msi" -ArgumentList "/quiet /qn /norestart /log install.log ADDLOCAL=feaNetIDMgrPlugin,feaClient,feaOpenAFS AcceptLicense=1" -Wait;
    }
    Write-Host("AFS installation finished");    
}

main