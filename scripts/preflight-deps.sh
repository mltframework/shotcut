#!/bin/bash
# Check host build dependencies for scripts/build-shotcut.sh on Debian/Ubuntu.
# Exit 0 = all present, 1 = missing packages, 2 = unsupported OS.
# Never installs packages; only reports.

set -uo pipefail

OS_FAMILY=""
MISSING_PACKAGES=()

add_missing() {
    local label="$1"
    local pkg="$2"
    echo "missing: ${label} (provided by apt package: ${pkg})"
    MISSING_PACKAGES+=("$pkg")
}

have_cmd() {
    command -v "$1" >/dev/null 2>&1
}

check_command() {
    local cmd="$1"
    local pkg="$2"
    if ! have_cmd "$cmd"; then
        add_missing "$cmd" "$pkg"
        return 1
    fi
    return 0
}

check_perl_module() {
    local module="$1"
    local pkg="$2"
    if ! have_cmd perl; then
        # perl itself is reported by check_command; skip module probe.
        return 1
    fi
    if ! perl -M"$module" -e 1 >/dev/null 2>&1; then
        add_missing "perl module $module" "$pkg"
        return 1
    fi
    return 0
}

check_pkg_config() {
    local pc="$1"
    local pkg="$2"
    if ! have_cmd pkg-config && ! have_cmd pkgconf; then
        # pkg-config itself is reported separately; skip PC checks.
        return 1
    fi
    local pc_bin=pkg-config
    have_cmd pkg-config || pc_bin=pkgconf
    if ! "$pc_bin" --exists "$pc" >/dev/null 2>&1; then
        add_missing "$pc" "$pkg"
        return 1
    fi
    return 0
}

check_header() {
    local header="$1"
    local pkg="$2"
    local path
    for path in /usr/include /usr/local/include; do
        if [ -f "$path/$header" ] || [ -d "$path/$header" ]; then
            return 0
        fi
    done
    add_missing "header $header" "$pkg"
    return 1
}

detect_os() {
    local id="" id_like=""
    if [ -f /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        id="${ID:-}"
        id_like="${ID_LIKE:-}"
    fi

    case "$id" in
        debian|ubuntu|linuxmint|pop|raspbian|elementary|zorin|neon)
            OS_FAMILY="debian"
            return
            ;;
    esac

    case " $id_like " in
        *" debian "*|*" ubuntu "*)
            OS_FAMILY="debian"
            return
            ;;
    esac

    if [ -f /etc/debian_version ]; then
        OS_FAMILY="debian"
        return
    fi

    OS_FAMILY="unsupported"
}

check_debian_deps() {
    check_command git git || true
    check_command make make || true
    check_command cmake cmake || true
    check_command ninja ninja-build || true
    check_command meson meson || true
    check_command python3 python3 || true
    check_command perl perl || true
    check_command autoconf autoconf || true
    check_command automake automake || true
    # Debian's libtool package provides libtoolize; libtool-bin adds the
    # libtool wrapper. Either is enough for the autoconf build path.
    if ! have_cmd libtool && ! have_cmd libtoolize; then
        add_missing "libtool/libtoolize" libtool
    fi
    check_command autoreconf autoconf || true
    check_command aclocal automake || true
    check_command autopoint gettext || true
    check_command gettext gettext || true
    # Debian may provide pkg-config or pkgconf
    if ! have_cmd pkg-config && ! have_cmd pkgconf; then
        add_missing "pkg-config" pkg-config
    fi
    check_command gcc gcc || true
    check_command g++ g++ || true
    check_command glslc glslc || true

    if ! have_cmd nasm && ! have_cmd yasm; then
        add_missing "nasm or yasm" nasm
    fi

    if ! have_cmd curl && ! have_cmd wget; then
        add_missing "curl or wget" curl
    fi

    check_perl_module "List::MoreUtils" liblist-moreutils-perl || true
    check_perl_module "XML::Parser" libxml-parser-perl || true

    # Eigen3: prefer pkg-config, fall back to headers.
    if have_cmd pkg-config || have_cmd pkgconf; then
        check_pkg_config eigen3 libeigen3-dev || true
    else
        check_header "eigen3/Eigen/Core" libeigen3-dev || \
            check_header "Eigen/Core" libeigen3-dev || true
    fi
}

print_install_hint() {
    local sorted
    # Use absolute paths: PATH may be empty in tests / constrained environments.
    sorted=$(printf '%s\n' "${MISSING_PACKAGES[@]}" | /usr/bin/sort -u | /usr/bin/tr '\n' ' ')
    sorted=${sorted%% }
    echo ""
    echo "Install missing dependencies with: sudo apt install -y ${sorted}"
}

main() {
    detect_os

    if [ "$OS_FAMILY" != "debian" ]; then
        echo "Preflight dependency check is not supported on this OS."
        echo "Only Debian/Ubuntu-family distributions are supported."
        echo "Install the build tools required by scripts/build-shotcut.sh manually, then continue."
        exit 2
    fi

    echo "Checking dependencies for Debian/Ubuntu..."
    echo ""
    check_debian_deps

    if [ ${#MISSING_PACKAGES[@]} -eq 0 ]; then
        echo ""
        echo "All required dependencies are installed."
        exit 0
    fi

    print_install_hint
    exit 1
}

main "$@"
