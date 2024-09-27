
$mltoutfile = "mlt-tarball";
$mlttarballurl = "https://github.com/mltframework/mlt/releases/download/v7.28.0/mlt-7.28.0.tar.gz";
$mltunpackpath = "./src/mlt";

$fftwurl = "https://fftw.org/fftw-3.3.10.tar.gz";
$fftwoutfile = "fftw-tarball";
$fftwunpackpath = "./src/fftw"

$ffmpegurl = "https://ffmpeg.org/releases/ffmpeg-7.0.2.tar.xz";
$ffmpegoutfile = "ffmpeg-tarball";
$ffmpegunpackpath = "./src/ffmpeg"

$Frei0rurl = "https://github.com/dyne/frei0r/releases/download/v2.3.3/frei0r-v2.3.3_win64.zip";
$Frei0routfile = "Frei0r-zip";
$Frei0runpackpath = "./src/Frei0r"

$SDLurl = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-2.30.7-win32-x64.zip";
$SDLoutfile = "SDL-zip";
$SDLunpackpath = "./src/SDL"


#Region Helpers
function downloadtarball {
    [OutputType([Void])]
    param(
        [Parameter(Mandatory)]
        [string]$url,
        [Parameter(Mandatory)]
        [string]$outpath
    )
    & {
        Invoke-WebRequest `
        $url `
        -OutFile $outpath;
    }
}

function unpacktarball {
    [OutputType([Void])]
    param(
        [Parameter(Mandatory)]
        [string]$outputfile,
        [Parameter(Mandatory)]
        [string]$unpackpath
    )   
    & {
        Get-ChildItem -Path $PSScriptRoot -Filter $outputfile | `
        Foreach-Object {
            tar -xvzf $_.FullName -C $unpackpath;
        }
    }
    
}
function deletetarball {
    [OutputType([Void])]
    param(
        [string]$outputfile
    )
    
    & {
        Remove-Item $outputfile -Force -Recurse -Verbose;
    }
}
#EndRegion




try {
    
    
    #Region MLT
    if (-not (Test-Path $mltunpackpath)) {
        downloadtarball -url $mlttarballurl -outpath $mltoutfile; 
        unpacktarball -outputfile $mltoutfile -unpackpath $mltunpackpath;
        deletetarball -outputfile $mltoutfile;
    }
    #EndRegion
    
    #Region QT
    Write-Host -ForegroundColor Cyan "[INFO]: checking QT"
    
    if (-not (Test-Path "./src/Qt*")) {
        Write-Host -ForegroundColor Cyan "[INFO]: QT not installed - installing to => ./src/Qt"
        & "$($PSScriptRoot)/Get-Qt.ps1";
    }
    #EndRegion
    
    
    # FFTW
    #Region FFTW
    
    Write-Host -ForegroundColor Cyan "[INFO]: checking FFTW"
    
    if (-not (Test-Path $fftwunpackpath)) {
        Write-Host -ForegroundColor Cyan "[INFO]: FFTW not found... installing to this project to => $fftwunpackpath"
        downloadtarball -url $fftwurl -outpath $fftwoutfile;
        unpacktarball -outputfile $fftwoutfile -unpackpath $fftwunpackpath;
        deletetarball $fftwoutfile;
    }
    
    #EndRegion
    
    
    # FFmpeg: multimedia format and codec libraries
    Write-Host -ForegroundColor Cyan "[INFO]: checking FFmpeg: multimedia format and codec libraries"
    #Region FFMPEG
    if (-not (Test-Path $ffmpegunpackpath)) {
        Write-Host -ForegroundColor Cyan "[INFO]: FFMPEG not found, installing to this project to => $($ffmpegunpackpath)..."
        downloadtarball -url $ffmpegurl -outpath $ffmpegoutfile;
        unpacktarball -outputfile $ffmpegoutfile -unpackpath $ffmpegunpackpath;
        deletetarball $ffmpegoutfile;
    
    }
    #EndRegion
    
    
    # Frei0r: video plugins
    Write-Host -ForegroundColor Cyan "[INFO]: checking Frei0r: video plugins"
    #Region Frei0r
    if (-not (Test-Path $Frei0runpackpath)) {
        Write-Host -ForegroundColor Cyan "[INFO]: Frei0r not found, installing to this project to => $Frei0runpackpath..."
        Invoke-WebRequest $Frei0rurl -OutFile $Frei0routfile;
        Expand-Archive -Path $Frei0routfile -DestinationPath $Frei0runpackpath;
        Remove-Item $Frei0routfile -Force -Recurse -Verbose;
    }
    #EndRegion
    
    
    # SDL: cross-platform audio playback
    #Region SDL
    Write-Host -ForegroundColor Cyan "[INFO]: checking SDL: cross-platform audio playback..."
    if (-not (Test-Path $SDLunpackpath)) {
        Write-Host -ForegroundColor Cyan "[INFO]: SDL not found, installing to this project to => $SDLunpackpath..."
        # not tarball zip file for win64
        Invoke-WebRequest $SDLurl -OutFile $SDLoutfile;
        Expand-Archive -Path $SDLoutfile -DestinationPath $SDLunpackpath;
        Remove-Item $SDLoutfile -Force -Recurse -Verbose;
    }
    #EndRegion
}
catch {
    throw $_;
}
