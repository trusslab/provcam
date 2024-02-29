/******************************************************************************
 * (c) Copyright 2019-2020 Xilinx, Inc. All rights reserved.
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

#include <stdlib.h>
#include <mediactl/mediactl.h>
#include <mediactl/v4l2subdev.h>
#include "v4l2_subdev_helper.h"
#include "helper.h"
#include "mediactl_helper.h"
#include "vcap_scd_int.h"
#include "video_int.h"

#define MEDIA_SCD_ENTITY               "xlnx-scdchan."
#define MEDIA_SCD_PAD                  0

#define MAX_SCD_CHANNELS               8

struct vcap_scd_data {
	size_t n_channels;
	char media_scd_entity[MAX_SCD_CHANNELS][MEDIA_ENTITY_MAX_LEN];
	size_t used_channels;
};

int vcap_scd_check_configuration(const struct vlib_vdev *vdev, struct vlib_config_data *cfg)
{
	if (cfg->enable_scd) {
		if (SCD_STREAM & vdev->scd_type) {
			cfg->scd_type = SCD_STREAM;
		} else if (SCD_MEMORY & vdev->scd_type) {
			cfg->scd_type = SCD_MEMORY;
		} else {
			vlib_err("SCD = True for device %s, but it doesn't support SCD.", vdev->entity_name);
			cfg->scd_type = SCD_NONE;
			return VLIB_ERROR_SCD_INVALID_CONFIG;
		}
	} else {
		if (SCD_STREAM & vdev->scd_type) {
			cfg->scd_type = SCD_NONE;
			vlib_err("Device %s uses Stream based SCD, pipeline cannot "
					"run without SCD = True.", vdev->entity_name);
			return VLIB_ERROR_SCD_INVALID_CONFIG;
		}
	}

	return VLIB_SUCCESS;
}

int vcap_scd_reset_scd_channel(struct vlib_vdev *vdev)
{
	if (SCD == vdev->dev_type) {
		struct vcap_scd_data *data = (struct vcap_scd_data *)vdev->priv;
		data->used_channels = 0;
	} else if (SCD_MEMORY == vdev->scd_type) {
		memset(vdev->scd_media_entity, 0x00, sizeof(vdev->scd_media_entity));
	}

	return VLIB_SUCCESS;
}

int vcap_scd_alloc_scd_channel(struct vlib_vdev *vdev, struct vlib_vdev *scd_vdev)
{
	struct vcap_scd_data *data = scd_vdev->priv;

	if (data->used_channels >= data->n_channels) {
		vlib_err("Fail to get SCD channel for device %s\n", vdev->entity_name);
		return VLIB_ERROR_SCD_CH_NOT_AVAILABLE;
	}

	/* Allocate next available SCD channel */
	strcpy(vdev->scd_media_entity, data->media_scd_entity[data->used_channels++]);

	vlib_info("Allocated SCD channel \"%s\" to \"%s\",\"(%s)\"\n", vdev->scd_media_entity,
			vdev->entity_name, vdev->display_text);

	return VLIB_SUCCESS;
}

int vcap_scd_set_media_ctrl(const struct vlib_vdev *scd_vdev, const struct vlib_vdev *vdev, struct vlib_config_data *cfg)
{
	int ret;
	char fmt_str[100];
	struct media_device *media =  video_get_mdev(scd_vdev);
	const char *scd_media_entity = vdev->scd_media_entity;

	char *pixel_fmt_str = media_get_pixel_fmt_from_fmt(cfg->format);


	/* Enumerate entities, pads and links */
	ret = media_device_enumerate(media);
	if (ret < 0) {
		vlib_dbg("Failed to enumerate media: %d \n", ret);
		return VLIB_NO_MEDIA_SRC;
	}

#ifdef VLIB_LOG_LEVEL_DEBUG
	const struct media_device_info *info = media_get_info(media);
	print_media_info(info);
#endif

	if (scd_media_entity) {

		if (pixel_fmt_str) {
			media_set_fmt_str(fmt_str, (char *) scd_media_entity, 0,
					pixel_fmt_str, cfg->width_in, cfg->height_in);
		} else {
			vlib_dbg("SCD: Invalid input format\n");
			return VLIB_ERROR_SET_FORMAT_FAILED;
		}

		ret = v4l2_subdev_parse_setup_formats(media, fmt_str);
		if (ret < 0) {
			vlib_dbg("SCD: Unable to set sink format : %s (%d)\n", strerror(-ret), -ret);
			return VLIB_ERROR_SET_FORMAT_FAILED;
		}
	} else {
		vlib_dbg("SCD : Unable to find SCD entity\n");
		return VLIB_ERROR_SCD_NOT_AVAILABLE;
	}

	return ret;
}

struct vlib_vdev *vcap_scd_init(const struct matchtable *mte, void *media)
{
	struct vlib_vdev *vd = calloc(1, sizeof(*vd));
	if (!vd) {
		return NULL;
	}

	struct vcap_scd_data *data = calloc(1, sizeof(*data));
	if (!data) {
		free(vd);
		return NULL;
	}

	vd->vsrc_type = VSRC_TYPE_MEDIA;
	vd->mdev = media;
	vd->display_text = "SCD Input";
	vd->entity_name = mte->s;
	vd->priv = data;

	if (strcmp(vd->entity_name, "video_cap input 0") == 0) {
		vd->dev_type = SCD;
	}

	size_t nents = media_get_entities_count(media);
	for (size_t i = 1; i <= nents; i++) {
		const struct media_entity_desc *info;
		/* Iterating from last channel as we are currently
		 * getting channel 0 at last index */

		struct media_entity *entity = media_get_entity(media, nents - i);
		if (!entity) {
			vlib_dbg("failed to get entity %zu\n", nents - i);
			continue;
		}

		info = media_entity_get_info(entity);

		if (strstr(info->name, MEDIA_SCD_ENTITY)) {
			strcpy(data->media_scd_entity[data->n_channels++], info->name);
			vlib_info("Found SCD channel \"%s\" for \"%s\"\n", info->name, vd->entity_name);
		}
	}

	return vd;
}
