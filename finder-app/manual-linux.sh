#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    make     ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make     ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

if [ ! -f "${OUTDIR}/Image" ] ; then
    sudo cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ] ; then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sys sbin tmp
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
mkdir -p conf

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ] ; then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    make distclean
    make defconfig
else
    cd busybox
fi

make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

echo "Library dependencies"
#${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
#${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

libs_output=`${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep -e "program interpreter" -e "Shared librar"`

libs_list=
# I'm too lazy to do it manually...
while read -r line ; do
    line=`echo "${line}" | cut -d] -f1`
    tmp=`echo "${line}" | cut -d[ -f2`
    if [ ! -z "${tmp}" ] ; then line="${tmp}" ; fi
    tmp=`echo "${line}" | cut -d: -f2`
    if [ ! -z "${tmp}" ] ; then line="${tmp}" ; fi

    while [ true ] ; do
	tmp=`echo "${line}" | cut -d/ -f1`
	if [ "${tmp}" == "${line}" ] ; then break ; fi
	if [ -z "${tmp}" ] ; then break ; fi
	line="${line:${#tmp}+1}"
    done

    libs_list="${libs_list} ${line}"
done <<< "${libs_output}"

sysroot=`${CROSS_COMPILE}gcc -print-sysroot`

# copying the libraries
for thelib in ${libs_list} ; do
    src=`find ${sysroot} -name ${thelib} | tail -n 1`
    dst="lib"
    if [ ! -z `echo "${src}" | grep "lib64"` ] ; then dst="${dst}64" ; fi
    echo "Copying ${thelib} to ${dst}..."
    cp ${src} ${OUTDIR}/rootfs/${dst}/.
done

cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE} all

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp    writer ${OUTDIR}/rootfs/home/.
cp    *.sh   ${OUTDIR}/rootfs/home/.
cp -r conf   ${OUTDIR}/rootfs/home/.
cp -r conf/* ${OUTDIR}/rootfs/conf/.

# Chown the root directory
cd ${OUTDIR}
sudo chown -R root:root rootfs

# Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
echo "Find $?"
cd ${OUTDIR}
gzip -f ${OUTDIR}/initramfs.cpio
echo "GZip $?"
