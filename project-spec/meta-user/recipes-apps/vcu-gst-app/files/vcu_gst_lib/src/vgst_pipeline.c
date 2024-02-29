/******************************************************************************
 * (c) Copyright 2017-2020 Xilinx, Inc. All rights reserved.
 *
 * This file contains confidential and proprietary information of Xilinx, Inc.
 * and is protected under U.S. and international copyright and other
 * intellectual property laws.
 *
 * DISCLAIMER
 * This disclaimer is not a license and does not grant any rights to the
 * materials distributed herewith. Except as otherwise provided in a valid
 * license issued to you by Xilinx, and to the maximum extent permitted by
 * applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
 * FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
 * MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
 * and (2) Xilinx shall not be liable (whether in contract or tort, including
 * negligence, or under any other theory of liability) for any loss or damage
 * of any kind or nature related to, arising under or in connection with these
 * materials, including for any direct, or any indirect, special, incidental,
 * or consequential loss or damage (including loss of data, profits, goodwill,
 * or any type of loss or damage suffered as a result of any action brought by
 * a third party) even if such damage or loss was reasonably foreseeable or
 * Xilinx had been advised of the possibility of the same.
 *
 * CRITICAL APPLICATIONS
 * Xilinx products are not designed or intended to be fail-safe, or for use in
 * any application requiring fail-safe performance, such as life-support or
 * safety devices or systems, Class III medical devices, nuclear facilities,
 * applications related to the deployment of airbags, or any other applications
 * that could lead to death, personal injury, or severe property or
 * environmental damage (individually and collectively, "Critical
 * Applications"). Customer assumes the sole risk and liability of any use of
 * Xilinx products in Critical Applications, subject only to applicable laws
 * and regulations governing limitations on product liability.
 *
 * THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
 * AT ALL TIMES.
 *******************************************************************************/


#include <math.h>
#include "vgst_pipeline.h"

GST_DEBUG_CATEGORY_EXTERN (vgst_lib);
#define GST_CAT_DEFAULT vgst_lib


VGST_ERROR_LOG
create_pipeline (vgst_ip_params *ip_param, vgst_enc_params *enc_param, vgst_playback *play_ptr, guint sink_type, gchar *uri, vgst_aud_params *aud_param) {
    gint llp2_design = vlib_is_llp2_design();
    play_ptr->pipeline        = gst_pipeline_new ("vcu-trd");
    if (ip_param->src_type == FILE_SRC || STREAMING_SRC == ip_param->src_type)
      play_ptr->ip_src          = gst_element_factory_make (FILE_SRC_NAME,   NULL);
    else if (ip_param->src_type == LIVE_SRC)
      play_ptr->ip_src          = gst_element_factory_make (V4L2_SRC_NAME,   NULL);

    play_ptr->srccapsfilter   = gst_element_factory_make ("capsfilter",      NULL);
    play_ptr->queue           = gst_element_factory_make ("queue",           NULL);
    play_ptr->enc_queue       = gst_element_factory_make ("queue",           NULL);
    play_ptr->enccapsfilter   = gst_element_factory_make ("capsfilter",      NULL);
    play_ptr->deccapsfilter   = gst_element_factory_make ("capsfilter",      NULL);
    if (sink_type == DISPLAY) {
      play_ptr->videosink       = gst_element_factory_make ("kmssink",       NULL);
      play_ptr->fpsdisplaysink  = gst_element_factory_make ("fpsdisplaysink",NULL);
    } else if (sink_type == RECORD) {
      play_ptr->videosink       = gst_element_factory_make ("filesink",      NULL);
      printf("[myles]%s: sink_type is RECORD, where uri is: %s.\n", __func__, uri);
      if (strstr(uri, MP4_MUX_TYPE) != NULL) {
        play_ptr->mux           = gst_element_factory_make ("qtmux",         NULL);
      } else if (strstr(uri, MKV_MUX_TYPE) != NULL) {
        play_ptr->mux           = gst_element_factory_make (MKV_MUX_NAME,   NULL);
      } else if (strstr(uri, TS_MUX_TYPE) != NULL) {
        play_ptr->mux           = gst_element_factory_make (MPEG_TS_MUX_NAME,NULL);
      } else {
        play_ptr->mux           = gst_element_factory_make ("qtmux",         NULL);
      }
    } else if (sink_type == STREAM) {
      play_ptr->videosink   = gst_element_factory_make ("udpsink",           NULL);
      play_ptr->mux         = gst_element_factory_make (MPEG_TS_MUX_NAME,    NULL);
      if((llp2_design && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode) {
        play_ptr->rtpbin     = gst_element_factory_make ("rtpbin",           NULL);
        play_ptr->v_rtcpsink = gst_element_factory_make ("udpsink",          NULL);
        play_ptr->v_rtcpsrc  = gst_element_factory_make ("udpsrc",           NULL);
      }
    }
    if (ip_param->accelerator) {
      play_ptr->bypass     = gst_element_factory_make ("sdxbypass",       NULL);
      if (!play_ptr->bypass) {
        GST_ERROR ("FAILED to create bypass element");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->bypass, NULL);
    }
    if (ip_param->enable_roi) {
      play_ptr->roi     = gst_element_factory_make ("xlnxroivideo1detect",      NULL);
      if (!play_ptr->roi) {
        GST_ERROR ("FAILED to create roi element");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->roi, NULL);
    }
    if (ip_param->enable_scd && (SCD_MEMORY == ip_param->scd_type)) {
      play_ptr->xilinxscd     = gst_element_factory_make ("xilinxscd",       NULL);
      if (!play_ptr->xilinxscd) {
        GST_ERROR ("FAILED to create xilinxscd element");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->xilinxscd, NULL);
    }
    if (aud_param->enable_audio && (aud_param->audio_in == AUDIO_HDMI_IN || AUDIO_SDI_IN == aud_param->audio_in || AUDIO_I2S_IN == aud_param->audio_in ||\
        LIVE_SRC != ip_param->src_type)) {
      play_ptr->alsasrc         = gst_element_factory_make ("alsasrc",       NULL);
      play_ptr->alsasink        = gst_element_factory_make ("alsasink",      NULL);
      play_ptr->audconvert      = gst_element_factory_make ("audioconvert",  NULL);
      play_ptr->audresample     = gst_element_factory_make ("audioresample", NULL);
      play_ptr->audcapsfilter   = gst_element_factory_make ("capsfilter",    NULL);
      play_ptr->audconvert2     = gst_element_factory_make ("audioconvert",  NULL);
      play_ptr->audresample2    = gst_element_factory_make ("audioresample", NULL);
      play_ptr->audcapsfilter2  = gst_element_factory_make ("capsfilter",    NULL);
      play_ptr->volume          = gst_element_factory_make ("volume",        NULL);
      play_ptr->audqueue        = gst_element_factory_make ("queue",         NULL);

      if ((sink_type == STREAM) && ((llp2_design && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode)) {
        play_ptr->audioenc      = gst_element_factory_make ("opusenc",       NULL);
        play_ptr->audiortppay   = gst_element_factory_make ("rtpopuspay",    NULL);
        play_ptr->a_rtpsink     = gst_element_factory_make ("udpsink",       NULL);
        play_ptr->a_rtcpsink    = gst_element_factory_make ("udpsink",       NULL);
        play_ptr->a_rtcpsrc     = gst_element_factory_make ("udpsrc",        NULL);
        if (!play_ptr->audiortppay || !play_ptr->a_rtpsink || !play_ptr->a_rtcpsink || !play_ptr->a_rtcpsrc) {
          GST_ERROR ("FAILED to create audio rtp/rtcp elements");
          return VGST_ERROR_PIPELINE_CREATE_FAIL;
        }
        GST_DEBUG ("Audio rtp/rtcp elements created!");
        gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->audiortppay, play_ptr->a_rtpsink, play_ptr->a_rtcpsink, play_ptr->a_rtcpsrc, NULL);
      }
      else {
        play_ptr->audioenc      = gst_element_factory_make ("faac",          NULL);
      }
      if (!play_ptr->alsasrc || !play_ptr->alsasink || !play_ptr->audconvert || !play_ptr->audresample || !play_ptr->audcapsfilter || \
          !play_ptr->audqueue || !play_ptr->audioenc || !play_ptr->volume || !play_ptr->audcapsfilter2 || \
          !play_ptr->audresample2 || !play_ptr->audcapsfilter2) {
        GST_ERROR ("FAILED to create audio elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      } else {
        GST_DEBUG ("All audio elements are created !!!");
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->audconvert, play_ptr->audresample, play_ptr->audcapsfilter, \
                        play_ptr->audqueue, play_ptr->volume, play_ptr->audioenc, play_ptr->audconvert2, play_ptr->audresample2, \
                        play_ptr->audcapsfilter2, NULL);
      if (LIVE_SRC == ip_param->src_type) {
        GST_DEBUG ("alsasrc is added");
        gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->alsasrc, NULL);

        if (sink_type == DISPLAY && (aud_param->audio_in == AUDIO_HDMI_IN || AUDIO_SDI_IN == aud_param->audio_in || AUDIO_I2S_IN == aud_param->audio_in)) {
          GST_DEBUG ("alsasink is added");
          gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->alsasink, NULL);
        }
      }
    }
    if (!play_ptr->pipeline || !play_ptr->ip_src || !play_ptr->srccapsfilter || !play_ptr->queue || !play_ptr->enc_queue || !play_ptr->enccapsfilter || !play_ptr->deccapsfilter) {
      GST_ERROR ("FAILED to create common element");
      return VGST_ERROR_PIPELINE_CREATE_FAIL;
    } else {
      GST_DEBUG ("All common elements are created");
    }

    gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->queue, play_ptr->enc_queue,
                      play_ptr->enccapsfilter, play_ptr->deccapsfilter, NULL);

    if ((sink_type == STREAM) && ((llp2_design && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode)) {
      if(!play_ptr->rtpbin || (!play_ptr->v_rtcpsink) || (!play_ptr->v_rtcpsrc)) {
        GST_ERROR ("FAILED to create video rtp/rtcp elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      else {
        GST_DEBUG ("Video rtp/rtcp elements are created");
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->rtpbin, play_ptr->v_rtcpsink, play_ptr->v_rtcpsrc, NULL);
    }
    if (sink_type == STREAM || sink_type == RECORD) {
      if (!play_ptr->videosink)  {
        GST_ERROR ("FAILED to create video-sink element");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      } else {
        GST_DEBUG ("Video-sink element is created");
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->videosink, NULL);
    } else if (sink_type == DISPLAY) {
      if (!play_ptr->videosink || !play_ptr->fpsdisplaysink) {
        GST_ERROR ("FAILED to create display elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      } else {
        GST_DEBUG ("All display elements are created");
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->fpsdisplaysink, NULL);
    }

    if (!ip_param->raw && LIVE_SRC == ip_param->src_type) {
      if (enc_param->enc_type == AVC) {
        play_ptr->videoenc    = gst_element_factory_make (H264_ENC_NAME,    NULL);
        play_ptr->videodec    = gst_element_factory_make (H264_DEC_NAME,    NULL);
        play_ptr->videoparser = gst_element_factory_make (H264_PARSER_NAME, NULL);
      } else if (enc_param->enc_type == HEVC) {
        play_ptr->videoenc    = gst_element_factory_make (H265_ENC_NAME,    NULL);
        play_ptr->videodec    = gst_element_factory_make (H265_DEC_NAME,    NULL);
        play_ptr->videoparser = gst_element_factory_make (H265_PARSER_NAME, NULL);
      }
      if ((vlib_is_llp2_design() && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode) {
        if (enc_param->enc_type == AVC) {
          play_ptr->rtppay = gst_element_factory_make (H264_RTP_PAYLOAD_NAME,  NULL);
        } else if (enc_param->enc_type == HEVC) {
          play_ptr->rtppay = gst_element_factory_make (H265_RTP_PAYLOAD_NAME,  NULL);
        }
      } else {
        play_ptr->rtppay = gst_element_factory_make (MPEG_TS_RTP_PAYLOAD_NAME, NULL);
      }
      if (!play_ptr->videoenc || !play_ptr->videodec || !play_ptr->videoparser) {
        GST_ERROR ("FAILED to create enc-dec-parser elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      if (sink_type == STREAM && (!play_ptr->rtppay || !play_ptr->mux)) {
        GST_ERROR ("FAILED to create streaming elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      gst_bin_add_many(GST_BIN(play_ptr->pipeline), play_ptr->videoenc, play_ptr->videodec, play_ptr->videoparser, play_ptr->rtppay, play_ptr->mux, NULL);
    }
    return VGST_SUCCESS;
}

void
fetch_tag (const GstTagList * list, const gchar * tag, gpointer user_data) {
    uint br =0;
    vgst_playback *play_ptr = (vgst_playback *)user_data;
    gint i, num;

    num = gst_tag_list_get_tag_size (list, tag);
    for (i = 0; i < num; ++i) {
      const GValue *val;
      val = gst_tag_list_get_value_index (list, tag, i);
      if (G_VALUE_HOLDS_STRING (val)) {
        if (strstr(tag, "video-codec")) {
          play_ptr->video_type = strstr (g_value_get_string (val), "H.264") ? AVC : HEVC;
        } else if (strstr(tag, "audio-codec")) {
          play_ptr->video_type = 0;
        }
      } else if (G_VALUE_HOLDS_UINT (val)) {
        if (!play_ptr->file_br && play_ptr->video_type && gst_tag_list_get_uint (list, "bitrate", &br)) {
          play_ptr->file_br = br;
          GST_DEBUG ("bit rate value : %u", play_ptr->file_br);
        }
      }
    }
}


void
on_fps_measurement (GstElement *fps,
        gdouble fps_value,
        gdouble drop_rate,
        gdouble avg_rate,
        gpointer   data) {
    guint *fps_num = (guint *)data;
    *fps_num = round(fps_value);
}


void
set_property (vgst_application *app, gint index) {
    vgst_ip_params *ip_param = &app->ip_params[index];
    vgst_playback *play_ptr = &app->playback[index];
    vgst_enc_params *enc_param = &app->enc_params[index];
    vgst_cmn_params *cmn_param = app->cmn_params;
    vgst_op_params *op_param = &app->op_params[index];
    vgst_aud_params *aud_param =  &app->aud_params[index];

    gint llp2_design = vlib_is_llp2_design();
    guint dec_buffer_cnt = ceil((float)DEFAULT_DEC_BUFFER_CNT/cmn_param->num_src);
    dec_buffer_cnt = dec_buffer_cnt < MIN_DEC_BUFFER_CNT ? MIN_DEC_BUFFER_CNT : dec_buffer_cnt;
    GstCaps *srcCaps, *encCaps, *decCaps;
    if (STREAMING_SRC == ip_param->src_type) {
      GST_DEBUG ("buffer-size is %d", enc_param->bitrate * 1000);
      g_object_set (G_OBJECT(play_ptr->ip_src), "buffer-size", enc_param->bitrate * 1000, NULL);
      g_object_set (G_OBJECT(play_ptr->ip_src), "use-buffering", TRUE, NULL);
    }
    if (LIVE_SRC == ip_param->src_type) {
      g_object_set (G_OBJECT(play_ptr->ip_src), "io-mode", VGST_V4L2_IO_MODE_DMABUF_EXPORT, NULL);
      g_object_set (G_OBJECT(play_ptr->ip_src), "device", vlib_get_devname(ip_param->device_type), NULL);
      if (DP == cmn_param->driver_type && ip_param->raw) {
        g_object_set (G_OBJECT(play_ptr->ip_src), "io-mode", VGST_V4L2_IO_MODE_DMABUF_IMPORT, NULL);
      }
      if (!ip_param->raw) {
        g_object_set (G_OBJECT (play_ptr->videoenc),  "gop-length",       enc_param->gop_len,           NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "gop-mode",         enc_param->gop_mode,          NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "low-bandwidth",    enc_param->low_bandwidth,     NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "target-bitrate",   enc_param->bitrate,           NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "b-frames",         enc_param->b_frame,           NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "num-slices",       enc_param->slice,             NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "control-rate",     enc_param->rc_mode,           NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "qp-mode",          enc_param->qp_mode,           NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "prefetch-buffer",  enc_param->enable_l2Cache,    NULL);

        if (AVC == enc_param->enc_type) {
          g_object_set (G_OBJECT (play_ptr->videoenc),  "entropy-mode",   enc_param->entropy_mode,      NULL);
        }

        if ((TRUE == enc_param->max_picture_size) && ((VBR == enc_param->rc_mode) || \
                                                      (CBR == enc_param->rc_mode))) {
          guint max_picture_size = (guint)((enc_param->bitrate/cmn_param->frame_rate) * MAX_PICTURE_SIZE_MARGIN);
          GST_DEBUG ("Setting max-pictue-size to [%d]", max_picture_size);
          printf("[myles]%s: setting max-picture-size to [%d].\n", __func__, max_picture_size);
          g_object_set (G_OBJECT (play_ptr->videoenc),  "max-picture-size", max_picture_size,           NULL);
        }

        if ((llp2_design && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode) {
          g_object_set (G_OBJECT (play_ptr->videoenc),  "cpb-size",         LLP2_CPB_SIZE,              NULL);
          g_object_set (G_OBJECT (play_ptr->videoenc),  "initial-delay",    LLP2_CPB_SIZE/2,            NULL);
        }
        else {
          g_object_set (G_OBJECT (play_ptr->videoenc),  "cpb-size",         CPB_SIZE,                   NULL);
          g_object_set (G_OBJECT (play_ptr->videoenc),  "initial-delay",    CPB_SIZE/2,                 NULL);
        }

        if ((enc_param->gdr_mode != GDR_MODE_DISABLED) && (enc_param->gop_mode == LOW_DELAY_P) && \
            ((llp2_design && ip_param->enable_llp2) || (SUB_FRAME_LATENCY == enc_param->latency_mode))) {
          g_object_set (G_OBJECT (play_ptr->videoenc),  "gdr-mode",         enc_param->gdr_mode,      NULL);
          g_object_set (G_OBJECT (play_ptr->videoenc),  "periodicity-idr",  GDR_REFRESH_INTERVAL,     NULL);
        } else if (cmn_param->sink_type == STREAM) {
          g_object_set (G_OBJECT (play_ptr->videoenc),  "periodicity-idr",  enc_param->gop_len,       NULL);
        }

        if (enc_param->rc_mode == CBR) {
          printf("[myles]%s: mode is set to CBR, filler_data: %d.\n", __func__, enc_param->filler_data);
          g_object_set (G_OBJECT (play_ptr->videoenc),  "filler-data",      enc_param->filler_data,     NULL);
        }
        else if (enc_param->rc_mode == VBR) {
          guint max_bitrate = (guint)(enc_param->bitrate * MAX_BITRATE_MARGIN);
          GST_DEBUG ("Setting max-bitrate to [%d]", max_bitrate);
          g_object_set (G_OBJECT (play_ptr->videoenc),  "max-bitrate",      max_bitrate,                NULL);
        }
        if (SUB_FRAME_LATENCY == enc_param->latency_mode) {
          g_object_set (G_OBJECT (play_ptr->videodec),  "low-latency",      TRUE,                       NULL);
        }
        GST_DEBUG ("Setting internal-entropy-buffers to [%d]", dec_buffer_cnt);
        g_object_set (G_OBJECT (play_ptr->videodec),  "internal-entropy-buffers",  dec_buffer_cnt,      NULL);

        gchar * profile;
        if (ip_param->format == XV20) {
          profile = enc_param->enc_type == HEVC ? "main-422-10" : "high-4:2:2";
        } else if (ip_param->format == XV15) {
          profile = enc_param->enc_type == HEVC ? "main-10" : "high-10";
        } else if (ip_param->format == NV16) {
          profile = enc_param->enc_type == HEVC ? "main-422" : "high-4:2:2";
        } else {
          profile = enc_param->profile == BASELINE_PROFILE ? "constrained-baseline" :
                    enc_param->profile == MAIN_PROFILE ? "main" : "high";
        }

        if (AVC == enc_param->enc_type) {
          if (NORMAL_LATENCY == enc_param->latency_mode) {
            encCaps  = gst_caps_new_simple ("video/x-h264",
                                            "profile",   G_TYPE_STRING,     profile,
                                            NULL);
          } else {
            encCaps  = gst_caps_new_simple ("video/x-h264",
                                            "profile",   G_TYPE_STRING,     profile,
                                            "alignment",  G_TYPE_STRING,     "nal",
                                            NULL);
          }
        } else if (HEVC == enc_param->enc_type) {
          if (NORMAL_LATENCY == enc_param->latency_mode) {
              encCaps  = gst_caps_new_simple ("video/x-h265",
                                              "profile",   G_TYPE_STRING,     profile,
                                              NULL);
          } else {
            encCaps  = gst_caps_new_simple ("video/x-h265",
                                            "profile",     G_TYPE_STRING,     profile,
                                             "alignment",  G_TYPE_STRING,     "nal",
                                             NULL);
          }
        }
        GST_DEBUG ("new Caps for enc capsfilter %" GST_PTR_FORMAT, encCaps);
        g_object_set (G_OBJECT (play_ptr->enccapsfilter),  "caps",  encCaps, NULL);
        gst_caps_unref (encCaps);

        if (vlib_is_llp2_design() && ip_param->enable_llp2) {
          decCaps  = gst_caps_new_simple ("video/x-raw", NULL);
          gst_caps_set_features(decCaps, 0, gst_caps_features_new (GST_CAPS_FEATURE_MEMORY_XLNX_LL, NULL));
          GST_DEBUG ("new Caps for dec capsfilter %" GST_PTR_FORMAT, decCaps);
          g_object_set (G_OBJECT (play_ptr->deccapsfilter),  "caps",  decCaps, NULL);
          gst_caps_unref (decCaps);
        }
      }
    }
    gchar * format;
    format = ip_param->format == XV20 ? "NV16_10LE32" : ip_param->format == XV15 ?
                                 "NV12_10LE32" : ip_param->format == NV16 ? "NV16" : "NV12";
    srcCaps  = gst_caps_new_simple ("video/x-raw",
                                    "width",     G_TYPE_INT,        ip_param->width,
                                    "height",    G_TYPE_INT,        ip_param->height,
                                    "format",    G_TYPE_STRING,     format,
                                    "framerate", GST_TYPE_FRACTION, cmn_param->frame_rate, MAX_FRAME_RATE_DENOM,
                                    NULL);
    if (vlib_is_llp2_design() && ip_param->enable_llp2) {
      gst_caps_set_features(srcCaps, 0, gst_caps_features_new (GST_CAPS_FEATURE_MEMORY_XLNX_LL, NULL));
    }
    GST_DEBUG ("new Caps for src capsfilter %" GST_PTR_FORMAT, srcCaps);
    g_object_set (G_OBJECT (play_ptr->srccapsfilter),  "caps",  srcCaps, NULL);
    gst_caps_unref (srcCaps);

    if (cmn_param->sink_type == STREAM && ((llp2_design && ip_param->enable_llp2) || \
        SUB_FRAME_LATENCY == enc_param->latency_mode)) {
      g_object_set (G_OBJECT (play_ptr->v_rtcpsink),  "port",  op_param->port_num + 1,      NULL);
      g_object_set (G_OBJECT (play_ptr->v_rtcpsink),  "host",  op_param->host_ip,           NULL);
      g_object_set (G_OBJECT (play_ptr->v_rtcpsink),  "async", FALSE,                       NULL);
      g_object_set (G_OBJECT (play_ptr->v_rtcpsink),  "sync",  FALSE,                       NULL);
      g_object_set (G_OBJECT (play_ptr->v_rtcpsrc),   "port",  op_param->port_num + 2,      NULL);
    }

    if (aud_param->enable_audio) {
      srcCaps  = gst_caps_new_simple ("audio/x-raw",
                                      "format",    G_TYPE_STRING,     aud_param->format,
                                      "rate",      G_TYPE_INT,        aud_param->sampling_rate,
                                      "channel",   G_TYPE_INT,        aud_param->channel,
                                      NULL);
      GST_DEBUG ("new Caps for audio src capsfilter %" GST_PTR_FORMAT, srcCaps);
      g_object_set (G_OBJECT (play_ptr->audcapsfilter),  "caps",  srcCaps, NULL);
      gst_caps_unref (srcCaps);

      srcCaps  = gst_caps_new_simple ("audio/x-raw",
                                      "format",    G_TYPE_STRING,     "S24LE",
                                      "rate",      G_TYPE_INT,        aud_param->sampling_rate,
                                      "channel",   G_TYPE_INT,        aud_param->channel,
                                      NULL);
      GST_DEBUG ("new Caps for audio src capsfilter %" GST_PTR_FORMAT, srcCaps);
      g_object_set (G_OBJECT (play_ptr->audcapsfilter2),  "caps",  srcCaps, NULL);
      gst_caps_unref (srcCaps);

      if (cmn_param->sink_type == DISPLAY && aud_param->audio_out == AUDIO_DP_OUT) {
        g_object_set (G_OBJECT (play_ptr->alsasink), "device",    DP_DISPLAY_DEVICE_ID, NULL);
      } else if (cmn_param->sink_type == DISPLAY && aud_param->audio_out == AUDIO_HDMI_OUT) {
        if (llp2_design) {
          g_object_set (G_OBJECT (play_ptr->alsasink), "device", LLP2_HDMI_DISPLAY_DEVICE_ID, NULL);
        }
        else {
          g_object_set (G_OBJECT (play_ptr->alsasink), "device", HDMI_DISPLAY_DEVICE_ID,      NULL);
        }
      } else if (cmn_param->sink_type == DISPLAY && aud_param->audio_out == AUDIO_I2S_OUT) {
        g_object_set (G_OBJECT (play_ptr->alsasink), "device",    I2S_DISPLAY_DEVICE_ID, NULL);
      } else if (cmn_param->sink_type == DISPLAY && aud_param->audio_out == AUDIO_SDI_OUT) {
        g_object_set (G_OBJECT (play_ptr->alsasink), "device",    SDI_DISPLAY_DEVICE_ID, NULL);
      }

      if ((llp2_design && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode) {
          g_object_set (G_OBJECT (play_ptr->alsasink),    "latency-time",  LLP2_LATENCY_TIME, NULL);
          g_object_set (G_OBJECT (play_ptr->alsasink),    "buffer-time",   LLP2_BUFFER_TIME,  NULL);
      }

      if (aud_param->audio_in == AUDIO_HDMI_IN && LIVE_SRC == ip_param->src_type) {
        if (llp2_design) {
          g_object_set (G_OBJECT (play_ptr->alsasrc), "device", LLP2_HDMI_CAPTURE_DEVICE_ID, NULL);
        }
        else {
          g_object_set (G_OBJECT (play_ptr->alsasrc), "device", HDMI_CAPTURE_DEVICE_ID,      NULL);
        }
      } else if (aud_param->audio_in == AUDIO_SDI_IN && LIVE_SRC == ip_param->src_type) {
        g_object_set (G_OBJECT (play_ptr->alsasrc), "device",    SDI_CAPTURE_DEVICE_ID, NULL);
      } else if (aud_param->audio_in == AUDIO_I2S_IN && LIVE_SRC == ip_param->src_type) {
        g_object_set (G_OBJECT (play_ptr->alsasrc), "device",    I2S_CAPTURE_DEVICE_ID, NULL);
      }

      if (cmn_param->sink_type == STREAM && ((llp2_design && ip_param->enable_llp2) || \
          SUB_FRAME_LATENCY == enc_param->latency_mode)) {
        g_object_set (G_OBJECT (play_ptr->audiortppay), "pt",    LLP2_AUDIO_RTP_PAYLOAD_TYPE, NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtpsink),   "port",  op_param->port_num + 4,      NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtpsink),   "host",  op_param->host_ip,           NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtpsink),   "async", FALSE,                       NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtpsink),   "sync",  FALSE,                       NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtcpsink),  "port",  op_param->port_num + 5,      NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtcpsink),  "host",  op_param->host_ip,           NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtcpsink),  "async", FALSE,                       NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtcpsink),  "sync",  FALSE,                       NULL);
        g_object_set (G_OBJECT (play_ptr->a_rtcpsrc),   "port",  op_param->port_num + 6,      NULL);
      }
      g_object_set (G_OBJECT (play_ptr->volume), "volume",    aud_param->volume, NULL);
    }

    if (cmn_param->sink_type == RECORD) {
      printf("[myles]%s: recording location is set to %s.\n", __func__, op_param->file_out);
      g_object_set (G_OBJECT (play_ptr->videosink), "location",    op_param->file_out, NULL);
    } else if (cmn_param->sink_type == DISPLAY) {
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "fps-update-interval",     FPS_UPDATE_INTERVAL, NULL);
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "signal-fps-measurements", TRUE, NULL);
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "text-overlay",            FALSE, NULL);
      if (cmn_param->driver_type == DP) {
        g_object_set (G_OBJECT (play_ptr->videosink), "bus-id", cmn_param->bus_id, NULL);
      } else if (cmn_param->driver_type == HDMI_Tx) {
        g_object_set (G_OBJECT (play_ptr->videosink), "bus-id", cmn_param->bus_id, NULL);
        g_object_set (G_OBJECT (play_ptr->videosink), "plane-id",              cmn_param->plane_id, NULL);
        g_object_set (G_OBJECT (play_ptr->videosink), "hold-extra-sample", TRUE, NULL);
      } else if (cmn_param->driver_type == SDI_Tx) {
        g_object_set (G_OBJECT (play_ptr->videosink), "driver-name", SDI_TX_DRIVER_NAME, NULL);
        g_object_set (G_OBJECT (play_ptr->videosink), "hold-extra-sample", TRUE, NULL);
      }
      if (((llp2_design && ip_param->enable_llp2) || (SUB_FRAME_LATENCY == enc_param->latency_mode)) && \
            cmn_param->driver_type != DP) {
        /* special case 2-4kp30 requires 10ms max-lateness */
        if ((ip_param->width == MAX_WIDTH) && (ip_param->height == MAX_HEIGHT) && \
            (cmn_param->num_src == 2) && (cmn_param->frame_rate == MAX_SUPPORTED_FRAME_RATE/2)) {
          g_object_set (G_OBJECT (play_ptr->videosink), "max-lateness", LLP2_2_4KP30_MAX_LATENESS, NULL);
        } else {
          g_object_set (G_OBJECT (play_ptr->videosink), "max-lateness", LLP2_MAX_LATENESS, NULL);
        }
      }
      g_object_set (G_OBJECT (play_ptr->videosink), "show-preroll-frame", FALSE, NULL);
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "video-sink",         play_ptr->videosink, NULL);
      g_signal_connect (play_ptr->fpsdisplaysink,        "fps-measurements",   G_CALLBACK (on_fps_measurement), &play_ptr->fps_num[0]);
      cmn_param->plane_id++;
    } else if (cmn_param->sink_type == STREAM) {
      g_object_set (G_OBJECT (play_ptr->mux),       "alignment",    PKT_NUMBER_PER_BUFFER, NULL);
      g_object_set (G_OBJECT (play_ptr->videosink), "host",         op_param->host_ip, NULL);
      /* bitrate conversion from Kbps to bps needs multiplication of 1000 */
      g_object_set (G_OBJECT (play_ptr->videosink), "max-bitrate",  UDP_BUFFER_SIZE * 2, NULL);
      g_object_set (G_OBJECT (play_ptr->videosink), "port",         op_param->port_num, NULL);
      g_object_set (G_OBJECT (play_ptr->videosink), "qos-dscp",     QOS_DSCP_VALUE, NULL);
      g_object_set (G_OBJECT (play_ptr->videosink), "async",        FALSE, NULL);
      g_object_set (G_OBJECT (play_ptr->videosink), "buffer-size",  UDP_BUFFER_SIZE, NULL);
      g_object_set (G_OBJECT (play_ptr->videosink), "max-lateness", -1, NULL);
    }
    if (ip_param->enable_roi) {
      g_object_set (G_OBJECT (play_ptr->roi),       "capture-io-mode", VGST_V4L2_IO_MODE_DMABUF_EXPORT, NULL);
      g_object_set (G_OBJECT (play_ptr->roi),       "output-io-mode",  VGST_V4L2_IO_MODE_DMABUF_IMPORT, NULL);
      g_object_set (G_OBJECT (play_ptr->videoenc),  "qp-mode",         ROI,                             NULL);
    }
    if (((llp2_design && ip_param->enable_llp2) || (SUB_FRAME_LATENCY == enc_param->latency_mode)) && \
          play_ptr->enc_queue && cmn_param->sink_type == DISPLAY) {
      g_object_set (G_OBJECT (play_ptr->enc_queue), "max-size-buffers", 0, NULL);
    }
    if (play_ptr->queue) {
      g_object_set (G_OBJECT (play_ptr->queue), "max-size-bytes", 0, NULL);
    }
    if (play_ptr->audqueue) {
      g_object_set (G_OBJECT (play_ptr->audqueue), "max-size-bytes", 0, NULL);
    }
    if (ip_param->enable_scd && (SCD_MEMORY == ip_param->scd_type) && play_ptr->xilinxscd) {
      g_object_set (G_OBJECT (play_ptr->xilinxscd), "io-mode", VGST_V4L2_IO_MODE_DMABUF_IMPORT, NULL);
    }
}


VGST_ERROR_LOG
link_audio_pipeline (vgst_ip_params *ip_param, vgst_playback *play_ptr, gint sink_type, vgst_enc_params *enc_param) {
    gint ret = VGST_SUCCESS;
    gint llp2_design = vlib_is_llp2_design();
    gchar *videopadname = NULL, *audiopadname = NULL;
    GstPad *srcpad = NULL, *sinkpad = NULL;
    GstPadLinkReturn linkret = GST_PAD_LINK_OK;
    if (ip_param->raw == FALSE) {
      if (sink_type == DISPLAY) {
        if (llp2_design && ip_param->enable_llp2) {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue,
                                      play_ptr->videodec, play_ptr->deccapsfilter, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> deccapsfilter --> queue --> fpsdisplaysink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> deccapsfilter --> queue --> fpsdisplaysink successfully");
          }
        } else {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue,
                                      play_ptr->videodec, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink successfully");
          }
        }

        /*Audio Pipeline linking */
        if ((llp2_design && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode) {
          /* Audio pipeline for LLP1/LLP2 use-case */
          if (!gst_element_link_many (play_ptr->alsasrc, play_ptr->audcapsfilter, play_ptr->audqueue, play_ptr->alsasink, NULL)) {
            GST_ERROR ("Error linking for alsasrc --> capsfilter --> queue --> alsasink");
            ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
            goto CLEAN_UP;
          } else {
            GST_DEBUG ("Linked for alsasrc --> capsfilter --> queue --> alsasink successfully");
          }
        } else {
          if (!gst_element_link_many (play_ptr->alsasrc, play_ptr->audcapsfilter, play_ptr->audconvert, play_ptr->audresample, \
                                      play_ptr->volume, play_ptr->audcapsfilter2, play_ptr->audconvert2, play_ptr->audresample2,\
                                      play_ptr->audqueue, play_ptr->alsasink, NULL)) {
            GST_ERROR ("Error linking for alsasrc --> capsfilter --> audioconvert --> audioresample --> volume --> audcapsfilter2 --> audconvert2 --> audresample2 --> queue --> alsasink");
            ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
            goto CLEAN_UP;
          } else {
            GST_DEBUG ("Linked for alsasrc --> capsfilter --> audioconvert --> audioresample --> volume --> audcapsfilter2 --> audconvert2 --> audresample2 --> queue --> alsasink successfully");
          }
        }
      } else if (sink_type == RECORD) {
        if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                    play_ptr->videoparser, NULL)) {
          GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> videoparser");
          return VGST_ERROR_PIPELINE_LINKING_FAIL;
        } else {
          GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> videoparser successfully");
        }
        if (g_str_has_prefix (GST_ELEMENT_NAME (play_ptr->mux), MPEG_TS_MUX_NAME)) {
          play_ptr->vidpad = gst_element_get_request_pad(play_ptr->mux, "sink_%d");
          videopadname = gst_pad_get_name(play_ptr->vidpad);
          GST_DEBUG ("video Pad name %s", videopadname);
          play_ptr->audpad = gst_element_get_request_pad(play_ptr->mux, "sink_%d");
          audiopadname = gst_pad_get_name(play_ptr->audpad);
          GST_DEBUG ("audio Pad name %s", audiopadname);
        } else {
          play_ptr->vidpad = gst_element_get_request_pad(play_ptr->mux, "video_%u");
          videopadname = gst_pad_get_name(play_ptr->vidpad);
          GST_DEBUG ("video pad name %s", videopadname);
          play_ptr->audpad = gst_element_get_request_pad(play_ptr->mux, "audio_%u");
          audiopadname = gst_pad_get_name(play_ptr->audpad);
          GST_DEBUG ("audio pad name %s", audiopadname);
        }

        if (!gst_element_link_pads(play_ptr->videoparser, "src", play_ptr->mux, videopadname)) {
          GST_ERROR ("Error linking for videoparser --> mux");
          ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
          goto CLEAN_UP;
        } else {
          GST_DEBUG ("Linked for videoparser --> mux successfully");
        }
        if (!gst_element_link_many (play_ptr->alsasrc, play_ptr->audcapsfilter, play_ptr->audqueue, play_ptr->audconvert, play_ptr->audresample, play_ptr->volume, play_ptr->audioenc, NULL)) {
          GST_ERROR ("Error linking for alsasrc --> audiocapsfilter --> audioqueue --> audioconvert --> audioresample --> volume --> audioenc");
          ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
          goto CLEAN_UP;
        } else {
          GST_DEBUG ("Linked for alsasrc --> audiocapsfilter --> audioqueue --> audioconvert --> audioresample --> volume --> audioenc successfully");
        }
        if (!gst_element_link_pads(play_ptr->audioenc, "src", play_ptr->mux, audiopadname)) {
          GST_ERROR ("Error linking for audioenc --> mux");
          ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
          goto CLEAN_UP;
        } else {
          GST_DEBUG ("Linked for audioenc --> mux successfully");
        }
        if (!gst_element_link_many (play_ptr->mux, play_ptr->videosink, NULL)) {
          GST_ERROR ("Error linking for mux --> videosink");
          ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
          goto CLEAN_UP;
        } else {
          GST_DEBUG ("Linked for mux --> videosink successfully");
        }
      } else if (sink_type == STREAM) {
        /* llp1/llp2 audio-video data stream-out */
        if ((llp2_design && ip_param->enable_llp2) || SUB_FRAME_LATENCY == enc_param->latency_mode) {
          /* link llp1/llp2 video pipeline */
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                      play_ptr->enc_queue, play_ptr->rtppay, NULL)) {
            GST_ERROR ("Error linking for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> rtppay");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> rtppay successfully");
          }

          /* link llp1/llp2 audio pipeline */
          if (!gst_element_link_many (play_ptr->alsasrc, play_ptr->audconvert, play_ptr->audcapsfilter, play_ptr->audioenc, \
                                      play_ptr->audiortppay, NULL)) {
            GST_ERROR ("Error linking for alsasrc --> audconvert --> audcapsfilter --> audioenc --> audiortppay");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for alsasrc --> audconvert --> audcapsfilter --> audioenc --> audiortppay successfully");
          }

          /* Now link all to the rtpbin, start by getting an RTP sinkpad for session 0 */
          sinkpad = gst_element_get_request_pad (play_ptr->rtpbin, "send_rtp_sink_0");
          srcpad  = gst_element_get_static_pad (play_ptr->rtppay, "src");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (srcpad);

          /* get the RTP srcpad that was created when we requested the sinkpad above and
           * link it to the rtpsink sinkpad */
          srcpad  = gst_element_get_static_pad (play_ptr->rtpbin, "send_rtp_src_0");
          sinkpad = gst_element_get_static_pad (play_ptr->videosink, "sink");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (srcpad);
          gst_object_unref (sinkpad);

          /* get an RTCP srcpad for sending RTCP to the receiver */
          srcpad  = gst_element_get_request_pad (play_ptr->rtpbin, "send_rtcp_src_0");
          sinkpad = gst_element_get_static_pad (play_ptr->v_rtcpsink, "sink");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (sinkpad);

          /* we also want to receive RTCP, request an RTCP sinkpad for session 0 and
           * link it to the srcpad of the udpsrc for RTCP */
          srcpad  = gst_element_get_static_pad (play_ptr->v_rtcpsrc, "src");
          sinkpad = gst_element_get_request_pad (play_ptr->rtpbin, "recv_rtcp_sink_0");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (srcpad);

          /* now link all to the rtpbin, start by getting an RTP sinkpad for session 0 */
          sinkpad = gst_element_get_request_pad (play_ptr->rtpbin, "send_rtp_sink_1");
          srcpad  = gst_element_get_static_pad (play_ptr->audiortppay, "src");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (srcpad);

          /* get the RTP srcpad that was created when we requested the sinkpad above and
           * link it to the rtpsink sinkpad*/
          srcpad  = gst_element_get_static_pad (play_ptr->rtpbin, "send_rtp_src_1");
          sinkpad = gst_element_get_static_pad (play_ptr->a_rtpsink, "sink");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (srcpad);
          gst_object_unref (sinkpad);

          /* get an RTCP srcpad for sending RTCP to the receiver */
          srcpad  = gst_element_get_request_pad (play_ptr->rtpbin, "send_rtcp_src_1");
          sinkpad = gst_element_get_static_pad (play_ptr->a_rtcpsink, "sink");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (sinkpad);

          /* we also want to receive RTCP, request an RTCP sinkpad for session 0 and
           * link it to the srcpad of the udpsrc for RTCP */
          srcpad  = gst_element_get_static_pad (play_ptr->a_rtcpsrc, "src");
          sinkpad = gst_element_get_request_pad (play_ptr->rtpbin, "recv_rtcp_sink_1");
          linkret = gst_pad_link (srcpad, sinkpad);
          if(linkret != GST_PAD_LINK_OK) {
            GST_ERROR("Linking source and sink pad failed, error - %d", linkret);
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          }
          gst_object_unref (srcpad);
        }
        else {
          // Only supporting ts muxed data streaming-out
          play_ptr->vidpad = gst_element_get_request_pad(play_ptr->mux, "sink_%d");
          videopadname = gst_pad_get_name(play_ptr->vidpad);
          GST_DEBUG ("video pad name %s", videopadname);
          play_ptr->audpad = gst_element_get_request_pad(play_ptr->mux, "sink_%d");
          audiopadname = gst_pad_get_name(play_ptr->audpad);
          GST_DEBUG ("audio pad name %s", audiopadname);

          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue, NULL)) {
            GST_ERROR ("Error linking for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue");
            ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
            goto CLEAN_UP;
          } else {
            GST_DEBUG ("Linked for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue successfully");
          }

          if (!gst_element_link_pads(play_ptr->enc_queue, "src", play_ptr->mux, videopadname)) {
            GST_ERROR ("Error linking for enc_queue --> mux");
            ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
            goto CLEAN_UP;
          } else {
            GST_DEBUG ("Linked for enc_queue --> mux successfully");
          }
          if (!gst_element_link_many (play_ptr->alsasrc, play_ptr->audcapsfilter, play_ptr->audqueue, play_ptr->audconvert, play_ptr->audresample, play_ptr->volume, play_ptr->audioenc, NULL)) {
            GST_ERROR ("Error linking for alsasrc --> audiocapsfilter --> audioqueue --> audioconvert --> audioresample --> volume --> audioenc");
            ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
            goto CLEAN_UP;
          } else {
            GST_DEBUG ("Linked for alsasrc --> audiocapsfilter --> audioqueue --> audioconvert --> audioresample --> volume --> audioenc successfully");
          }
          if (!gst_element_link_pads(play_ptr->audioenc, "src", play_ptr->mux, audiopadname)) {
            GST_ERROR ("Error linking for audioenc --> mux");
            ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
            goto CLEAN_UP;
          } else {
            GST_DEBUG ("Linked for audioenc --> mux successfully");
          }

          if (!gst_element_link_many (play_ptr->mux, play_ptr->rtppay, play_ptr->videosink, NULL)) {
            GST_ERROR ("Error linking for mux --> rtppay --> stream_sink");
            ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for mux --> rtppay --> stream_sink successfully");
          }
        }
      }
    } else {
      // Video RAW Pipeline linking
      if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
        GST_ERROR ("Error linking for testsrc --> capsfilter --> queue --> fpsdisplaysink");
        ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
        goto CLEAN_UP;
      } else {
        GST_DEBUG ("Linked for ip_src --> capsfilter --> queue --> fpsdisplaysink successfully");
      }
      // Audio RAW Pipeline linking
      if (!gst_element_link_many (play_ptr->alsasrc, play_ptr->audcapsfilter, play_ptr->audconvert, play_ptr->audresample, \
                                  play_ptr->volume, play_ptr->audcapsfilter2, play_ptr->audconvert2, play_ptr->audresample2,\
                                  play_ptr->audqueue, play_ptr->alsasink, NULL)) {
        GST_ERROR ("Error linking for alsasrc --> capsfilter --> audioconvert --> audioresample --> volume --> audcapsfilter2 --> audconvert2 --> audresample2 --> queue --> alsasink");
        ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
        goto CLEAN_UP;
      } else {
        GST_DEBUG ("Linked for alsasrc --> capsfilter --> audioconvert --> audioresample --> volume --> audcapsfilter2 --> audconvert2 --> audresample2 --> queue --> alsasink successfully");
      }
    }
CLEAN_UP :
    if (videopadname)
      g_free (videopadname);
    if (audiopadname)
      g_free (audiopadname);

    return ret;
}


VGST_ERROR_LOG
link_elements (vgst_ip_params *ip_param, vgst_playback *play_ptr, gint sink_type, vgst_aud_params *aud_param, gchar *uri, vgst_enc_params *enc_param) {
    if (FILE_SRC == ip_param->src_type || STREAMING_SRC == ip_param->src_type) {
      g_object_set (G_OBJECT(play_ptr->ip_src), "uri", ip_param->uri, NULL);
      g_signal_connect (play_ptr->ip_src, "pad-added", G_CALLBACK (on_pad_added), play_ptr);
      g_signal_connect (GST_BIN (play_ptr->ip_src), "deep-element-added", G_CALLBACK (on_deep_element_added), NULL);
      return VGST_SUCCESS;
    }
    if (aud_param->enable_audio && (aud_param->audio_in == AUDIO_HDMI_IN || AUDIO_SDI_IN == aud_param->audio_in || AUDIO_I2S_IN == aud_param->audio_in)) {
      return link_audio_pipeline (ip_param, play_ptr, sink_type, enc_param);
    }
    if (ip_param->raw == FALSE) {
      if (sink_type == DISPLAY) {
        if (vlib_is_llp2_design() && ip_param->enable_llp2) {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue,
                                      play_ptr->videodec, play_ptr->deccapsfilter, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> deccapsfilter --> queue --> fpsdisplaysink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> deccapsfilter --> queue --> fpsdisplaysink successfully");
          }
        } else if (ip_param->enable_scd && (SCD_MEMORY == ip_param->scd_type)) {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->xilinxscd, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue,
                                      play_ptr->videodec, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> capsfilter --> xilinxscd --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> capsfilter --> xilinxscd --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink successfully");
          }
        } else if (ip_param->enable_roi) {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->roi, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue,
                                      play_ptr->videodec, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> capsfilter --> roi --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> capsfilter --> roi --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink successfully");
          }
        } else {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue,
                                      play_ptr->videodec, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink successfully");
          }
        }
      } else if (sink_type == STREAM) {
        if (vlib_is_llp2_design() && ip_param->enable_llp2) {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                      play_ptr->enc_queue, play_ptr->rtppay, play_ptr->videosink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> videosink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> videosink successfully");
            }
        } else if (ip_param->accelerator) {
          if(SUB_FRAME_LATENCY == enc_param->latency_mode) {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->bypass, play_ptr->videoenc,
                                        play_ptr->enccapsfilter, play_ptr->enc_queue, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> bypass --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> UDPsink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> bypass --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> UDPsink successfully");
            }
          } else { /* NORMAL_LATENCY == enc_param->latency_mode */
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->bypass, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                        play_ptr->enc_queue, play_ptr->mux, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> bypass --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> UDPsink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> bypass --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> UDPsink successfully");
            }
          }
        } else if (ip_param->enable_roi) {
          if(SUB_FRAME_LATENCY == enc_param->latency_mode) {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->roi, play_ptr->videoenc,
                                        play_ptr->enccapsfilter, play_ptr->enc_queue, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> roi --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> UDPsink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> roi --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> UDPsink successfully");
            }
          } else { /* NORMAL_LATENCY == enc_param->latency_mode */
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->roi, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                        play_ptr->enc_queue, play_ptr->mux, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> roi --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> UDPsink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> roi --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> UDPsink successfully");
            }
          }
        } else if (ip_param->enable_scd && (SCD_MEMORY == ip_param->scd_type)) {
          if(SUB_FRAME_LATENCY == enc_param->latency_mode) {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->xilinxscd, play_ptr->videoenc,
                                        play_ptr->enccapsfilter, play_ptr->enc_queue, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> xilinxscd --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> xilinxscd --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> videosink successfully");
            }
          } else { /* NORMAL_LATENCY == enc_param->latency_mode */
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->xilinxscd, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                        play_ptr->enc_queue, play_ptr->mux, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> xilinxscd --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> xilinxscd --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> videosink successfully");
            }
          }
        } else {
          if(SUB_FRAME_LATENCY == enc_param->latency_mode) {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                        play_ptr->enc_queue, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> rtppay --> videosink successfully");
            }
          } else { /* NORMAL_LATENCY == enc_param->latency_mode */
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                        play_ptr->enc_queue, play_ptr->mux, play_ptr->rtppay, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> enc_queue --> mpegtsmux --> rtppay --> videosink successfully");
            }
          }
        }
      } else if (sink_type == RECORD) {
        if (ip_param->enable_scd && (SCD_MEMORY == ip_param->scd_type)) {
          if (strstr(uri, TS_MUX_TYPE) != NULL) {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->xilinxscd, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                        play_ptr->mux, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> capsfilter --> xilinxscd --> videoenc --> queue --> capsfilter --> mux --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> capsfilter --> xilinxscd --> videoenc --> queue --> capsfilter --> mux --> videosink successfully");
            }
          } else {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->xilinxscd, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                        play_ptr->videoparser, play_ptr->mux, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> capsfilter --> xilinxscd --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> capsfilter --> xilinxscd --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink successfully");
            }
          }
        } else if (ip_param->enable_roi) {
          if (strstr(uri, TS_MUX_TYPE) != NULL) {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->roi, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                        play_ptr->mux, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> capsfilter --> roi --> videoenc --> queue --> capsfilter --> mux --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> capsfilter --> roi --> videoenc --> queue --> capsfilter --> mux --> videosink successfully");
            }
          } else {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->roi, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                        play_ptr->videoparser, play_ptr->mux, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> capsfilter --> roi --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> capsfilter --> roi --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink successfully");
            }
          }
        } else {
          if (strstr(uri, TS_MUX_TYPE) != NULL) {
            printf("[Myles]%s: TS_MUX_TYPE is used...\n", __func__);
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                        play_ptr->mux, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> mux --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> mux --> videosink successfully");
            }
          } else {
            if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                        play_ptr->videoparser, play_ptr->mux, play_ptr->videosink, NULL)) {
              GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink");
              return VGST_ERROR_PIPELINE_LINKING_FAIL;
            } else {
              GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink successfully");
            }
          }
        }
      }
    } else {
      if (ip_param->accelerator) {
        // It Comes here means need to use raw path means Src --> SDx-Filter --> Sink path
        if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->bypass, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
          GST_ERROR ("Error linking for testsrc --> capsfilter --> bypass --> queue --> fpsdisplaysink");
          return VGST_ERROR_PIPELINE_LINKING_FAIL;
        } else {
          GST_DEBUG ("Linked for ip_src --> capsfilter --> bypass --> queue --> fpsdisplaysink successfully");
        }
      } else {
        // It Comes here means need to use raw path means Src --> Sink path
        if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
          GST_ERROR ("Error linking for testsrc --> capsfilter --> queue --> fpsdisplaysink");
          return VGST_ERROR_PIPELINE_LINKING_FAIL;
        } else {
          GST_DEBUG ("Linked for ip_src --> capsfilter --> queue --> fpsdisplaysink successfully");
        }
      }
    }
    return VGST_SUCCESS;
}

void
get_coordinates (guint *x, guint *y, guint cnt, guint num_src) {
    if (num_src > 4) {
      if (cnt == 0) {
        *x = 0;
        *y = 0;
      } else if (cnt == 1) {
        *x = 960;
        *y = 0;
      } else if (cnt == 2) {
        *x = 1920;
        *y = 0;
      } else if (cnt == 3) {
        *x = 2880;
        *y = 0;
      } else if (cnt == 4) {
        *x = 0;
        *y = 1080;
      } else if (cnt == 5) {
        *x = 960;
        *y = 1080;
      } else if (cnt == 6) {
        *x = 1920;
        *y = 1080;
      } else if (cnt == 7) {
        *x = 2880;
        *y = 1080;
      }
    } else {
      if (cnt == 0) {
        *x = 0;
        *y = 0;
      } else if (cnt == 1) {
        *x = 1920;
        *y = 0;
      } else if (cnt == 2) {
        *x = 0;
        *y = 1080;
      } else if (cnt == 3) {
        *x = 1920;
        *y = 1080;
      }
    }
}
