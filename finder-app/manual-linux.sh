#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
BUILD=/tmp/build_uru
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

    # Task: Add your kernel build steps here
	# START DC_MODS
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper #clean removing .config file
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig #Setup default config
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all #Build kernel image
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs #Build devicetree
fi

echo "Adding the Image in outdir"
cp -v "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" "${OUTDIR}"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Task: Create necessary base directories
# DC_ROOM:
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
cd ..

#END DC_ROOM

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Task:  Configure busybox
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} menuconfig
	make defconfig
else
    cd busybox
fi

# DC_ROOM:
# Task: Make and install busybox
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd "$OUTDIR"

echo "Library dependencies"

INTERPRETER=$(${CROSS_COMPILE}readelf -a rootfs/bin/busybox | \
              grep "Requesting program interpreter:" | \
              awk -F': ' '{print $2}' | sed 's/]*$//')
SHAREDLIB=$(${CROSS_COMPILE}readelf -a rootfs/bin/busybox | \
            grep "Shared library:" | \
            sed -e 's/.*Shared library: *\[//' -e 's/\].*$//')

SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)

if [ -z "$SYSROOT" ]; then
    echo "Error: Could not determine sysroot path."
    exit 1
fi


# Copy the interpreter to rootfs
if [ ! -z "$INTERPRETER" ]; then
    #INTERPRETER_PATH=$(dirname "$INTERPRETER")
    INTERPRETER_PATH=$(find "$SYSROOT" -name $(basename "$INTERPRETER"))
    mkdir -p "rootfs${INTERPRETER_PATH}"
    #cp -v $(find ${OUTDIR}/rootfs -name $(basename "$INTERPRETER")) "rootfs${INTERPRETER}"
    cp -v "$INTERPRETER_PATH" "rootfs${INTERPRETER}"
fi


# Copy the shared libraries to rootfs
for LIB in $SHAREDLIB; do
    LIB_PATH=$(find "$SYSROOT" -name "$LIB")
    if [ ! -z "$LIB_PATH" ]; then
        DEST_DIR="rootfs/lib64"
        mkdir -p "$DEST_DIR"
        cp -v "$LIB_PATH" "$DEST_DIR"
    else
        echo "Warning: Library $LIB not found in sysroot."
    fi
done

# Task: Make device nodes

mkdir -p rootfs/dev

sudo mknod -m 666 rootfs/dev/null c 1 3
sudo mknod -m 600 rootfs/dev/console c 5 1


# Task: Clean and build the writer utility
make -C "$FINDER_APP_DIR" clean #-C tells make to change to the directory specified by $FINDER_APP_DIR
make -C "$FINDER_APP_DIR" CROSS_COMPILE=aarch64-none-linux-gnu-

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
#cp ${FINDER_APP_DIR}/start-qemu-app.sh ${OUTDIR}/rootfs/home
#cp ${FINDER_APP_DIR}/autorun-terminal.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home

# Task: Chown the root directory
sudo chown -R root rootfs

# Task: Create initramfs.cpio.gz

if [ -e ${OUTDIR}/initramfs.cpio.gz ]; then
    rm ${OUTDIR}/initramfs.cpio.gz
fi

cd rootfs

find . | cpio -H newc -ov --owner root:root > /tmp/initramfs.cpio
gzip -f /tmp/initramfs.cpio

mv /tmp/initramfs.cpio.gz ${OUTDIR}/initramfs.cpio.gz

#qemu-system-arm -m 1024M -nographic -M versatilepb -kernel zImage -append "console=ttyAMA0 rdinit=bin/sh" -dtb versatile-pb.dtb -initrd initramfs.cpio.gz
#"$FINDER_APP_DIR/start-qemu-terminal.sh" "${OUTDIR}"
#"$FINDER_APP_DIR/start-qemu-app.sh" "${OUTDIR}"

#END DC_ROOM
