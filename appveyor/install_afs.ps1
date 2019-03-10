function main () {

    $clnt = new-object System.Net.WebClient; 

    Write-Host("Downloading file for afs installation.");	
    $clnt.DownloadFile("https://www.auristor.com/downloads/openafs/windows/x64/yfs-openafs-en_US-AMD64-1_7_3301.msi","C:\\yfs-openafs-en_US-AMD64-1_7_3301.msi");
    Start-Process "C:\\yfs-openafs-en_US-AMD64-1_7_3301.msi" -ArgumentList "/quiet /qn /norestart /log install.log ADDLOCAL=feaNetIDMgrPlugin,feaClient,feaOpenAFS AcceptLicense=1" -Wait;
    
}

main