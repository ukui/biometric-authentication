/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * Author: Droiing <jianglinxuan@kylinos.cn>
 *         chenziyi <chenziyi@kylinos.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef DRIVE_REALIZATION_H
#define DRIVE_REALIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "driver_type.h"
#include "driver_internal.h"
#include "close.h"
#include "aes_128_cfb.h"
#include "community_define.h"

#include <biometric_common.h>
#include <biometric_storage.h>
#include <biometric_version.h>

#include <libfprint-2/fprint.h>

int community_ops_driver_init(bio_dev *dev);

void community_ops_free(bio_dev *dev);

int community_ops_discover(bio_dev *dev);

int community_ops_open(bio_dev *dev);

void community_ops_close(bio_dev *dev);

char *community_ops_capture(bio_dev *dev, OpsActions action);

int community_ops_enroll(bio_dev *dev, OpsActions action, int uid, int idx, char * bio_idx_name);

int community_ops_verify(bio_dev *dev, OpsActions action, int uid, int idx);

int community_ops_identify(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end);

feature_info *community_ops_search(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end);

int community_ops_clean(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end);

feature_info *community_ops_get_feature_list(bio_dev *dev, OpsActions action, int uid, int idx_start, int idx_end);

int community_ops_stop_by_user(bio_dev *dev, int waiting_ms);

const char *community_ops_get_dev_status_mesg(bio_dev *dev);

const char *community_ops_get_ops_result_mesg(bio_dev *dev);

const char *community_ops_get_notify_mid_mesg(bio_dev *dev);

void *community_ops_attach(bio_dev *dev);

void *community_ops_detach(bio_dev *dev);

#endif // DRIVE_REALIZATION_H
