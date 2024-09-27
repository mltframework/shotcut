param(
    [switch]$run = $false
)

$deps = @("Qt", "mlt", "ffmpeg", "fftw", "SDL", "Frei0r");

#Region Main
try {
    & {
        $missing = $false;
        :deps for ($i = 0; $i -lt $deps.Count; $i++) {
            if (-not (Test-Path "./src/$($deps[$i])")) {
                $missing = $true;
                break deps;
            }
        }
    
        if($missing) {
            & install.ps1;
        } else {
            # build

            # run if set
            if ($run) {

            }
        }
    
    }
    
}
catch {
    throw $_;
}
#EndRegion
