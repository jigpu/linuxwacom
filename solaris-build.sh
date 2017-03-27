#!/bin/bash

set -e


#
# Produce a version number from the output of `git describe` which
# is then massaged into the format used by semantic versioning.
#
function generate_linuxwacom_version() {
    local SRCDIR="$1"
    local SUFFIX="$2"
    if [[ -n "$SUFFIX" ]]; then
        SUFFIX="-$SUFFIX"
    fi
    git -C "$SRCDIR" describe | \
    sed -e "s/^release-\(.*\)/\1/" \
        -e "s/^\(.*\)-\([0-9]*\)-\(g[0-9a-f]\{7,7\}\)\$/\1${SUFFIX}+r\2.\3/"
}


#
# Return the value associated with a given key in a line-separated
# list of key="value" or key=value pairs.
#
function key_to_value() {
    local KEY="$1"
    local LIST="$2"
    sed -n -e 's%^'"$KEY"'="\(.*\)"%\1%p' -e 's%^'"$KEY"'=\(.*\)%\1%p' <<<"$LIST" | head -n1
}


#
# Create a Solaris ".pkg.tar.gz" package from the contents of a
# working directory which contains finalized Prototype and pkginfo
# files.
#
function create_package() {
    local PKGDIR="$1"
    local PKGINFO=$(cat "$PKGDIR/pkginfo")
    local PKG=$(key_to_value "PKG" "$PKGINFO")
    local ARCH=$(key_to_value "ARCH" "$PKGINFO")
    local VERSION=$(key_to_value "VERSION" "$PKGINFO")

    pkgmk -o -d "$PKGDIR" -f "$PKGDIR/Prototype"
    tar -cf - -C "$PKGDIR" $PKG | gzip -9 -c > "$PKGDIR/${PKG}_${VERSION}_${ARCH}.pkg.tar.gz"
}


#
# Create "prototype" information about the files that are to be
# package. This function performs basic transformation of the
# raw output provided by the `pkgproto` command but will likely
# need further modification on a case-by-case basis to ensure
# permissions are set properly.
#
# See `man -s4 prototype`, [1], and [2] for more information.
#
# [1]: http://www.ibiblio.org/pub/packages/solaris/i86pc/html/creating.solaris.packages.html
# [2]: http://www.garex.net/sun/packaging/pkginfo.html
#
function generate_prototype() {
    local DESTDIR="$1"
    local PROTO
    PROTO=$(pkgproto "$DESTDIR=/")
    PROTO=$(echo 'i pkginfo'; echo '!default 0755 root bin'; echo "$PROTO")
    PROTO=$(sed '/^d none \/ /d' <<<"$PROTO")
    PROTO=$(sed 's/^\(d .*\) [0-7]\{4\} .* .*$/\1 ? ? ?/' <<<"$PROTO")
    PROTO=$(sed 's/^\(f .*\) [0-7]\{4\} .* .*$/\1/' <<<"$PROTO")
    echo "$PROTO"
}


#
# Creates a linuxwacom driver package from files installed to an
# alternate (non-root) directory.
#
# NOTE! When installing this package, Solaris will likely complain
# that wacom_drv.so is already installed on the system as part of
# the SUNWxorg-server package. This is normal and the file should
# be overwritten.
#
function package_linuxwacom() {
    local DESTDIR="$1"
    local PKGDIR="$2"
    local VERSION="$3"
    local PROTO=$(generate_prototype "$DESTDIR")

    # Remove "?" permissions on "wacomcfg" and "TkXinput" directories
    PROTO=$(sed 's/^\(d .*wacomcfg.*\) ? ? ?$/\1/' <<<"$PROTO")
    PROTO=$(sed 's/^\(d .*TkXinput.*\) ? ? ?$/\1/' <<<"$PROTO")

    PKGINFO=$(cat <<EOF
PKG=WAClinuxwacom
NAME="Xorg driver for Wacom tablets"
VERSION="$VERSION"
ARCH="$(isainfo -n)"
CLASSES="system none"
CATEGORY="system"
VENDOR="linuxwacom Project"
EMAIL="linuxwacom-discuss@lists.sourceforge.net"
EOF
)
    echo "$PKGINFO" > "$PKGDIR/pkginfo"
    echo "$PROTO" > "$PKGDIR/Prototype"

    create_package "$PKGDIR"
}


#
# Installs the linuxwacom driver to a specified destination
# directory.
#
function install_linuxwacom() {
    local SRCDIR="$1"
    local DESTDIR="$2"

    pushd "$SRCDIR"
    gmake install DESTDIR="$DESTDIR"
    popd
}


#
# Compiles the linuxwacom driver, setting up the environment along the
# way to comply with the unique needs of Solaris 10. The function requires
# that your provide it with a path to the "/usr/src/uts/common" directory
# of the usbwcm source tree. Additional configuration options (e.g.
# "--with-tcl=/opt/csw --with-tk=/opt/csw" can also be provided in order
# to change how the driver is built.
#
function compile_linuxwacom() {
    local SRCDIR="$1"
    local HEADERS_KERNEL="$2"
    local CONFIGOPTS="$3"
    local XORG_ALTERNATE="$4"

    pushd "$SRCDIR"

    export CFLAGS="$CFLAGS -I${HEADERS_KERNEL} -fPIC"

    if [[ -n "$XORG_ALTERNATE" ]]; then
        # Solaris 10 does not have the necessary Xorg SDK headers available
        # for installation, so this script has to download them into an
        # alternate root directory. If an alternate root was provided to
        # this function, then update CFLAGS and PKG_CONFIG_PATH so that
        # everything can be found...
        export CFLAGS="$CFLAGS -I${XORG_ALTERNATE}/usr/X11/include"
        export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$XORG_ALTERNATE/usr/lib/pkgconfig"
    fi

    # Note that PATH should have already been set up at this point to
    # contain the following important paths:
    #   * /usr/ccs/bin contains development tools (ar, ranlib)
    #   * /usr/sfw/bin contains third-party software (gcc, gmake, gar, granlib)
    #   * /opt/csw/bin contains OpenCSW software (libtool, autoconf, etc.)
    ./bootstrap
    ./configure --prefix="/usr" --with-linux \
                --with-xmoduledir=/usr/X11/lib/modules/input \
                --with-xorg-sdk="$XORG_ALTERNATE/usr/X11/include/xorg" $CONFIGOPTS
    gmake

    popd
}


#
# Download the latest code from an appropriate location.
# At the moment, grab it from my Github repository.
#
function get_source() {
    local SRCDIR="$1"
    local REPO="$2"
    local COMMIT="$3"

    if [[ "$REPO" == "linuxwacom" ]]; then
        # Legacy linuxwacom repo is named "code" on SourceForge...
        REPO="code"
    fi

    local URL="http://git.code.sf.net/p/linuxwacom/${REPO}.git"

    if [[ ! -d "$SRCDIR" ]]; then
        git clone -b "$COMMIT" "$URL" "$SRCDIR"
    fi
}


#
# The Xorg SDK and other required headers must be installed for linuxwacom
# to build. In theory, installing SUNWxorg-headers and its dependencies
# should be sufficient. In reality, the version of SUNWxorg-headers
# contained on the Solaris 10 DVD does *not* contain the full SDK. We
# need to manually get everything from the OpenIndiana legacy archive.
# This requires a bit of futzing because OpenIndiana assumes the presence
# of an IPS package manager which isn't present on my Solaris 10 installs.
# See [1] ("How to manually download individual files from the OpenIndiana
# (or Solaris) pkg repo?") for an overview of what we're doing to emulate
# one.
#
# [1]: https://serverfault.com/questions/348139/
#
function get_xorg_sdk() {
    local WORKDIR="$1"
    local SVR="http://pkg.openindiana.org/legacy"
    local X11_PKG="SUNWxwinc@0.5.11,5.11-0.101:20081119T231501Z"
    local XORG_PKG="SUNWxorg-headers@0.5.11,5.11-0.101:20081119T231341Z"

    # Download package manifests
    wget -cP "$WORKDIR" "$SVR/manifest/0/$X11_PKG" "$SVR/manifest/0/$XORG_PKG"

    # Download gzipped package contents
    cat "$WORKDIR/$X11_PKG" "$WORKDIR/$XORG_PKG" | \
      sed -n "s%^file \([^ ]*\).*%$SVR/file/0/\1%p" | xargs wget -cP "$WORKDIR"

    # Create destination directories
    cat "$WORKDIR/$X11_PKG" "$WORKDIR/$XORG_PKG" | \
      sed -n "s%^dir.*path=\([^ ]*\).*%mkdir -p \"$WORKDIR/\1\"%p" | sh -s

    # Extract gzipped files to destinations
    cat "$WORKDIR/$X11_PKG" "$WORKDIR/$XORG_PKG" | \
      sed -n "s%^file \([^ ]*\).* path=\([^ ]*\).*%\1|\2%p" | \
      sed "s%\([^|]*\)|\(.*\)%gzcat \"$WORKDIR/\1\" > \"$WORKDIR/\2\"%" | sh -s

    # Set up symlinks
    cat "$WORKDIR/$X11_PKG" "$WORKDIR/$XORG_PKG" | \
      sed -n "s%^link.*path=\([^ ]*\).*target=\([^ ]*\).*%\1|\2%p" | \
      sed "s%\([^|]*\)|\(.*\)%ln -s \"\2\" \"$WORKDIR/\1\"%" | sh -s

    # Remove temporary files
    rm -f "$WORKDIR"/* 2> /dev/null || true
}


#
# The linuxwacom driver has a few build dependencies that must be
# satisfied before the code can compile. In theory, we might be
# able to find official Oracle/Sun packages that fit the bill; in
# reality its just easier to get them from the OpenCSW Software
# Archive. Ask before actually installing anything to be courteous.
#
function install_build_deps() {
    local PACKAGES="$@"
    local INSTALL=0

    if [[ ! -d /opt/csw || ! -e /opt/csw/bin/pkgutil ]]; then
        echo "OpenCSW not found. It must be installed to obtain build dependencies."
        echo "Do you wish to proceed and install the OpenCSW package manager?"
        select yn in "Yes" "No"; do
            case $yn in
                Yes ) INSTALL=1; break;;
                No ) exit;;
                * ) echo "Please answer 'Yes' or 'No'.";;
            esac
        done
    elif [[ ! -x /opt/csw/bin/pkgutil ]]; then
        echo "OpenCSW pkgutil found, but not executable. Exiting."
        exit 1
    fi

    if [[ $INSTALL -ne 0 ]]; then
        yes | pkgadd -a <(echo setuid=nocheck) -d http://get.opencsw.org/now CSWpkgutil
    fi
    /opt/csw/bin/pkgutil -U
    yes | /opt/csw/bin/pkgutil -i $PACKAGES
}


#
# Ensure that the system has everything that it needs to perform a
# build. If prerequisites are not satisfied, ask the user if they
# would like to install them.
#
function check_prerequisites() {
    while true ; do
        local SYS=""
        local CSW=""
        local PATHMOD=""

        command -v "gcc"        >/dev/null 2>&1 || SYS="$SYS gcc"
        command -v "gmake"      >/dev/null 2>&1 || SYS="$SYS gmake"
        command -v "wget"       >/dev/null 2>&1 || SYS="$SYS wget"

        command -v "libtoolize" >/dev/null 2>&1 || CSW="$CSW libtool"
        command -v "autoconf"   >/dev/null 2>&1 || CSW="$CSW autoconf"
        command -v "automake"   >/dev/null 2>&1 || CSW="$CSW automake"
        command -v "gsed"       >/dev/null 2>&1 || CSW="$CSW gsed"
        command -v "git"        >/dev/null 2>&1 || CSW="$CSW git"
        command -v "/opt/csw/bin/pkg-config" >/dev/null 2>&1 || CSW="$CSW pkgconfig"

        if [[ -z "$SYS" && -z "$CSW" ]]; then
            break
        fi

        echo "Unable to find the following dependencies in the current PATH."
        echo "  ==> $SYS $CSW"
        echo

        if [[ "$PATH" != *"/usr/ccs/bin"* ]]; then
            PATHMOD="$PATHMOD:/usr/ccs/bin"
        fi
        if [[ "$PATH" != *"/usr/sfw/bin"* ]]; then
            PATHMOD="$PATHMOD:/usr/sfw/bin"
        fi
        if [[ "$PATH" != *"/opt/csw/bin"* ]]; then
            PATHMOD="$PATHMOD:/opt/csw/bin"
        fi

        if [[ -n "$PATHMOD" ]]; then
            echo "Would you like me to update PATH to \"\$PATH$PATHMOD\"?"
            select yn in "Yes" "No"; do
                case $yn in
                    Yes ) export PATH="$PATH$PATHMOD"; break;;
                    No ) exit 1;;
                    * ) echo "Please answer 'Yes' or 'No'.";;
                esac
            done
            continue
        elif [[ -n "$CSW" ]]; then
            echo "Would you like me to install packages from OpenCSW?"
            select yn in "Yes" "No"; do
                case $yn in
                    Yes ) install_build_deps $CSW; break;;
                    No ) exit 1;;
                    * ) echo "Please answer 'Yes' or 'No'.";;
                esac
            done
            continue
        else
            echo "Still unable to find necessary packages. Aborting."
            exit 1
        fi
    done

    if [[ ! -f "$XORGDIR/usr/X11/include/xorg/xorgVersion.h" ]]; then
        echo "You seem to be missing the Xorg SDK. Download?"
        select yn in "Yes" "No"; do
            case $yn in
                Yes ) get_xorg_sdk "$XORGDIR"; break;;
                No ) exit 1;;
                * ) echo "Please answer 'Yes' or 'No'.";;
            esac
        done
    fi

    return 0
}


WORKDIR="$(pwd)/workdir"
XORGDIR="$WORKDIR/xorg-sdk"
USBWCMDIR="$WORKDIR/usbwcm_src"

LINUXWACOM_SRCDIR="$WORKDIR/linuxwacom_src"
LINUXWACOM_DSTDIR="$WORKDIR/linuxwacom_dst"
LINUXWACOM_PKGDIR="$WORKDIR"


if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root."
    exit 1
fi

mkdir -p $WORKDIR || true

check_prerequisites

get_source "$USBWCMDIR" usbwcm master

if [[ -f "README.solaris.md" ]]; then	
	LINUXWACOM_SRCDIR="$(pwd)"
else
	get_source "$LINUXWACOM_SRCDIR" linuxwacom master
fi

compile_linuxwacom "$LINUXWACOM_SRCDIR" "$USBWCMDIR/usr/src/uts/common/" "" "$XORGDIR"
install_linuxwacom "$LINUXWACOM_SRCDIR" "$LINUXWACOM_DSTDIR"

LINUXWACOM_VERSION=$(generate_linuxwacom_version "$LINUXWACOM_SRCDIR" "")
while [[ -z "$LINUXWACOM_VERSION" ]]; do
	echo "Unable to determine linuxwacom version for packaging."
	echo -n "Please provide version (e.g. 0.12.0): "
	read LINUXWACOM_VERSION
done

package_linuxwacom "$LINUXWACOM_DSTDIR" "$LINUXWACOM_PKGDIR" "$LINUXWACOM_VERSION"
