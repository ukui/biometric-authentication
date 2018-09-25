/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * Author: Droiing <jianglinxuan@kylinos.cn>
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

#include <biometric_common.h>
#include <biometric_version.h>
#include <biometric_stroge.h>
#include <biometric_version.h>

#include <libfprint/fprint.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <openssl/aes.h>
#include <fcntl.h>

#include "community_ops.h"
#include "community_define.h"

// Configure driver parameters
int community_para_config(bio_dev * dev, GKeyFile * conf);

// Driver initialization and resource recovery function
int community_ops_driver_init(bio_dev *dev);
void community_ops_free(bio_dev *dev);
int community_ops_discover(bio_dev *dev);

// Device initialization and resource recovery function
int community_ops_open(bio_dev *dev);
void community_ops_close(bio_dev *dev);

// Functional operation of device
int community_ops_enroll(bio_dev *dev, OpsActions action, int uid, int idx,
				  char * bio_idx_name);
int community_ops_verify(bio_dev *dev, OpsActions action, int uid, int idx);
int community_ops_identify(bio_dev *dev, OpsActions action, int uid,
					int idx_start, int idx_end);
feature_info * community_ops_search(bio_dev *dev, OpsActions action, int uid,
				  int idx_start, int idx_end);
int community_ops_clean(bio_dev *dev, OpsActions action, int uid,
				 int idx_start, int idx_end);
feature_info * community_ops_get_feature_list(bio_dev *dev, OpsActions action,
									   int uid, int idx_start, int idx_end);
int community_ops_feature_rename(bio_dev *dev, int uid, int idx, char * new_name);

// Framework auxiliary function
int community_ops_stop_by_user(bio_dev *dev, int waiting_ms);
const char * community_ops_get_dev_status_mesg(bio_dev *dev);
const char * community_ops_get_ops_result_mesg(bio_dev *dev);
const char * community_ops_get_notify_mid_mesg(bio_dev *dev);
void community_ops_attach(bio_dev *dev);
void community_ops_detach(bio_dev *dev);

// Device initialization and resource release related operations
int community_internal_device_init(bio_dev *dev);
void community_internal_device_free(bio_dev *dev);

// Processing fingerprint characteristic data
struct fp_print_data **community_internal_create_fp_data(bio_dev *dev,
														 feature_info * info_list);
void community_internal_free_fp_data(struct fp_print_data **fp_data_list);

// Internal implementation of the device enroll operation
int community_internal_enroll(bio_dev *dev);
static void community_internal_enroll_stage_cb(struct fp_dev *fpdev,
											   int result,
											   struct fp_print_data *print,
											   struct fp_img *img,
											   void *user_data);
int community_internal_enroll_stop(bio_dev *dev);
void community_internal_enroll_stopped_cb(struct fp_dev *fpdev,
												   void *user_data);

// Internal implementation of the device identify operation
int community_internal_identify(bio_dev *dev,
								struct fp_print_data **print_gallery);
static void community_internal_identify_cb(struct fp_dev *fpdev,
										   int result,
										   size_t match_offset,
										   struct fp_img *img,
										   void *user_data);
int community_internal_identify_stop(bio_dev *dev);
static void community_internal_identify_stopped_cb(struct fp_dev *fpdev,
												   void *user_data);

// Internal auxiliary function
void community_internal_timeout_tv_update(bio_dev *dev);
void community_internal_interactive_waiting(bio_dev *dev);

int community_internal_aes_encrypt(unsigned char *in, int len,
								   unsigned char *key, unsigned char *out);
int community_internal_aes_decrypt(unsigned char *in, int len,
								   unsigned char *key, unsigned char *out);



int community_para_config(bio_dev * dev, GKeyFile * conf)
{
	community_fpdev *cfpdev = dev->dev_priv;
	char * key_file = NULL;

	cfpdev->aes_key = NULL;
	GError * err = NULL;

	key_file = g_key_file_get_string(conf, dev->device_name, "AESKey", &err);
	if (err != NULL)
	{
		bio_print_warning(_("Get AES Key File Error[%d]: %s, use default Key.\n"),
							err->code, err->message);
		g_error_free(err);

		cfpdev->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset(cfpdev->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf((char *)cfpdev->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	if (access(key_file, F_OK | R_OK) != 0)
	{
		bio_print_warning(_("AES Key File (%s) does not Exist or has no Read "
							"Permission, use default key.\n"), key_file);

		cfpdev->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset(cfpdev->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf((char *)cfpdev->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	FILE *fp = NULL;
	int len = 0;
	int read_len = 0;

	fp = fopen(key_file, "r");
	if (fp == NULL)
	{
		bio_print_warning(_("Can not open AES Key File: %s, use default key.\n")
						  , key_file);

		cfpdev->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset(cfpdev->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf((char *)cfpdev->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);

	if (len == 0)
	{
		bio_print_warning(_("AES Key File is Enpty, use default Key.\n"));
		fclose(fp);

		cfpdev->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset(cfpdev->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf((char *)cfpdev->aes_key, "%s", DEFAULT_AES_KEY);

		return 0;
	}

	if (len > DEFAULT_AES_KEY_MAX_LEN)
		len = DEFAULT_AES_KEY_MAX_LEN;

	cfpdev->aes_key = malloc(len + 1);
	memset(cfpdev->aes_key, 0, len + 1);

	fseek(fp, 0, SEEK_SET);
	read_len = fread(cfpdev->aes_key, 1, len, fp);
	cfpdev->aes_key[read_len * 1] = 0;

	fclose(fp);

	if (cfpdev->aes_key[0] == '\0')
	{
		bio_print_warning(_("AES Key is Enpty, use default Key.\n"));

		free(cfpdev->aes_key);
		cfpdev->aes_key = malloc(strlen(DEFAULT_AES_KEY) + 1);
		memset(cfpdev->aes_key, 0, strlen(DEFAULT_AES_KEY) + 1);
		sprintf((char *)cfpdev->aes_key, "%s", DEFAULT_AES_KEY);
	}

	return 0;
}

int community_ops_driver_init(bio_dev *dev)
{
	return 0;
}

void community_ops_free(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;

	community_internal_device_free(dev);

	if (cfpdev->aes_key != NULL)
	{
		free(cfpdev->aes_key);
		cfpdev->aes_key = NULL;
	}

	free(cfpdev);
	dev->dev_priv = NULL;
}

int community_ops_discover(bio_dev *dev)
{
	int usb_num = 0;

	bio_print_info(_("Detect %s device\n"), dev->device_name);

	usb_num = community_internal_device_init(dev);
	community_internal_device_free(dev);

	// Detect USB devices
	if( usb_num < 0 )
	{
		 bio_print_info(_("No %s fingerprint device detected\n"), dev->device_name);
		 return -1;
	}
	if( usb_num == 0 )
	{
		 bio_print_info(_("No %s fingerprint device detected\n"), dev->device_name);
		 return 0;
	}

	bio_print_info(_("There is %d %s fingerprint device detected\n"),
				   usb_num, dev->device_name);

	return usb_num;
}

// Device initialization and open
int community_ops_open(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;

	int usb_num = 0;
	bio_set_dev_status(dev, DEVS_COMM_IDLE);
	bio_set_ops_result(dev, OPS_COMM_SUCCESS);
	bio_set_notify_mid(dev, NOTIFY_COMM_IDLE);

	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return -1;
	}
	bio_set_dev_status(dev, DEVS_OPEN_DOING);

	usb_num = community_internal_device_init(dev);
	if (usb_num <= 0)
	{
		snprintf(cfpdev->extra_info, EXTRA_INFO_LENGTH,
						 _("Device failed to open"));
		bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_EXTRA);
		bio_print_warning("%s\n", bio_get_notify_mid_mesg(dev));
	}

	bio_set_dev_status(dev, DEVS_COMM_IDLE);
	bio_set_ops_abs_result(dev, OPS_OPEN_SUCCESS);
	return 0;
}

// Device close and resource recovery
void community_ops_close(bio_dev *dev)
{
	community_internal_device_free(dev);
}

int community_ops_enroll(bio_dev *dev, OpsActions action, int uid, int idx,
				  char * bio_idx_name)
{
	community_fpdev *cfpdev = dev->dev_priv;
	unsigned char *plaintext = NULL;
	unsigned char *ciphertext = NULL;
	char *chara_data = NULL;
	size_t len;
	int r = 0;

	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return -1;
	}

	if (action == ACTION_START)
	{
		bio_set_dev_status(dev, DEVS_ENROLL_DOING);

		// Enroll fingerprint information and get the characteristic array
		bio_set_notify_abs_mid(dev, COMMUNITY_SAMPLE_START);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));

		r = community_internal_enroll(dev);
		if (r < 0 || cfpdev->enrolled_print == NULL)
		{
			switch (r)
			{
			case DOR_STOP_BY_USER:
				bio_set_ops_result(dev, OPS_COMM_STOP_BY_USER);
				bio_set_notify_mid(dev, NOTIFY_COMM_STOP_BY_USER);
				break;
			case DOR_TIMEOUT:
				bio_set_ops_result(dev, OPS_COMM_TIMEOUT);
				bio_set_notify_mid(dev, NOTIFY_COMM_TIMEOUT);
				break;
			case DOR_FAIL:
			default:
				bio_set_ops_result(dev, OPS_COMM_FAIL);
				snprintf(cfpdev->extra_info, EXTRA_INFO_LENGTH,
								 _("Unknown error, error code: %d"), r);
				bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_EXTRA);
				bio_print_warning("%s\n", bio_get_notify_mid_mesg(dev));
				break;
			}
			bio_set_dev_status(dev, DEVS_COMM_IDLE);

			return -1;
		}

		len = fp_print_data_get_data(cfpdev->enrolled_print, &plaintext);
		fp_print_data_free(cfpdev->enrolled_print);
		cfpdev->enrolled_print = NULL;

		// Encrypted characteristic data
		ciphertext = malloc(len);
		memset(ciphertext, 0, len);
		printf("AES Key = %s\n", cfpdev->aes_key);
		community_internal_aes_encrypt(plaintext, len, cfpdev->aes_key, ciphertext);

		// The characteristic array is converted to a string, using base64 encoding
		chara_data = malloc(len * 2);
		memset(chara_data, 0, len * 2);
		bio_base64_encode(ciphertext, chara_data, len);

		// Generate the info list corresponding to the feature
		feature_info * info;
		info = bio_sto_new_feature_info(uid, dev->bioinfo.biotype,
												dev->device_name, idx,
												bio_idx_name);
		info->sample = bio_sto_new_feature_sample(-1, NULL);
		info->sample->no = 1;
		info->sample->data = bio_sto_new_str(chara_data);

		print_feature_info(info);

		// Store the feature information in the database
		sqlite3 *db = bio_sto_connect_db();
		bio_sto_set_feature_info(db, info);
		bio_sto_disconnect_db(db);

		// Release resources
		bio_sto_free_feature_info_list(info);
		free(ciphertext);
		free(chara_data);
		free(plaintext);

		bio_set_notify_mid(dev, NOTIFY_COMM_SUCCESS);
		bio_set_ops_result(dev, OPS_COMM_SUCCESS);
		bio_set_dev_status(dev, DEVS_COMM_IDLE);

		return 0;
	} else {
		bio_set_notify_mid(dev, NOTIFY_COMM_STOP_BY_USER);
		bio_set_ops_result(dev, OPS_COMM_STOP_BY_USER);
		bio_set_dev_status(dev, DEVS_COMM_IDLE);

		return -1;
	}
}

int community_ops_verify(bio_dev *dev, OpsActions action, int uid, int idx)
{
	struct fp_print_data **fp_data_list;

	int i = 0;
	int index = -1;
	int found_uid = -1;

	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return -1;
	}

	if (action == ACTION_START)
	{
		bio_set_dev_status(dev, DEVS_VERIFY_DOING);

		// Get the feature list from the database
		sqlite3 *db = bio_sto_connect_db();
		feature_info *info_list = NULL;
		feature_info *l = NULL;
		info_list = bio_sto_get_feature_info(db, uid, dev->bioinfo.biotype,
											 dev->device_name, idx,
											 idx);
		bio_sto_disconnect_db(db);
		print_feature_info(info_list);

		// Create a community fingerprint data list
		fp_data_list = community_internal_create_fp_data(dev, info_list);

		// Identify the fingerprint using the given fingerprint list
		bio_set_notify_abs_mid(dev, COMMUNITY_SAMPLE_START);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));

		index = community_internal_identify(dev, fp_data_list);
		community_internal_free_fp_data(fp_data_list);
		if (index < 0)
		{
			switch (index)
			{
			case DOR_STOP_BY_USER:
				bio_set_ops_abs_result(dev, OPS_VERIFY_STOP_BY_USER);
				bio_set_notify_abs_mid(dev, NOTIFY_VERIFY_STOP_BY_USER);
				break;
			case DOR_TIMEOUT:
				bio_set_ops_abs_result(dev, OPS_VERIFY_TIMEOUT);
				bio_set_notify_abs_mid(dev, NOTIFY_VERIFY_TIMEOUT);
				break;
			case DOR_NO_MATCH:
				bio_set_ops_abs_result(dev, OPS_VERIFY_NO_MATCH);
				bio_set_notify_abs_mid(dev, NOTIFY_VERIFY_NO_MATCH);
				break;
			}
			bio_set_dev_status(dev, DEVS_COMM_IDLE);

			return -1;
		}

		l = info_list;
		for (i = 0; i < index; i++)
			if (l->next != NULL)
				l = l->next;

		bio_sto_free_feature_info_list(l->next);
		l->next = NULL;
		bio_print_debug(_("Find the following feature matching:\n"));
		print_feature_info(l);
		bio_sto_free_feature_info_list(info_list);

		bio_set_ops_abs_result(dev, OPS_VERIFY_MATCH);
		bio_set_notify_abs_mid(dev, NOTIFY_VERIFY_MATCH);

		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		return found_uid;
	} else {
		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_VERIFY_STOP_BY_USER);
		bio_set_notify_abs_mid(dev, NOTIFY_VERIFY_STOP_BY_USER);
		return -1;
	}
}

int community_ops_identify(bio_dev *dev, OpsActions action, int uid,
					int idx_start, int idx_end)
{
	struct fp_print_data **fp_data_list;

	int i = 0;
	int index = -1;
	int found_uid = -1;

	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return -1;
	}

	if (action == ACTION_START)
	{
		bio_set_dev_status(dev, DEVS_IDENTIFY_DOING);

		// Get the feature list from the database
		sqlite3 *db = bio_sto_connect_db();
		feature_info *info_list = NULL;
		feature_info *l = NULL;
		info_list = bio_sto_get_feature_info(db, uid, dev->bioinfo.biotype,
											 dev->device_name, idx_start,
											 idx_end);
		bio_sto_disconnect_db(db);
		print_feature_info(info_list);

		// Create a community fingerprint data list
		fp_data_list = community_internal_create_fp_data(dev, info_list);

		// Identify the fingerprint using the given fingerprint list
		bio_set_notify_abs_mid(dev, COMMUNITY_SAMPLE_START);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));

		index = community_internal_identify(dev, fp_data_list);
		community_internal_free_fp_data(fp_data_list);
		if (index < 0)
		{
			switch (index)
			{
			case DOR_STOP_BY_USER:
				bio_set_ops_abs_result(dev, OPS_IDENTIFY_STOP_BY_USER);
				bio_set_notify_abs_mid(dev, NOTIFY_IDENTIFY_STOP_BY_USER);
				break;
			case DOR_TIMEOUT:
				bio_set_ops_abs_result(dev, OPS_IDENTIFY_TIMEOUT);
				bio_set_notify_abs_mid(dev, NOTIFY_IDENTIFY_TIMEOUT);
				break;
			case DOR_NO_MATCH:
				bio_set_ops_abs_result(dev, OPS_IDENTIFY_NO_MATCH);
				bio_set_notify_abs_mid(dev, NOTIFY_IDENTIFY_NO_MATCH);
				break;
			}
			bio_set_dev_status(dev, DEVS_COMM_IDLE);

			return -1;
		}

		l = info_list;
		for (i = 0; i < index; i++)
			if (l->next != NULL)
				l = l->next;

		bio_sto_free_feature_info_list(l->next);
		l->next = NULL;
		bio_print_debug(_("Find the following feature matching:\n"));
		print_feature_info(l);
		found_uid = l->uid;
		bio_sto_free_feature_info_list(info_list);

		bio_set_ops_abs_result(dev, OPS_IDENTIFY_MATCH);
		bio_set_notify_abs_mid(dev, NOTIFY_IDENTIFY_MATCH);

		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		return found_uid;
	} else {
		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_IDENTIFY_STOP_BY_USER);
		bio_set_notify_abs_mid(dev, NOTIFY_IDENTIFY_STOP_BY_USER);
		return -1;
	}
}

feature_info * community_ops_search(bio_dev *dev, OpsActions action, int uid,
				  int idx_start, int idx_end)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct fp_print_data **fp_data_list;
	feature_info * found = NULL;
	feature_info * new_fi = NULL;
	feature_sample * sample = NULL;

	int i = 0;
	int index = -1;
	int offset = 0;
	int found_num = 0;

	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return NULL;
	}

	if (action == ACTION_START)
	{
		bio_set_dev_status(dev, DEVS_SEARCH_DOING);

		// Get the feature list from the database
		sqlite3 *db = bio_sto_connect_db();
		feature_info *info_list = NULL;
		feature_info *l = NULL;
		info_list = bio_sto_get_feature_info(db, uid, dev->bioinfo.biotype,
											 dev->device_name, idx_start,
											 idx_end);
		bio_sto_disconnect_db(db);
		print_feature_info(info_list);

		// Create a community fingerprint data list
		fp_data_list = community_internal_create_fp_data(dev, info_list);

		// Identify the fingerprint using the given fingerprint list
		bio_set_notify_abs_mid(dev, COMMUNITY_SAMPLE_START);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));

		offset = 0;
		found_num = 0;
		do {
			index = community_internal_identify(dev, &(fp_data_list[offset]));
			if (index >= 0)
			{
				l = info_list;
				sample = l->sample;
				for (i = 0; i < index + offset; i++)
				{
					if (sample->next != NULL)
						sample = sample->next;
					else
						if (l->next != NULL)
						{
							l = l->next;
							sample = l->sample;
						}
				}

				new_fi = bio_sto_new_feature_info(l->uid, l->biotype, l->driver,
												 l->index, l->index_name);
				new_fi->sample = bio_sto_new_feature_sample(sample->no, sample->data);
				new_fi->next = NULL;
				new_fi->sample->next = NULL;

				bio_print_debug(_("Search from offset %d, index %d has been "
								  "searched, global index %d(%d + %d)\n"),
								offset, index, index + offset, offset, index);
				found_num++;
				snprintf(cfpdev->extra_info, EXTRA_INFO_LENGTH,
						 _("The %d feature has been searched(UID = %d, Index = "
						   "%d, Index Name = %s), please press your finger "
						   "to continue the search"),
						 found_num, new_fi->uid, new_fi->index,
						 new_fi->index_name);
				bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_EXTRA);
				bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));

				if (found == NULL)
					found = new_fi;
				else
				{
					l = found;
					while (l->next != NULL)
						l = l->next;
					l->next = new_fi;
				}

				offset += index + 1;
			}
		} while (index >= 0 && fp_data_list[offset] != NULL);
		community_internal_free_fp_data(fp_data_list);

		if (found == NULL)
		{
			switch (index)
			{
			case DOR_STOP_BY_USER:
				bio_set_ops_abs_result(dev, OPS_SEARCH_STOP_BY_USER);
				bio_set_notify_abs_mid(dev, NOTIFY_SEARCH_STOP_BY_USER);
				break;
			case DOR_TIMEOUT:
				bio_set_ops_abs_result(dev, OPS_SEARCH_TIMEOUT);
				bio_set_notify_abs_mid(dev, NOTIFY_SEARCH_TIMEOUT);
				break;
			case DOR_NO_MATCH:
				bio_set_ops_abs_result(dev, OPS_SEARCH_NO_MATCH);
				bio_set_notify_abs_mid(dev, NOTIFY_SEARCH_NO_MATCH);
				break;
			}
			bio_set_dev_status(dev, DEVS_COMM_IDLE);

			return NULL;
		}

		bio_sto_free_feature_info_list(info_list);
		bio_print_debug(_("Find the following feature matching:\n"));
		print_feature_info(found);

		bio_set_ops_abs_result(dev, OPS_SEARCH_MATCH);
		bio_set_notify_abs_mid(dev, NOTIFY_SEARCH_MATCH);

		bio_set_dev_status(dev, DEVS_COMM_IDLE);

		return found;
	}
	else
	{
		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_SEARCH_STOP_BY_USER);
		bio_set_notify_abs_mid(dev, NOTIFY_SEARCH_STOP_BY_USER);
		return NULL;
	}
}

int community_ops_clean(bio_dev *dev, OpsActions action, int uid,
				 int idx_start, int idx_end)
{
	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return 0;
	}

	if (action == ACTION_START) {
		bio_set_dev_status(dev, DEVS_CLEAN_DOING);

		sqlite3 * db;
		db = bio_sto_connect_db();
		int ret = 0;
		ret = bio_sto_clean_feature_info(db, uid, dev->bioinfo.biotype,
												 dev->device_name, idx_start,
												 idx_end);
		bio_sto_disconnect_db(db);
		if (ret == 0) {
			bio_set_ops_abs_result(dev, OPS_CLEAN_SUCCESS);
			bio_set_notify_abs_mid(dev, NOTIFY_CLEAN_SUCCESS);
		} else {
			bio_set_ops_result(dev, OPS_CLEAN_FAIL);
			bio_set_notify_abs_mid(dev, NOTIFY_CLEAN_FAIL);
		}

		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		return ret;
	} else {
		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_CLEAN_STOP_BY_USER);
		bio_set_notify_abs_mid(dev, NOTIFY_CLEAN_STOP_BY_USER);
		return 0;
	}
}

feature_info * community_ops_get_feature_list(bio_dev *dev, OpsActions action,
									   int uid, int idx_start, int idx_end)
{
	if (dev->enable == FALSE) {
		bio_set_dev_status(dev, DEVS_COMM_DISABLE);
		return NULL;
	}

	if (action == ACTION_START) {
		feature_info * finfo_list = NULL;
		sqlite3 *db = bio_sto_connect_db();

		bio_set_dev_status(dev, DEVS_GET_FLIST_DOING);

		finfo_list = bio_sto_get_feature_info(db, uid, dev->bioinfo.biotype,
													 dev->device_name, idx_start,
													 idx_end);
		print_feature_info(finfo_list);
		bio_sto_disconnect_db(db);

		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_GET_FLIST_SUCCESS);
		bio_set_notify_abs_mid(dev, NOTIFY_GET_FLIST_SUCCESS);

		return finfo_list;
	} else {
		bio_set_dev_status(dev, DEVS_COMM_IDLE);
		bio_set_ops_abs_result(dev, OPS_GET_FLIST_STOP_BY_USER);
		bio_set_notify_abs_mid(dev, NOTIFY_GET_FLIST_STOP_BY_USER);

		return NULL;
	}
}

// Framework auxiliary function
int community_ops_stop_by_user(bio_dev *dev, int waiting_ms)
{
	community_fpdev *cfpdev = dev->dev_priv;
	int timeout = bio_get_ops_timeout_ms();
	int timeused = 0;
	int dev_status = bio_get_dev_status(dev);
	int ops_type = dev_status / 100;

	bio_print_info(_("Device %s[%d] received interrupt request\n"),
				   dev->device_name, dev->driver_id);

	if (waiting_ms < timeout)
		timeout = waiting_ms;

	if (bio_get_dev_status(dev) % 100 != DEVS_COMM_IDLE)
	{
		bio_set_dev_status(dev, ops_type * 100 + DEVS_COMM_STOP_BY_USER);

		cfpdev->ops_result = DOR_STOP_BY_USER;
		cfpdev->ops_done = true;
	}

	while ((bio_get_dev_status(dev) % 100 != DEVS_COMM_IDLE) &&
		   (timeused < timeout)){
		timeused += OPS_DETECTION_INTERVAL_MS;
		usleep(OPS_DETECTION_INTERVAL_MS * 1000);
	}

	if (bio_get_dev_status(dev) % 100 == DEVS_COMM_IDLE)
		return 0;

	// Interrupt failed, restore operation status
	bio_set_dev_status(dev, dev_status);
	return -1;
}

const char * community_ops_get_dev_status_mesg(bio_dev *dev)
{
	return NULL;
}

const char * community_ops_get_ops_result_mesg(bio_dev *dev)
{
	return NULL;
}

const char * community_ops_get_notify_mid_mesg(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;

	switch (bio_get_notify_mid(dev))
	{
	case COMMUNITY_ENROLL_COMPLETE:
		return _("Sample complete");
	case COMMUNITY_ENROLL_FAIL:
		return _("Enrollment failed due to incomprehensible data. "
				 "(Please use the same finger at different sampling "
				 "stages of the same enroll)");
	case COMMUNITY_COMMON_RETRY:
		return _("Please place your finger again because of poor "
				 "quality of the sample or other scanning problems");
	case COMMUNITY_COMMON_RETRY_TOO_SHORT:
		return _("Your swipe was too short, please place your "
				 "finger again.");
	case COMMUNITY_COMMON_RETRY_CENTER_FINGER:
		return _("Didn't catch that, please center your finger on the "
				 "sensor and try again.");
	case COMMUNITY_COMMON_RETRY_REMOVE_FINGER:
		return _("Because of the scanning image quality or finger "
				 "pressure problem, the sampling failed, please remove "
				 "the finger and retry");
	case COMMUNITY_ENROLL_EMPTY:
		return _("Unable to generate feature data, enroll failure");
	case COMMUNITY_SAMPLE_START:
		return _("Sample start, please press and lift your finger"
				 " (Some devices need to swipe your finger)");

	case COMMUNITY_ENROLL_EXTRA:
		return cfpdev->extra_info;
	default:
		return NULL;
	}
}

void community_ops_attach(bio_dev *dev)
{

}

void community_ops_detach(bio_dev *dev)
{

}

// Drives internal functions

/**
 * Detect and open the device
 * The return value：
 *	>= 0, The number of devices found
 *	< 0, Error in processing
 */
int community_internal_device_init(bio_dev *dev)
{
	struct fp_dscv_dev **discovered_devs = NULL;
	struct fp_dscv_dev *ddev;
	struct fp_driver *drv;
	struct fp_dev *community_dev = NULL;
	community_fpdev *cfpdev = dev->dev_priv;
	int usbNum = 0;
	int i = 0;

	fp_init();

	discovered_devs = fp_discover_devs();

	for (i = 0; discovered_devs[i] != NULL; i++)
	{
		int drv_id = -1;
		ddev = discovered_devs[i];

		drv = fp_dscv_dev_get_driver(ddev);
		drv_id = fp_driver_get_driver_id(drv);

		if (drv_id == cfpdev->community_driver_id)
		{
			community_dev = fp_dev_open(ddev);
			if (!community_dev)
			{
				bio_print_warning(_("Could not open device (driver %s)"),
								  fp_driver_get_full_name(drv));
				continue;
			}

			usbNum++;
		}
	}
	fp_dscv_devs_free(discovered_devs);

	if (usbNum > 0)
	{
		cfpdev->dev = community_dev;

		cfpdev->ops_result = DOR_NO_MATCH;
		cfpdev->ops_done = true;
		cfpdev->ops_stopped = true;

		cfpdev->ops_timeout_ms = bio_get_ops_timeout_ms();
		cfpdev->ops_detection_interval_tv.tv_sec = 0;
		cfpdev->ops_detection_interval_tv.tv_usec = OPS_DETECTION_INTERVAL_MS * 1000;

		cfpdev->enroll_times = fp_dev_get_nr_enroll_stages(community_dev);
		cfpdev->sample_times = 0;
		cfpdev->enrolled_print = NULL;
	}

	return usbNum;
}

void community_internal_device_free(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct fp_dev *community_dev = cfpdev->dev;
	fp_dev_close(community_dev);

	cfpdev->dev = NULL;

	cfpdev->ops_result = DOR_NO_MATCH;
	cfpdev->ops_done = true;
	cfpdev->ops_stopped = true;

	if (cfpdev->enrolled_print != NULL)
		fp_print_data_free(cfpdev->enrolled_print);

	fp_exit();
}

/**
 * The fp_print_data list is generated from the feature_info list and returns
 * the fp_print_data list
 * Parameter:
 *	feature_info * info_list	Feature list
 * Return:
 *	fp_print_data **	The fp_print_data list address, ending with NULL
 * Note:
 *	The fp_print_data list needs to be freed using the
 *	community_internal_free_fp_data function
 */
struct fp_print_data **community_internal_create_fp_data(bio_dev *dev,
														 feature_info * info_list)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct fp_print_data **fp_data_list;
	feature_info * l = info_list;
	int len = 0;
	int i = 0;
	int fp_num = 0;

	// Gets the length of the feature list
	while (l != NULL)
	{
		feature_sample *sample = l->sample;
		while (sample != NULL)
		{
			fp_num++;
			sample = sample->next;
		}
		l = l->next;
	}

	// Traverse feature lists, decode feature
	fp_data_list = malloc((fp_num + 1) * sizeof(struct fp_print_data **));
	memset(fp_data_list, 0, (fp_num + 1) * sizeof(struct fp_print_data **));
	l = info_list;
	i = 0;
	while (l != NULL)
	{
		feature_sample *sample = l->sample;
		while (sample != NULL)
		{
			// Decoding feature
			len = strlen(sample->data);
			unsigned char * ciphertext = malloc(len);
			memset(ciphertext, 0, len);
			len = bio_base64_decode(sample->data, ciphertext);

			// Decrypted characteristic data
			unsigned char * plaintext = malloc(len);
			memset(plaintext, 0, len);
			community_internal_aes_decrypt(ciphertext, len, cfpdev->aes_key, plaintext);
			printf("AES Key = %s\n", cfpdev->aes_key);

			// Generate community fingerprint feature data structure
			fp_data_list[i] = fp_print_data_from_data(plaintext, len);

			// Release resources
			free(ciphertext);
			free(plaintext);

			sample = sample->next;
			i++;
		}
		l = l->next;
	}

	return fp_data_list;
}

/**
 * Release the fp_data_list list
 * Parameter:
 *	fp_print_data **fp_data_list	The fp_print_data list address
 */
void community_internal_free_fp_data(struct fp_print_data **fp_data_list)
{
	struct fp_print_data * fp_data;
	int i = 0;

	fp_data = fp_data_list[i];
	while (fp_data)
	{
		fp_print_data_free(fp_data);
		i++;
		fp_data = fp_data_list[i];
	}
	free(fp_data_list);
}

/**
 * Enroll operation main processing function
 * Parameter:
 *	bio_dev *dev	Biometric device structure
 * Return:
 *	0	Enroll success
 *	-1	Enroll failure
 *	-2	Operation timeout
 *	-3	The operation is stop by the user
 *
 * Note
 *	The return value needs to be manually released
 */
int community_internal_enroll(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct fp_dev *community_dev = cfpdev->dev;
	int r;

	cfpdev->ops_done = false;
	if (cfpdev->enrolled_print != NULL)
	{
		fp_print_data_free(cfpdev->enrolled_print);
		cfpdev->enrolled_print = NULL;
	}

	cfpdev->sample_times = 1;
	r = fp_async_enroll_start(community_dev,
							  community_internal_enroll_stage_cb, dev);
	if (r < 0)
	{
		bio_print_error(_("Failed to call function %s\n"),
						"community_internal_enroll");
		return DOR_FAIL;
	}

	// Wait for the interaction to complete or timeout or be stop by the user
	community_internal_interactive_waiting(dev);

	// Stop the enroll operation
	community_internal_enroll_stop(dev);

	return cfpdev->ops_result;
}

static void community_internal_enroll_stage_cb(struct fp_dev *fpdev,
											   int result,
											   struct fp_print_data *print,
											   struct fp_img *img,
											   void *user_data)
{
	bio_dev *dev = user_data;
	community_fpdev *cfpdev = dev->dev_priv;

	if (result < 0) {
		snprintf(cfpdev->extra_info, EXTRA_INFO_LENGTH,
				 _("Unknown error, error code: %d"), result);
		bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_EXTRA);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));

		cfpdev->ops_done = true;
		cfpdev->ops_result = DOR_FAIL;
		community_internal_enroll_stop(dev);

		return;
	}

	switch (result)
	{
	case FP_ENROLL_COMPLETE:
		bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_COMPLETE);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		cfpdev->ops_result = 0;

		if (print)
			cfpdev->enrolled_print = print;
		else
		{
			snprintf(cfpdev->extra_info, EXTRA_INFO_LENGTH,
					 _("Enroll failed: The feature was successfully sampled, "
					   "but the encoding of the sampling feature could "
					   "not be generated"));
			bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_EXTRA);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));

			cfpdev->enrolled_print = NULL;
			cfpdev->ops_result = DOR_FAIL;
		}

		break;
	case FP_ENROLL_FAIL:
		bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_FAIL);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		break;
	case FP_ENROLL_PASS:
		snprintf(cfpdev->extra_info, EXTRA_INFO_LENGTH,
				 _("The %d [%d/%d] sampling was successful, in the next "
				   "sampling: please press and lift your finger (Some "
				   "devices need to swipe your finger)"),
				 cfpdev->sample_times, cfpdev->sample_times, cfpdev->enroll_times);
		bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_EXTRA);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		cfpdev->sample_times++;

		// Update timeout
		community_internal_timeout_tv_update(dev);
		break;
	case FP_ENROLL_RETRY:
		bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		break;
	case FP_ENROLL_RETRY_TOO_SHORT:
		bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY_TOO_SHORT);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		break;
	case FP_ENROLL_RETRY_CENTER_FINGER:
		bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY_CENTER_FINGER);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		break;
	case FP_ENROLL_RETRY_REMOVE_FINGER:
		bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY_REMOVE_FINGER);
		bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
		break;
	}

	if (result != FP_ENROLL_PASS && result != FP_ENROLL_COMPLETE)
		cfpdev->ops_result = DOR_FAIL;

	if (result != FP_ENROLL_PASS)
	{
		community_internal_enroll_stop(dev);
		cfpdev->ops_done = true;
	}
}

int community_internal_enroll_stop(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct fp_dev *community_dev = cfpdev->dev;

	cfpdev->ops_stopped = false;
	fp_async_enroll_stop(community_dev,
						   community_internal_enroll_stopped_cb, dev);

	// Using the wait function will block unaccountably, so wait is not
	// considered for the time being...
#if 0
	// Wait for the stop operation to complete
	int r = 0;
	while (!cfpdev->ops_stopped)
	{
//		r = fp_handle_events_timeout(&(cfpdev->ops_detection_interval_tv));
//		r = fp_handle_events();
		if (r < 0)
			cfpdev->ops_stopped = true;
	}
#else
	cfpdev->ops_stopped = true;
#endif

	return 0;
}

void community_internal_enroll_stopped_cb(struct fp_dev *fpdev,
												   void *user_data)
{
	bio_dev *dev = user_data;
	community_fpdev *cfpdev = dev->dev_priv;

	cfpdev->ops_stopped = true;
}

/**
 * Scans the fingerprint and compares it with the given fingerprint data
 * table to return the index of the matched table items
 * Parameter:
 *	bio_dev *dev	Community fingerprint device structure
 *	struct fp_print_data ** Fingerprint data list
 * Return:
 *	-1	No match
 *	-2	Operation timeout
 *	-3	The operation is stop by the user
 *	>=0	Matches the index of list items
 */
int community_internal_identify(bio_dev *dev,
								struct fp_print_data **print_gallery)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct fp_dev *community_dev = cfpdev->dev;
	int r;

	cfpdev->ops_done = false;
	r = fp_async_identify_start(community_dev, print_gallery,
								community_internal_identify_cb, dev);
	if (r < 0)
	{
		bio_print_error(_("Failed to call function %s\n"),
						"community_internal_enroll");
		return DOR_NO_MATCH;
	}

	struct timeval time_now;
	gettimeofday(&time_now, NULL);
	cfpdev->ops_timeout_tv.tv_sec = time_now.tv_sec + cfpdev->ops_timeout_ms / 1000;
	cfpdev->ops_timeout_tv.tv_usec = time_now.tv_usec;

	while (!cfpdev->ops_done)
	{
		r = fp_handle_events_timeout(&(cfpdev->ops_detection_interval_tv));
		if (r < 0)
			cfpdev->ops_done = true;

		gettimeofday(&time_now, NULL);

		if (time_now.tv_sec > cfpdev->ops_timeout_tv.tv_sec)
		{
			cfpdev->ops_result = DOR_TIMEOUT;
			cfpdev->ops_done = true;

		} else
			if (time_now.tv_sec == cfpdev->ops_timeout_tv.tv_sec)
				if (time_now.tv_usec >= cfpdev->ops_timeout_tv.tv_usec)
				{
					cfpdev->ops_result = DOR_TIMEOUT;
					cfpdev->ops_done = true;
				}
	}

	community_internal_identify_stop(dev);

	return cfpdev->ops_result;
}

void community_internal_identify_cb(struct fp_dev *fpdev,
										   int result,
										   size_t match_offset,
										   struct fp_img *img,
										   void *user_data)
{
	bio_dev *dev = user_data;
	community_fpdev *cfpdev = dev->dev_priv;

	if (result < 0) {
		snprintf(cfpdev->extra_info, EXTRA_INFO_LENGTH,
				 _("Unknown error, error code: %d"), result);
		bio_set_notify_abs_mid(dev, COMMUNITY_ENROLL_EXTRA);
		bio_print_warning("%s\n", bio_get_notify_mid_mesg(dev));
		result = FP_VERIFY_NO_MATCH;
	}
	else
	{
		switch (result) {
		case FP_VERIFY_NO_MATCH:
			bio_set_notify_mid(dev, NOTIFY_COMM_NO_MATCH);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
			break;
		case FP_VERIFY_MATCH:
			bio_set_notify_mid(dev, NOTIFY_COMM_SUCCESS);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
			break;
		case FP_VERIFY_RETRY:
			bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
			break;
		case FP_VERIFY_RETRY_TOO_SHORT:
			bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY_TOO_SHORT);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
			break;
		case FP_VERIFY_RETRY_CENTER_FINGER:
			bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY_CENTER_FINGER);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
			break;
		case FP_VERIFY_RETRY_REMOVE_FINGER:
			bio_set_notify_abs_mid(dev, COMMUNITY_COMMON_RETRY_REMOVE_FINGER);
			bio_print_debug("%s\n", bio_get_notify_mid_mesg(dev));
			break;
		}
	}

	if (result == FP_VERIFY_MATCH)
		cfpdev->ops_result = match_offset;
	else
		cfpdev->ops_result = DOR_NO_MATCH;
	cfpdev->ops_done = true;

//	fp_async_identify_stop(fpdev,
//						   community_internal_identify_stopped_cb, dev);
}

int community_internal_identify_stop(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct fp_dev *community_dev = cfpdev->dev;
	int r = 0;

	cfpdev->ops_stopped = false;
	fp_async_identify_stop(community_dev,
						   community_internal_identify_stopped_cb, dev);

	// Wait for the stop operation to complete
	while (!cfpdev->ops_stopped)
	{
		r = fp_handle_events_timeout(&(cfpdev->ops_detection_interval_tv));
		if (r < 0)
			cfpdev->ops_stopped = true;
	}

	return 0;
}

static void community_internal_identify_stopped_cb(struct fp_dev *fpdev,
												   void *user_data)
{
	bio_dev *dev = user_data;
	community_fpdev *cfpdev = dev->dev_priv;

	cfpdev->ops_stopped = true;
}

void community_internal_timeout_tv_update(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct timeval time_now;

	gettimeofday(&time_now, NULL);
	cfpdev->ops_timeout_tv.tv_sec = time_now.tv_sec + cfpdev->ops_timeout_ms / 1000;
	cfpdev->ops_timeout_tv.tv_usec = time_now.tv_usec;
}

/**
 * Interactive wait function. Due to the particularity of libfprint, sleep
 * waiting cannot be used directly, so it is necessary for this function
 * to detect the interaction state and wait.
 */
void community_internal_interactive_waiting(bio_dev *dev)
{
	community_fpdev *cfpdev = dev->dev_priv;
	struct timeval time_now;
	int r = 0;

	gettimeofday(&time_now, NULL);
	cfpdev->ops_timeout_tv.tv_sec = time_now.tv_sec + cfpdev->ops_timeout_ms / 1000;
	cfpdev->ops_timeout_tv.tv_usec = time_now.tv_usec;

	/*
	 * Exit condition: cfpdev->ops_done == true, Operation is completed
	 *
	 * Possible situations:
	 *	1. The operation is complete. The relevant callback function completed
	 *     by the operation modifies the value of cfpdev->ops_done;
	 *	2. The operation is stop by the user. Modify the value of
	 *     cfpdev->ops_done from the stop function;
	 *	3. Operation timeout. Modify the value of cfpdev->ops_done by the
	 *     following code;
	 */
	while (!cfpdev->ops_done)
	{
		r = fp_handle_events_timeout(&(cfpdev->ops_detection_interval_tv));
		if (r < 0)
			cfpdev->ops_done = true;

		gettimeofday(&time_now, NULL);

		if (time_now.tv_sec > cfpdev->ops_timeout_tv.tv_sec)
		{
			cfpdev->ops_result = DOR_TIMEOUT;
			cfpdev->ops_done = true;

		} else
			if (time_now.tv_sec == cfpdev->ops_timeout_tv.tv_sec)
				if (time_now.tv_usec >= cfpdev->ops_timeout_tv.tv_usec)
				{
					cfpdev->ops_result = DOR_TIMEOUT;
					cfpdev->ops_done = true;
				}
	}
}

int community_internal_aes_encrypt(unsigned char *in, int len,
								   unsigned char *key, unsigned char *out)
{
	if (!in || !key || !out)
		return -1;

	AES_KEY aes;
	unsigned char iv[AES_BLOCK_SIZE] = {0};		// 初始偏移向量IV
	int i = 0;
	int num = 0;

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		iv[i] = i;

	if (AES_set_encrypt_key((unsigned char*) key, 128, &aes) < 0)
	{
		return -2;
	}

	AES_cfb128_encrypt((unsigned char*) in, (unsigned char*) out, len,
					&aes, iv, &num, AES_ENCRYPT);

	return 0;
}
int community_internal_aes_decrypt(unsigned char *in, int len,
								   unsigned char *key, unsigned char *out)
{
	if (!in || !key || !out)
		return -1;

	AES_KEY aes;
	unsigned char iv[AES_BLOCK_SIZE] = {0};		// 初始偏移向量IV
	int i = 0;
	int num = 0;

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		iv[i] = i;

	if(AES_set_encrypt_key((unsigned char*) key, 128, &aes) < 0)
	{
		return -2;
	}

	AES_cfb128_encrypt((unsigned char*) in, (unsigned char*) out, len,
						&aes, iv, &num, AES_DECRYPT);
	return 0;
}
