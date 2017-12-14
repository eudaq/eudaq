function main () {

    $clnt = new-object System.Net.WebClient; 

    try{
       $clnt.DownloadFile("https://www.secure-endpoints.com/binaries/heimdal/Heimdal-AMD64-full-1-6-2-0.msi","C:\\Heimdal-AMD64-full-1-6-2-0.msi");
    }
    catch {}
    
    $clnt.DownloadFile("http://dl.openafs.org/dl/openafs/1.7.31/winxp/openafs-en_US-64bit-1-7-3100.msi","C:\\openafs-en_US-64bit-1-7-3100.msi");
	$clnt.DownloadFile("http://dl.openafs.org/dl/openafs/1.7.31/winxp/openafs-32bit-tools-en_US-1-7-3100.msi","C:\\openafs-32bit-tools-en_US-1-7-3100.msi");
   
    If( Test-Path("C:\\Heimdal-AMD64-full-1-6-2-0.msi") ) {
	Start-Process "C:\\Heimdal-AMD64-full-1-6-2-0.msi" -ArgumentList "/quiet /qn /norestart /log install.log ADDLOCAL=ALL" -Wait;
	Start-Process "C:\\openafs-en_US-64bit-1-7-3100.msi" -ArgumentList "/quiet /qn /norestart /log install.log ADDLOCAL=ALL" -Wait;
	Start-Process "C:\\openafs-32bit-tools-en_US-1-7-3100.msi" -ArgumentList "/quiet /qn /norestart /log install.log ADDLOCAL=ALL" -Wait;
    }  Else {
	Write-Host("It seems that not all files for afs installation were downloaded properly - trying alternative.");	$clnt.DownloadFile("https://www.auristor.com/downloads/openafs/windows/x64/yfs-openafs-en_US-AMD64-1_7_3301.msi","C:\\yfs-openafs-en_US-AMD64-1_7_3301.msi");
	Start-Process "C:\\yfs-openafs-en_US-AMD64-1_7_3301.msi" -ArgumentList "/quiet /qn /norestart /log install.log ADDLOCAL=feaNetIDMgrPlugin,feaClient,feaOpenAFS AcceptLicense=1" -Wait;
    }
    
}

main