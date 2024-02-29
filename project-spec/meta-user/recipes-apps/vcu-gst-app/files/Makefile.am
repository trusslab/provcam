
bin_PROGRAMS = vcu_gst_app
lib_LIBRARIES = libvcu_apm_lib.a libvideo.a libvcu_gst_lib.a

AM_DEFAULT_SOURCE_EXT = .c

vcu_gst_app_INC_DIR = ../vcu_gst_apps/app/vcu_gst_app/include
vcu_apm_lib_INC_DIR = ../vcu_gst_apps/lib/vcu_apm_lib/include
vcu_video_lib_INC_DIR = ../vcu_gst_apps/lib/vcu_video_lib/include
vcu_gst_lib_INC_DIR = ../vcu_gst_apps/lib/vcu_gst_lib/include

VCU_GST_CFLAGS = $(GST_CFLAGS) $(pkg-config --cflags  gstreamer-1.0 gstreamer1.0-plugins-base libdrm libv4l2 libmediactl)
VCU_GST_LIBS = $(GST_LIBS) $(pkg-config --libs gstreamer-1.0 gstreamer1.0-plugins-base libdrm libv4l2 libmediactl) -lgstvideo-1.0


nodist_vcu_gst_app_SOURCES = \
		$(vcu_gst_app_INC_DIR)/configs.h \
		$(vcu_gst_app_INC_DIR)/vcu_gst_app.h
nodist_libvcu_apm_lib_a_SOURCES = \
		$(vcu_apm_lib_INC_DIR)/uio_common.h \
		$(vcu_apm_lib_INC_DIR)/uio_perfmon.h \
		$(vcu_apm_lib_INC_DIR)/uio_perfmon_hw.h
nodist_libvideo_a_SOURCES = \
		$(vcu_video_lib_INC_DIR)/drm_helper.h \
		$(vcu_video_lib_INC_DIR)/gpio_utils.h \
		$(vcu_video_lib_INC_DIR)/helper.h \
		$(vcu_video_lib_INC_DIR)/mediactl_helper.h \
		$(vcu_video_lib_INC_DIR)/v4l2_subdev_helper.h \
		$(vcu_video_lib_INC_DIR)/vcap_csi.h \
		$(vcu_video_lib_INC_DIR)/vcap_csi_int.h \
		$(vcu_video_lib_INC_DIR)/vcap_hdmi_int.h \
		$(vcu_video_lib_INC_DIR)/vcap_scd_int.h \
		$(vcu_video_lib_INC_DIR)/vcap_sdi_int.h \
		$(vcu_video_lib_INC_DIR)/vcap_tpg.h \
		$(vcu_video_lib_INC_DIR)/vcap_tpg_int.h \
		$(vcu_video_lib_INC_DIR)/video.h \
		$(vcu_video_lib_INC_DIR)/video_int.h \
		$(vcu_video_lib_INC_DIR)/xilinx-v4l2-controls.h
nodist_libvcu_gst_lib_a_SOURCES = \
		$(vcu_gst_lib_INC_DIR)/vgst_config.h \
		$(vcu_gst_lib_INC_DIR)/vgst_err.h \
		$(vcu_gst_lib_INC_DIR)/vgst_lib.h \
		$(vcu_gst_lib_INC_DIR)/vgst_pipeline.h \
		$(vcu_gst_lib_INC_DIR)/vgst_split_pipeline.h \
		$(vcu_gst_lib_INC_DIR)/vgst_utils.h

libvcu_apm_lib_a_includedir = /usr/include
libvcu_apm_lib_a_include_HEADERS = \
		$(vcu_apm_lib_INC_DIR)/perfapm.h
		$(vcu_apm_lib_INC_DIR)/uio_common.h
		$(vcu_apm_lib_INC_DIR)/uio_perfmon.h
		$(vcu_apm_lib_INC_DIR)/uio_perfmon_hw.h
libvideo_a_includedir = /usr/include
libvideo_a_include_HEADERS = \
		$(vcu_video_lib_INC_DIR)/video.h
libvcu_gst_lib_a_includedir = /usr/include
libvcu_gst_lib_a_include_HEADERS = \
		$(vcu_gst_lib_INC_DIR)/vgst_err.h \
		$(vcu_gst_lib_INC_DIR)/vgst_lib.h


libvcu_apm_lib_a_SOURCES = \
			lib/vcu_apm_lib/src/perfmon.c \
			lib/vcu_apm_lib/src/uio_common.c \
			lib/vcu_apm_lib/src/uio_perfmon.c
libvideo_a_SOURCES = \
			lib/vcu_video_lib/src/drm_helper.c \
			lib/vcu_video_lib/src/gpio_utils.c \
			lib/vcu_video_lib/src/mediactl_helper.c \
			lib/vcu_video_lib/src/v4l2_subdev_helper.c \
			lib/vcu_video_lib/src/vcap_csi.c \
			lib/vcu_video_lib/src/vcap_hdmi.c \
			lib/vcu_video_lib/src/vcap_scd.c \
			lib/vcu_video_lib/src/vcap_sdi.c \
			lib/vcu_video_lib/src/vcap_tpg.c \
			lib/vcu_video_lib/src/video.c \
			lib/vcu_video_lib/src/video_src.c
libvcu_gst_lib_a_SOURCES =  \
			lib/vcu_gst_lib/src/vgst_lib.c \
			lib/vcu_gst_lib/src/vgst_pipeline.c \
			lib/vcu_gst_lib/src/vgst_split_pipeline.c \
			lib/vcu_gst_lib/src/vgst_utils.c
vcu_gst_app_SOURCES = \
			app/vcu_gst_app/src/vcu_gst_app.c

vcu_gst_app_CFLAGS = $(VCU_GST_CFLAGS) -I$(vcu_gst_app_INC_DIR) -I$(vcu_apm_lib_INC_DIR) -I$(vcu_video_lib_INC_DIR) -I$(vcu_gst_lib_INC_DIR) -I../recipe-sysroot/usr/include/drm -I../recipe-sysroot/usr/include
libvcu_apm_lib_a_CFLAGS = $(VCU_GST_CFLAGS) -I$(vcu_apm_lib_INC_DIR)
libvideo_a_CFLAGS = $(VCU_GST_CFLAGS) -I$(vcu_video_lib_INC_DIR) -I../recipe-sysroot/usr/include/drm -I../recipe-sysroot/usr/include
libvcu_gst_lib_a_CFLAGS = $(VCU_GST_CFLAGS) -I$(vcu_video_lib_INC_DIR) -I$(vcu_gst_lib_INC_DIR)


libvcu_gst_lib_a_LDFLAGS = -lvideo $(VCU_GST_LIBS) -lmediactl -lv4l2subdev -ldrm
libvcu_gst_lib_a_DEPENDENCIES = libvideo.a
vcu_gst_app_LDFLAGS = -L$(CURDIR) -lvcu_apm_lib -lvcu_gst_lib -lvideo -lmediactl -lv4l2subdev -ldrm $(VCU_GST_LIBS)
vcu_gst_app_DEPENDENCIES = libvcu_apm_lib.a libvideo.a libvcu_gst_lib.a
