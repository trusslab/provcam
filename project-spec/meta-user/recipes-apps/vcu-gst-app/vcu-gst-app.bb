#
# This is the vct-gst-app application recipe
#
#
SUMMARY = "vcu_gst_app application"
SECTION = "PETALINUX/apps"
DESCRIPTION = "gstreamer test applications for VCU in ZynqMP"
LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://COPYING;md5=1a59b35f362129863822d80cb0e05bae"

SRC_URI = " \
	file://AUTHORS;subdir=vcu_gst_apps \
	file://autogen.sh;subdir=vcu_gst_apps \
	file://configure.ac;subdir=vcu_gst_apps \
	file://COPYING;subdir=vcu_gst_apps \
	file://Makefile.am;subdir=vcu_gst_apps \
	file://vcu_apm_lib.zip;name=vcu_apm_lib;subdir=vcu_gst_apps/lib/vcu_apm_lib \
	file://vcu_video_lib.zip;name=vcu_video_lib;subdir=vcu_gst_apps/lib/vcu_video_lib \
	file://vcu_gst_lib.zip;name=vcu_gst_lib;subdir=vcu_gst_apps/lib/vcu_gst_lib \
	file://vcu_gst_app.zip;name=vcu_gst_app;subdir=vcu_gst_apps/app/vcu_gst_app"

S = "${WORKDIR}/vcu_gst_apps"

DEPENDS = "gstreamer1.0 gstreamer1.0-plugins-base v4l-utils libdrm"

# NOTE: if this software is not capable of being built in a separate build directory
# from the source, you should replace autotools with autotools-brokensep in the
# inherit line
inherit pkgconfig autotools

# Specify any options you want to pass to the configure script using EXTRA_OECONF:
EXTRA_OECONF = ""

FILES_${PN} += "/usr/bin/vcu_gst_app"

do_install() {
	install -d ${D}/${bindir}
	oe_runmake install DESTDIR=${D}
	install -m 0755 ${B}/vcu_gst_app ${D}/${bindir}
}
