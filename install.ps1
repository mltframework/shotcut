
$mltoutfile = "mlt-tarball.tar.gz";
$mlttarballurl = "https://github.com/mltframework/mlt/releases/download/v7.28.0/mlt-7.28.0.tar.gz";
$mltunpackpath = "./src/mlt";

$fftwurl = "https://fftw.org/fftw-3.3.10.tar.gz";
$fftwoutfile = "fftw-tarball.tar.gz";
$fftwunpackpath = "./src/fftw"

$ffmpegurl = "https://ffmpeg.org/releases/ffmpeg-7.0.2.tar.xz";
$ffmpegoutfilexz = "ffmpeg-tarball.tar.xz";
$ffmpegoutfiletar = "ffmpeg-tarball.tar";
$ffmpegunpackpath = "./src/ffmpeg"

$Frei0rurl = "https://github.com/dyne/frei0r/releases/download/v2.3.3/frei0r-v2.3.3_win64.zip";
$Frei0routfile = "Frei0r.zip";
$Frei0runpackpath = "./src/Frei0r"

$SDLurl = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-2.30.7-win32-x64.zip";
$SDLoutfile = "SDL.zip";
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
        tar -C $unpackpath --extract --file="$($outputfile)" --verbose;
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
    Write-Host -ForegroundColor Cyan "[INFO]: checking MLT"
    
    function getmlt {
        if (-not (Test-Path $mltunpackpath)) {
            Write-Host -ForegroundColor Cyan "[INFO]: MLT not found. installing to => $mltunpackpath"
            New-Item -ItemType Directory -Path $mltunpackpath;

            downloadtarball -url $mlttarballurl -outpath $mltoutfile; 
            unpacktarball -outputfile $mltoutfile -unpackpath $mltunpackpath;
            deletetarball -outputfile $mltoutfile;
        }
    }
    #EndRegion
    
    #Region QT
    function getqt {
        Write-Host -ForegroundColor Cyan "[INFO]: checking QT"
        
        if (-not (Test-Path "./src/Qt*")) {
            Write-Host -ForegroundColor Cyan "[INFO]: QT not installed - installing to => ./src/Qt"
            & "$($PSScriptRoot)/Get-Qt.ps1";
        }
    }

    #EndRegion
    
    
    # FFTW
    #Region FFTW
    function getfftw {
        Write-Host -ForegroundColor Cyan "[INFO]: checking FFTW"
        
        if (-not (Test-Path $fftwunpackpath)) {
            New-Item -ItemType Directory -Path $fftwunpackpath;

            Write-Host -ForegroundColor Cyan "[INFO]: FFTW not found... installing to this project to => $fftwunpackpath"
            downloadtarball -url $fftwurl -outpath $fftwoutfile;
            unpacktarball -outputfile $fftwoutfile -unpackpath $fftwunpackpath
            deletetarball $fftwoutfile;
        }
    }
    
    
    #EndRegion
    
    
    # FFmpeg: multimedia format and codec libraries
    #Region FFMPEG
    function getffmpeg {
        Write-Host -ForegroundColor Cyan "[INFO]: checking FFmpeg: multimedia format and codec libraries"
        if (-not (Test-Path $ffmpegunpackpath)) {
            New-Item -ItemType Directory -Path $ffmpegunpackpath;

            Write-Host -ForegroundColor Cyan "[INFO]: FFMPEG not found, installing to this project to => $($ffmpegunpackpath)..."
            
            downloadtarball -url $ffmpegurl -outpath $ffmpegoutfilexz;
            
            xz.exe -d -v $ffmpegoutfilexz;
            unpacktarball -outputfile $ffmpegoutfiletar -unpackpath $ffmpegunpackpath;
            
            Remove-Item $ffmpegoutfiletar -Force -Recurse -Verbose;
        
        }
    }
    
    #EndRegion
    
    
    # Frei0r: video plugins
    #Region Frei0r
    function getfrei0r {
        Write-Host -ForegroundColor Cyan "[INFO]: checking Frei0r: video plugins"
        if (-not (Test-Path $Frei0runpackpath)) {

            New-Item -ItemType Directory -Path $Frei0runpackpath;

            Write-Host -ForegroundColor Cyan "[INFO]: Frei0r not found, installing to this project to => $Frei0runpackpath..."

            Invoke-WebRequest $Frei0rurl -OutFile $Frei0routfile;
            
            Expand-Archive -Path $Frei0routfile -DestinationPath $Frei0runpackpath;
            
            Remove-Item $Frei0routfile -Force -Recurse -Verbose;
        }
    }

    #EndRegion
    
    
    # SDL: cross-platform audio playback
    #Region SDL
    function getsdl {
        Write-Host -ForegroundColor Cyan "[INFO]: checking SDL: cross-platform audio playback..."
        if (-not (Test-Path $SDLunpackpath)) {
            New-Item -ItemType Directory -Path $SDLunpackpath;
            
            Write-Host -ForegroundColor Cyan "[INFO]: SDL not found, installing to this project to => $SDLunpackpath..."
            
            Invoke-WebRequest $SDLurl -OutFile $SDLoutfile;
            
            Expand-Archive -Path $SDLoutfile -DestinationPath $SDLunpackpath;
            
            Remove-Item $SDLoutfile -Force -Recurse -Verbose;
        }
    }

    #EndRegion
}
catch {
    throw $_;
}

getmlt;

# this wont work - just cloned from git anyways in C:\qt6

# best I can do for qt6 at the moment is something like 
# local installation of qt6
# New-Item -Itemtype Junction -Path ./src/Qt -Target "$home\projects\qt6"
# getqt;
getfftw;
getffmpeg;
getfrei0r;
getsdl;