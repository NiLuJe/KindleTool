#!/bin/sh

# Worker bee for re-package tasks
# Creates a package model locked to everything.
# https://www.adrive.com/public/WKQ7Qm/mkpkgs.sh.zip

# Using at least KindleTool-v1.6.4.108-ge16765c or more recent.
# And persuming it is on your path (/usr/local/bin is a good place for it).
# No version shown here because this is actually a sym-link to the binary.
# KT='/usr/local/bin/kindletool'
KT='kindletool'

# Everything except the kitchen sink:
ALL='ota2 -d touch -d 295 -d 296 -d 297 -d 298 -d 2E1 -d 2E2 -d 2E6 -d 2E7 -d 2E8 -d 341 -d 342 -d 343 -d 344 -d 347 -d 34A -O -s 0 -t 18446744073709551615'

RIP="$KT convert -k "
PAK="$KT create $ALL "

echo $RIP
echo $PAK

# call: mkpkgs.sh <arg-1>
# Where:
#   no arg-1    Do entire table (That is, do the place holder '-')
#   character   Select by field one of each row (I.E: by groups)
# That is, you may select everything or select one group.
# If we ever have more than 62 packages, this script is in trouble.

# Fields of table that follows (none can be empty)
# Fields are seperated by white space, literals are not quoted
# 1: Selection where '-' is a place holder for the default of "everything"
# The first field may contain multiple selection characters (defines "groups")
# 2: Input *.bin to un-package
# 3: Output *.tar.gz from un-package is the input to create
# 4: Output *.bin from create

KEY='-'     # Default is the place holder character for 'all'.
# Key : k   # KUALBooklet
# Key : u   # usbnet
# Key : r   # rescue pack
# Key : c   # rescue pack with cowards rescue pack
# Key : p   # Python

if [ $# -gt 0 ] ; then
    KEY=$1
fi

export 'KT_WITH_UNKNOWN_DEVCODES=1'

while read ctl pkg_in pkg_tmp pkg_out ; do
#   echo 'ctl = ' $ctl ' extract = ' ${ctl#*$KEY}
    if [ "${ctl#*$KEY}" != "$ctl" ]
    then
    #    printf '\n' 
    #    printf '%s %s %s %s\n' $ctl $pkg_in $pkg_tmp $pkg_out
        $RIP $pkg_in  2>&1
        $PAK $pkg_tmp $pkg_out 2>&1
    fi
done <<EoF
-k  Update_KUALBooklet_v2.7_install.bin                     Update_KUALBooklet_v2.7_install_converted.tar.gz                      Update_KUALBooklet_v2.7_koa2_nomax_install.bin
-k  Update_KUALBooklet_v2.7_uninstall.bin                   Update_KUALBooklet_v2.7_uninstall_converted.tar.gz                    Update_KUALBooklet_v2.7_koa2_nomax_uninstall.bin
-u  Update_usbnet_0.21.N_install_pw2_kt2_kv_pw3_koa_kt3.bin Update_usbnet_0.21.N_install_pw2_kt2_kv_pw3_koa_kt3_converted.tar.gz  Update_usbnet_0.21.N_install_mx7_koa2_nomax.bin
-u  Update_usbnet_0.21.N_uninstall.bin                      Update_usbnet_0.21.N_uninstall_converted.tar.gz                       Update_usbnet_0.21.N_uninstall_koa2_nomax.bin
-c  Update_crp_2.N_install.bin                              Update_crp_2.N_install_converted.tar.gz                               Update_crp_2.N_install_koa2_nomax.bin
-c  Update_crp_2.N_uninstall.bin                            Update_crp_2.N_uninstall_converted.tar.gz                             Update_crp_2.N_uninstall_koa2_nomax.bin
-rc Update_rp_20131220.N_install.bin                        Update_rp_20131220.N_install_converted.tar.gz                         Update_rp_20131220.N_install_koa2_nomax.bin
-p  Update_python_0.14.N_install_pw2_kt2_kv_pw3_koa_kt3.bin Update_python_0.14.N_install_pw2_kt2_kv_pw3_koa_kt3_converted.tar.gz  Update_python_0.14.N_install_koa2_nomax.bin
-p  Update_python_0.14.N_uninstall.bin                      Update_python_0.14.N_uninstall_converted.tar.gz                       Update_python_0.14.N_uninstall_koa2_nomax.bin
EoF
