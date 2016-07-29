# Script to install libusb
# Based on python install script by
# Authors: Olivier Grisel, Jonathan Helmus, Kyle Kastner, and Alex Willmer
# License: CC0 1.0 Universal: http://creativecommons.org/publicdomain/zero/1.0/

$PAR_DOWNLOAD_LOCATION=$args[0]
$PAR_STORAGE_LOCATION=$args[1]

function Download ($url, $filename) {
    $webclient = New-Object System.Net.WebClient

    # Download and retry up to 5 times in case of network transient errors.
    Write-Host "Downloading " $url
    $retry_attempts = 4
    for ($i = 0; $i -lt $retry_attempts; $i++) {
        try {
            $webclient.DownloadFile($url, $filename)
            break
        }
        Catch [Exception]{
	    Write-Host "Download attempt " $i " of " $retry_attempts " failed."
            Start-Sleep 1
        }
    }
    if (Test-Path $filename) {
        Write-Host "File saved at" $filename
    } else {
        # Retry once to get the error message if any at the last try
        $webclient.DownloadFile($url, $filename)
    }
    return $filename
}

Download($PAR_DOWNLOAD_LOCATION, $PAR_STORAGE_LOCATION)
