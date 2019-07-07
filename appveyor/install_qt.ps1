function main () {

    $clnt = new-object System.Net.WebClient; 

    Write-Host("Downloading file for Qt installation.");	
    # $clnt.DownloadFile("http://download.qt.io/official_releases/qt/5.13/5.13.0/qt-opensource-windows-x86-5.13.0.exe","qt-opensource-windows-x86-5.13.0.exe");
    $clnt.DownloadFile("http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe","qt-unified-windows-x86-online.exe");
    Start-Process "C:\\qt-unified-windows-x86-online.exe" -ArgumentList "/quiet /qn /norestart /log install.log ADDLOCAL=feaNetIDMgrPlugin,feaClient,feaOpenAFS AcceptLicense=1" -Wait;
    
}

main