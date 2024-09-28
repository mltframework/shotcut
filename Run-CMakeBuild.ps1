param(
    [string]
    $vcpkg_dir = "C:\Program Files\vcpkg\",

    [string]
    $qt_path = "$($PSScriptRoot)/../Qt"
)

if (!(Test-Path "$vcpkg_dir")) {
    throw "could not find the vcpkg directory please update this value with your path to vcpkg directory";
}

if (!(Test-Path "$qt_path")) {
    throw "Qt dependency is not installed yet, for this script to work install Qt to this project's root directory or change the path to your Qt installation";
}

$qt5_widgets_cmake_path = "$($qt_path)/5.15.2/msvc2019_64/lib/cmake/Qt5Widgets/";
$qt5_network_cmake_path = "$($qt_path)/5.15.2/msvc2019_64/lib/cmake/Qt5Network/";
$qt5_printsupport_cmake_path = "$($qt_path)/5.15.2/msvc2019_64/lib/cmake/Qt5PrintSupport/";
$qt5_linguisttools_cmake_path = "$($qt_path)/5.15.2/msvc2019_64/lib/cmake/Qt5LinguistTools";

<#
    https://cmake.org/cmake/help/latest/envvar/CMAKE_PREFIX_PATH.html#envvar:CMAKE_PREFIX_PATH
#>
$cmake_prefix_path = $(
    @(
        $qt5_widgets_cmake_path, 
        $qt5_network_cmake_path, 
        $qt5_printsupport_cmake_path, 
        $qt5_linguisttools_cmake_path
        # probably a unix system as per cmake documentation are colon separated
    ) -join $(if ($env:OS -match "Windows") {";"} else {":"})
);


cmake `
-A x64 `
-B ./build `
-DPTE_ENABLE_PCH=1 `
-DCMAKE_TOOLCHAIN_FILE="$($vcpkg_dir)/scripts/buildsystems/vcpkg.cmake" `
-DCMAKE_PREFIX_PATH="$($cmake_prefix_path)"