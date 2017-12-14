# Script to download file on several trials
# Based on python install script by
# Authors: Olivier Grisel, Jonathan Helmus, Kyle Kastner, and Alex Willmer
# License: CC0 1.0 Universal: http://creativecommons.org/publicdomain/zero/1.0/
[CmdletBinding()]
Param(
  [Parameter(Mandatory=$True,Position=1)]
   [string]$downloadLocation,
	
   [Parameter(Mandatory=$True)]
   [string]$storageLocation
)

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

Download $downloadLocation $storageLocation
