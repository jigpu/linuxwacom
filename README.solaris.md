**Solaris Notes For linuxwacom**


# Overview  ###########################################################

[usbwcm STREAMS module]:
    https://github.com/linuxwacom/usbwcm/

Version 0.12.0 of the linuxwacom driver introduced support for the Solaris
operating system. This code is minimally tested and regressions may occur
without warning. At the time of writing, only Solaris 10 U9 and U11
("5/10" and "1/13") have been explicitly tested for compatibility. Please
contact the linuxwacom project maintainers through SourceForge or Github
if you have specific concerns or issues using this driver on Solaris.

Note that these sources do not contain any Solaris kernel code. License
incompatibilities prevent us from distributing both as a combined work.
Please be sure to install the [usbwcm STREAMS module][] in addition to
this driver.


# Usage  ##############################################################

[configuring X11]:
    http://linuxwacom.sourceforge.net/index_old.php/howto/x11

The Xorg server must be manually configured to make use of connected
tablets in Solaris 10. Please see the [configuring X11][] section of
the linuxwacom manual for details on the modifications that must be
made to the `/etc/X11/xorg.conf` file. The appropriate device node
(e.g. `/dev/usb/hid0`) will need to be determined based on the output
of `dmesg`. See the Troubleshooting section below if you encounter
problems. The mouse pointer should move in response to pen input once
the X server is properly configured.

Some applications require additional configuration to make full use
of tablet data. For example, GIMP must have the "stylus" and "eraser"
tools set to "Screen" mode in its Extended Input Devices preferences.
It is impossible to provide instructions for each application. Please
see your application documentation for details if the tablet does not
appear to be automatically detected.

The linuxwacom driver provides a pair of configuration utilities
that can be used to modify or tune the behavior of connected tablets.
The `xsetwacom` utility provides a scriptable command-line interface,
while `wacomcpl` is a graphical Tcl/Tk application. Debug utilities
such as `wacdump` and `xidump` are also included should issues arise.


# Build, Install, and Uninstall #######################################

## Solaris 10 ##

Although the linuxwacom driver can be built for Solaris 10, the process
is non-obvious largely due to a lack of Xorg SDK development headers. An
interactive script which takes care of the entire build process (from
obtaining prerequisites to setting up environment variables) should have
been distributed alongside this file. Please note that the final command
may warn you that the `wacom_drv.so` file is already present on the system
and ask what you would like to do. This is normal: the SUNWxorg-server
package installs an out-of-date version of this file and you should tell the
system to replace it.

    # ./solaris-build.sh
    # cp workdir/WAClinuxwacom_<version>_<arch>.pkg.tar.gz /tmp
    # cd /tmp
    # gzcat WAClinuxwacom_<version>_<arch>.pkg.tar.gz | tar xf -
    # pkgadd -d .

Once the linuxwacom (and usbwcm) driver has been installed, you will need
to edit your `/etc/X11/xorg.conf` file as outlined above in the **Usage**
section. If no `xorg.conf` file exists, it may be necessary to first
generate one by running `/usr/X11/bin/xorgcfg` as root.

Reboot, and the screen cursor should follow the pen motion if everything has
been properly installed and configured.

Uninstalling the package can be achieved with the following commands:

    # pkgrm WAClinuxwacom

## Other Solaris Versions ##

Building the linuxwacom driver on other Solaris versions has not been
tested. Build instructions that work on vanilla OS installations would
be appreciated.


# Troubleshooting  ####################################################


## Is the tablet recognized by the kernel?

Examine the output of the `dmesg` command to ensure that the kernel detects
the tablet. After connecting the tablet, run the following commands:

    # dmesg | egrep -i "usba|hid|usbwcm|wacom"
    # ls -l /dev/usb/hid*

The tablet should have "Wacom" in its name or have a USB identifier
beginning with "usb56a". For example, the following example `dmesg`
output lists a "usb56a,59" device which is probed as "mouse@2, hid1".
The "hid1" device is later clarified to be "/pci@0,0/pci106b,3f@6/mouse@2"
which we see is the same as `/dev/usb/hid1`:

    $ dmesg | egrep -i "usba|hid|usbwcm|wacom"
    [...]
    usba: [ID 912658 kern.info] USB 2.0 device (usb56a,59) operating at full speed (USB 1.x) on USB 1.10 root hub: mouse@2, hid1 at bus address 3
    usba: [ID 349649 kern.info]     Tablet DTH-2241 Tablet
    genunix: [ID 936769 kern.info] hid1 is /pci@0,0/pci106b,3f@6/mouse@2
    genunix: [ID 408114 kern.info] /pci@0,0/pci106b,3f@6/mouse@2 (hid1) online
    [...]
    
    $ ls -l /dev/usb/hid*
    lrwxrwxrwx   1 root     root          48 Apr 17 15:31 /dev/usb/hid1 -> ../../devices/pci@0,0/pci106b,3f@6/mouse@2:mouse


## Is the STREAMS module installed?

The linuxwacom driver requires the `usbwcm` STREAMS module to be installed
and functional. Please see the instructions above for information about
where the module may be found. Follow the troubleshooting steps that it
suggests if the module has already been installed.


## Is the device node configured correctly?

Devices which appear under the `/dev/usb` directory may change their
name depending on the order the kernel discovers them. Moving the tablet
to a different USB port may result in it getting renamed. Follow the
instructions in the **"Is the tablet recognized by the kernel?"**
troubleshooting section above to determine which device is associated with
the tablet. Ensure that the `/etc/X11/xorg.conf` file is configured to use
this device.


## Are there any errors logged by Xorg?

The Xorg server generates log files under the `/var/log` directory which
can provide insight into possible configuration errors. The `/var/log/Xorg.0.log`
file and `/etc/Xorg.0.log.old` files contain information about the current
and last X server runs, respectively.


## Is the server not starting?

Some configuration errors can prevent the X server from starting at all.
If you are stuck at a console (or an SSH connection), the display server
can be restarted with the following command. If the problem is resolved
the login screen should appear once again.

    # svcadm restart cde-login


## Does your application not recognize the tablet?

Some applications (e.g. GIMP) require additional setup beyond the
`/etc/X11/xorg.conf` file in order to recognize the tablet. This
situation will typically present itself as the tablet working
properly on the desktop, but not providing pressure or other data
to the desired application. Unfortunately, it is not possible to
provide a guide to all the different ways an application may need
to be configured before it starts working with the tablet. Please
see the documenation provided by the software for more information.


## Driver debug logs

When debugging issues with the driver, it may be useful to have it
log additional debug information to the `/var/log/Xorg.0.log` file.
This can be achived by adding the two following options to any or
all of the Wacom devices in `/etc/X11/xorg.conf`.

    Option "DebugLevel" "<number>"
    Option "CommonDBG" "<number>"

The number specified indicates the log verbosity. A value of 0
(default) disables debug logging. Higher values -- up to 12 -- 
provide additional information about the driver's inner workings.
