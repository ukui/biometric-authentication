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

#include <stdio.h>
#include <glib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <polkit/polkit.h>
#include <glib/gi18n.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <biometric_common.h>
#include <biometric_version.h>
#include "biometric-dbus-common.h"
#include "biometric-authenticationd.h"
#include "biometric-generated.h"
#include "dbus_comm.h"

static GMainLoop *pLoop = NULL;
static Biometric *pSkeleton = NULL;
static PolkitAuthority *pAuthority = NULL;

char* mesg_nodev;
char* mesg_devbusy;

struct timeval timestamp_dev_status[MAX_DEVICES] = {0};
struct timeval timestamp_ops_result[MAX_DEVICES] = {0};
struct timeval timestamp_notify_mid[MAX_DEVICES] = {0};

static gboolean gdbus_check_app_api_version(Biometric *object,
											GDBusMethodInvocation *invocation,
											int major, int minor, int function)
{
	int ret = 0;

	ret = bio_check_app_api_version(major, minor, function);
	biometric_complete_check_app_api_version(object, invocation, ret);

	return true;
}

bio_dev * get_dev_by_drvid(int drvid)
{
	GList * l = bio_get_dev_list();
	bio_dev * dev = NULL;

	if (l == NULL) {
		return NULL;
	}

	while (l != NULL) {
		dev = l->data;

		if (dev->driver_id == drvid) {
			break;
		}

		l = l->next;
	}

	if (dev->driver_id == drvid)
		return dev;
	else
		return NULL;
}

static gboolean gdbus_get_drv_list(Biometric *object,
								   GDBusMethodInvocation *invocation)
{
	int count = 0;
	GList * l = bio_get_drv_list();
	GVariant * sta;
	GVariantBuilder* _builder = g_variant_builder_new(G_VARIANT_TYPE("av"));

	while (l != NULL) {
		bio_dev * dev = l->data;

		GVariant * _item = g_variant_new("(issiiiiiiiiii)",
										 dev->driver_id,
										 dev->device_name,
										 dev->full_name,
										 dev->enable,
										 dev->dev_num,
										 dev->bioinfo.biotype,
										 dev->bioinfo.stotype,
										 dev->bioinfo.eigtype,
										 dev->bioinfo.vertype,
										 dev->bioinfo.idtype,
										 dev->bioinfo.bustype,
										 bio_get_dev_status(dev),
										 bio_get_ops_result(dev),
										 bio_get_notify_mid(dev));
		g_variant_builder_add(_builder, "v", _item);

		l = l->next;
		count += 1;
	}

	sta = g_variant_builder_end(_builder);
	g_variant_builder_unref(_builder);

	biometric_complete_get_drv_list(object, invocation, count, sta);

	return true;
}

static gboolean gdbus_get_dev_list(Biometric *object,
								   GDBusMethodInvocation *invocation)
{
	int count = 0;
	GList * l = bio_get_dev_list();
	GVariant * sta;
	GVariantBuilder* _builder = g_variant_builder_new(G_VARIANT_TYPE("av"));

	while (l != NULL) {
		bio_dev * dev = l->data;

		GVariant * _item = g_variant_new("(issiiiiiiiiii)",
										 dev->driver_id,
										 dev->device_name,
										 dev->full_name,
										 dev->enable,
										 dev->dev_num,
										 dev->bioinfo.biotype,
										 dev->bioinfo.stotype,
										 dev->bioinfo.eigtype,
										 dev->bioinfo.vertype,
										 dev->bioinfo.idtype,
										 dev->bioinfo.bustype,
										 bio_get_dev_status(dev),
										 bio_get_ops_result(dev),
										 bio_get_notify_mid(dev));
		g_variant_builder_add(_builder, "v", _item);

		l = l->next;
		count += 1;
	}

	sta = g_variant_builder_end(_builder);
	g_variant_builder_unref(_builder);

	biometric_complete_get_dev_list(object, invocation, count, sta);

	return true;
}

void gdbus_get_feature_list_pt(GSList * argv) {
	int count = 0;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = g_slist_nth_data(argv, 0);
	p1 = g_slist_nth_data(argv, 1);
	p2 = g_slist_nth_data(argv, 2);
	p3 = g_slist_nth_data(argv, 3);
	p4 = g_slist_nth_data(argv, 4);
	p5 = g_slist_nth_data(argv, 5);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int uid = (int)*p3;
	int idx_start = (int)*p4;
	int idx_end = (int)*p5;


	bio_dev * dev = get_dev_by_drvid(drvid);
	GVariant * flv_null;
	GVariantBuilder* _builder = g_variant_builder_new(G_VARIANT_TYPE("av"));
	flv_null = g_variant_builder_end(_builder);

	if (dev == NULL) {
		biometric_complete_get_feature_list(object, invocation, 0, flv_null);
		return;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[drvid]) != 0) {
		biometric_complete_get_feature_list(object, invocation, 0, flv_null);
		return;
	}
	g_variant_builder_unref(_builder);
	g_variant_unref(flv_null);

	feature_info * flist = NULL;
	flist = bio_ops_get_feature_list(dev, uid, idx_start, idx_end);
	pthread_mutex_unlock(&dev_dbus_mutex[drvid]);

	int ops_result = bio_get_ops_result(dev);
	if (ops_result != OPS_GET_FLIST_SUCCESS)
	{
		_builder = g_variant_builder_new(G_VARIANT_TYPE("av"));
		flv_null = g_variant_builder_end(_builder);
		biometric_complete_get_feature_list(object, invocation,
											-RetOpsError, flv_null);
		return;
	}

	// 构建特征信息列表
	feature_info * l = flist;
	GVariant * flv;
	_builder = g_variant_builder_new(G_VARIANT_TYPE("av"));

	while (l != NULL) {
		GVariant * _item = g_variant_new("(iisis)", l->uid, l->biotype,
										 bio_new_string(l->driver),
										 l->index,
										 bio_new_string(l->index_name));
		g_variant_builder_add(_builder, "v", _item);

		l = l->next;
		count += 1;
	}
	flv = g_variant_builder_end(_builder);
	g_variant_builder_unref(_builder);

	if (flist != NULL)
		bio_sto_free_feature_info(flist);

	biometric_complete_get_feature_list(object, invocation, count, flv);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	free(p5);
	g_slist_free(argv);
}

static gboolean gdbus_get_feature_list(Biometric *object,
							 GDBusMethodInvocation *invocation,
							 const int drvid, const int uid,
							 const int idx_start, const int idx_end)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &uid, sizeof(const int));
	argv = g_slist_append(argv, p3);

	p4 = malloc(sizeof(const int));
	memcpy(p4, &idx_start, sizeof(const int));
	argv = g_slist_append(argv, p4);

	p5 = malloc(sizeof(const int));
	memcpy(p5, &idx_end, sizeof(const int));
	argv = g_slist_append(argv, p5);

	err = pthread_create(&thr, NULL, (void *)gdbus_get_feature_list_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

void gdbus_update_status_pt(GSList * argv) {
	int ret = -1;
	int enable = -1;
	int dev_num = 0;
	int dev_status = -1;
	int ops_result = -1;
	int notify_mid = -1;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;

	p0 = g_slist_nth_data(argv,0);
	p1 = g_slist_nth_data(argv,1);
	p2 = g_slist_nth_data(argv,2);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;

	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev != NULL) {
		enable = dev->enable;
		dev_num = dev->dev_num;
		dev_status = bio_get_dev_status(dev);
		ops_result = bio_get_ops_result(dev);
		notify_mid = bio_get_notify_mid(dev);
		ret = -RetOpsSuccess;
	} else
		ret = -RetNoSuchDevice;

	biometric_complete_update_status(object, invocation, ret, enable, dev_num,
									 dev_status, ops_result, notify_mid,
									 timestamp_dev_status[drvid].tv_sec,
									 timestamp_dev_status[drvid].tv_usec,
									 timestamp_ops_result[drvid].tv_sec,
									 timestamp_ops_result[drvid].tv_usec,
									 timestamp_notify_mid[drvid].tv_sec,
									 timestamp_notify_mid[drvid].tv_usec);
	free(p0);
	free(p1);
	free(p2);
	g_slist_free(argv);
}

static gboolean gdbus_update_status(Biometric *object,
									GDBusMethodInvocation *invocation,
									const int drvid)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	err = pthread_create(&thr, NULL, (void *)gdbus_update_status_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

static gboolean gdbus_get_dev_mesg(Biometric *object,
								   GDBusMethodInvocation *invocation,
								   const int drvid)
{
	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		struct timeval now;
		gettimeofday(&now, NULL);
		biometric_complete_get_dev_mesg(object, invocation, mesg_nodev,
										now.tv_sec, now.tv_usec);
		return true;
	}

	biometric_complete_get_dev_mesg(object, invocation,
									bio_get_dev_status_mesg(dev),
									timestamp_dev_status[drvid].tv_sec,
									timestamp_dev_status[drvid].tv_usec);
	return true;
}

static gboolean gdbus_get_ops_mesg(Biometric *object,
								   GDBusMethodInvocation *invocation,
								   const int drvid)
{
	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		struct timeval now;
		gettimeofday(&now, NULL);
		biometric_complete_get_ops_mesg(object, invocation, mesg_nodev,
										now.tv_sec, now.tv_usec);
		return true;
	}

	biometric_complete_get_ops_mesg(object, invocation,
									bio_get_ops_result_mesg(dev),
									timestamp_ops_result[drvid].tv_sec,
									timestamp_ops_result[drvid].tv_usec);
	return true;
}

static gboolean gdbus_get_notify_mesg(Biometric *object,
								   GDBusMethodInvocation *invocation,
								   const int drvid)
{
	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		struct timeval now;
		gettimeofday(&now, NULL);
		biometric_complete_get_notify_mesg(object, invocation, mesg_nodev,
										   now.tv_sec, now.tv_usec);
		return true;
	}

	biometric_complete_get_notify_mesg(object, invocation, bio_get_notify_mid_mesg(dev),
									   timestamp_notify_mid[drvid].tv_sec,
									   timestamp_notify_mid[drvid].tv_usec);

	return true;
}

void gdbus_enroll_pt(GSList * argv) {
	int ret = -1;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	char *p5;

	p0 = g_slist_nth_data(argv,0);
	p1 = g_slist_nth_data(argv,1);
	p2 = g_slist_nth_data(argv,2);
	p3 = g_slist_nth_data(argv,3);
	p4 = g_slist_nth_data(argv,4);
	p5 = g_slist_nth_data(argv,5);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int uid = (int)*p3;
	int idx = (int)*p4;
	char * idx_name = p5;

	gboolean auth_ret = FALSE;
	int auth_uid = 0;
	get_dbus_caller_uid(invocation, &auth_uid);
	if (auth_uid == uid)
		auth_ret = authority_check_by_polkit(pAuthority,
											 invocation,
											 POLKIT_ENROLL_OWN_ACTION_ID);
	else
		auth_ret = authority_check_by_polkit(pAuthority,
											 invocation,
											 POLKIT_ENROLL_ADMIN_ACTION_ID);

	if (!auth_ret) {
		biometric_complete_enroll(object, invocation, -RetPermissionDenied);
		return;
	}

	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		biometric_complete_enroll(object, invocation, -RetNoSuchDevice);
		return;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[drvid]) != 0) {
		biometric_complete_enroll(object, invocation, -RetDeviceBusy);
		return;
	}

	if (idx == -1)
	{
		idx = bio_common_get_empty_index(dev, uid, 0, -1);
		bio_print_debug(_("Framework automatically assigns the free index '%d'"
						  " to driver %s user %d\n"),
						idx, dev->device_name, uid);
	}
	ret = bio_ops_enroll(dev, uid, idx, (char *)idx_name);
	pthread_mutex_unlock(&dev_dbus_mutex[drvid]);

	if (bio_get_ops_result(dev) % 100 == OPS_COMM_SUCCESS) {
		bio_print_info(_("Enroll Result: Success\n"));
		ret = -RetOpsSuccess;
	} else {
		bio_print_info(_("Enroll Result: Fail\n"));
		ret = -RetOpsError;
	}

	biometric_complete_enroll(object, invocation, ret);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	free(p5);
	g_slist_free(argv);
}

static gboolean gdbus_enroll(Biometric *object,
							 GDBusMethodInvocation *invocation,
							 const int drvid, const int uid, const int idx,
							 const char * idx_name)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	char *p5;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &uid, sizeof(const int));
	argv = g_slist_append(argv, p3);

	p4 = malloc(sizeof(const int));
	memcpy(p4, &idx, sizeof(const int));
	argv = g_slist_append(argv, p4);

	p5 = bio_new_string((char *)idx_name);
	argv = g_slist_append(argv, p5);

	err = pthread_create(&thr, NULL, (void *)gdbus_enroll_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

void gdbus_verify_pt(GSList * argv) {
	int ret = -1;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;

	p0 = g_slist_nth_data(argv,0);
	p1 = g_slist_nth_data(argv,1);
	p2 = g_slist_nth_data(argv,2);
	p3 = g_slist_nth_data(argv,3);
	p4 = g_slist_nth_data(argv,4);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int uid = (int)*p3;
	int idx = (int)*p4;

	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		biometric_complete_verify(object, invocation, -RetNoSuchDevice);
		return;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[drvid]) != 0) {
		biometric_complete_verify(object, invocation, -RetDeviceBusy);
		return;
	}

	ret = bio_ops_verify(dev, uid, idx);
	pthread_mutex_unlock(&dev_dbus_mutex[drvid]);

	int ops_result;
	ops_result = bio_get_ops_result(dev);

	switch (ops_result)
	{
	case OPS_VERIFY_MATCH:
		bio_print_info(_("Verify Result: Match\n"));
		ret = -RetOpsSuccess;
		break;
	case OPS_VERIFY_NO_MATCH:
		bio_print_info(_("Verify Result: No Match\n"));
		ret = -RetOpsNoMatch;
		break;
	default:
		ret = -RetOpsError;
	}

	biometric_complete_verify(object, invocation, ret);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	g_slist_free(argv);
}

static gboolean gdbus_verify (Biometric *object,
							  GDBusMethodInvocation *invocation,
							  const int drvid, const int uid, const int idx)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &uid, sizeof(const int));
	argv = g_slist_append(argv, p3);

	p4 = malloc(sizeof(const int));
	memcpy(p4, &idx, sizeof(const int));
	argv = g_slist_append(argv, p4);

	err = pthread_create(&thr, NULL, (void *)gdbus_verify_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

void gdbus_identify_pt(GSList * argv) {
	int ret = -1;
	int uid_ret = -1;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = g_slist_nth_data(argv, 0);
	p1 = g_slist_nth_data(argv, 1);
	p2 = g_slist_nth_data(argv, 2);
	p3 = g_slist_nth_data(argv, 3);
	p4 = g_slist_nth_data(argv, 4);
	p5 = g_slist_nth_data(argv, 5);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int uid = (int)*p3;
	int idx_start = (int)*p4;
	int idx_end = (int)*p5;

	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		biometric_complete_identify(object, invocation, -RetNoSuchDevice, -1);
		return;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[drvid]) != 0) {
		biometric_complete_identify(object, invocation, -RetDeviceBusy, -1);
		return;
	}

	uid_ret = bio_ops_identify(dev, uid, idx_start, idx_end);
	pthread_mutex_unlock(&dev_dbus_mutex[drvid]);

	int ops_result;
	ops_result = bio_get_ops_result(dev);

	switch (ops_result)
	{
	case OPS_IDENTIFY_MATCH:
		bio_print_info(_("Identify Result: UID = %d\n"), uid_ret);
		ret = -RetOpsSuccess;
		break;
	case OPS_IDENTIFY_NO_MATCH:
		bio_print_info(_("Identify Result: No Match\n"));
		ret = -RetOpsNoMatch;
		break;
	default:
		ret = -RetOpsError;
	}

	biometric_complete_identify(object, invocation, ret, uid_ret);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	free(p5);
	g_slist_free(argv);
}

static gboolean gdbus_identify(Biometric *object,
							   GDBusMethodInvocation *invocation,
							   const int drvid, const int uid,
							   const int idx_start, const int idx_end)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &uid, sizeof(const int));
	argv = g_slist_append(argv, p3);

	p4 = malloc(sizeof(const int));
	memcpy(p4, &idx_start, sizeof(const int));
	argv = g_slist_append(argv, p4);

	p5 = malloc(sizeof(const int));
	memcpy(p5, &idx_end, sizeof(const int));
	argv = g_slist_append(argv, p5);

	err = pthread_create(&thr, NULL, (void *)gdbus_identify_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

void gdbus_capture_pt(GSList * argv) {
	int ret = -1;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;

	p0 = g_slist_nth_data(argv, 0);
	p1 = g_slist_nth_data(argv, 1);
	p2 = g_slist_nth_data(argv, 2);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;

	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		biometric_complete_capture(object, invocation, -RetNoSuchDevice, bio_new_string(NULL));
		return;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[drvid]) != 0) {
		biometric_complete_capture(object, invocation, -RetDeviceBusy, bio_new_string(NULL));
		return;
	}

	char * caps = NULL;
	caps = bio_ops_capture(dev);
	pthread_mutex_unlock(&dev_dbus_mutex[drvid]);

	if (bio_get_ops_result(dev) % 100 == OPS_COMM_SUCCESS) {
		bio_print_info(_("Capture Result: %.*s\n"), 32, caps);
		ret = -RetOpsSuccess;
	} else {
		bio_print_info(_("Capture Result: ERROR\n"));
		ret = -RetOpsError;
		free(caps);
	}

	biometric_complete_capture(object, invocation, ret, caps);

	free(p0);
	free(p1);
	free(p2);
	g_slist_free(argv);
}

static gboolean gdbus_capture(Biometric *object,
							  GDBusMethodInvocation *invocation,
							  const int drvid)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	err = pthread_create(&thr, NULL, (void *)gdbus_capture_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

void gdbus_search_pt(GSList * argv)
{
		int count= 0;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = g_slist_nth_data(argv, 0);
	p1 = g_slist_nth_data(argv, 1);
	p2 = g_slist_nth_data(argv, 2);
	p3 = g_slist_nth_data(argv, 3);
	p4 = g_slist_nth_data(argv, 4);
	p5 = g_slist_nth_data(argv, 5);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int uid = (int)*p3;
	int idx_start = (int)*p4;
	int idx_end = (int)*p5;

	bio_dev * dev = get_dev_by_drvid(drvid);
	GVariant * flv_null;
	GVariantBuilder* _builder = g_variant_builder_new(G_VARIANT_TYPE("av"));
	flv_null = g_variant_builder_end(_builder);

	if (dev == NULL) {
		biometric_complete_search(object, invocation, -RetNoSuchDevice, flv_null);
		return;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[drvid]) != 0) {
		biometric_complete_search(object, invocation, -RetDeviceBusy, flv_null);
		return;
	}
	g_variant_builder_unref(_builder);
	g_variant_unref(flv_null);

	feature_info * flist = NULL;
	flist = bio_ops_search(dev, uid, idx_start, idx_end);
	pthread_mutex_unlock(&dev_dbus_mutex[drvid]);

	int ops_result;
	ops_result = bio_get_ops_result(dev);
	if (ops_result != OPS_SEARCH_MATCH)
	{
		_builder = g_variant_builder_new(G_VARIANT_TYPE("av"));
		flv_null = g_variant_builder_end(_builder);

		if (ops_result == OPS_SEARCH_NO_MATCH)
			biometric_complete_search(object, invocation, -RetOpsSuccess, flv_null);
		else
			biometric_complete_search(object, invocation, -RetOpsError, flv_null);
	}

	// 构建特征信息列表
	feature_info * l = flist;
	GVariant * flv;
	_builder = g_variant_builder_new(G_VARIANT_TYPE("av"));

	while (l != NULL) {
		GVariant * _item = g_variant_new("(iis)", l->uid, l->index,
										 bio_new_string(l->index_name));
		g_variant_builder_add(_builder, "v", _item);

		l = l->next;
		count += 1;
	}
	flv = g_variant_builder_end(_builder);
	g_variant_builder_unref(_builder);

	if (flist != NULL)
		bio_sto_free_feature_info(flist);

	biometric_complete_search(object, invocation, count, flv);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	free(p5);
	g_slist_free(argv);
}

static gboolean gdbus_search(Biometric *object,
							 GDBusMethodInvocation *invocation,
							 const int drvid, const int uid,
							 const int idx_start, const int idx_end)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &uid, sizeof(const int));
	argv = g_slist_append(argv, p3);

	p4 = malloc(sizeof(const int));
	memcpy(p4, &idx_start, sizeof(const int));
	argv = g_slist_append(argv, p4);

	p5 = malloc(sizeof(const int));
	memcpy(p5, &idx_end, sizeof(const int));
	argv = g_slist_append(argv, p5);

	err = pthread_create(&thr, NULL, (void *)gdbus_search_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

int gdbus_clean_by_single_dev(bio_dev * dev, int uid, int idx_start, int idx_end)
{
	int ret = 0;

	if (dev == NULL || get_dev_by_drvid(dev->driver_id) == NULL) {
		return -RetNoSuchDevice;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[dev->driver_id]) != 0) {
		return -RetDeviceBusy;
	}

	ret = bio_ops_clean(dev, uid, idx_start, idx_end);
	pthread_mutex_unlock(&dev_dbus_mutex[dev->driver_id]);

	if (bio_get_ops_result(dev) % 100 == OPS_COMM_SUCCESS) {
		bio_print_info(_("%s Clean Result: Success\n"), dev->device_name);
		return -RetOpsSuccess;
	} else {
		bio_print_info(_("%s Clean Result: Failed\n"), dev->device_name);
		return -RetOpsError;
	}

	return ret;
}

void gdbus_clean_pt(GSList * argv) {
	int ret = 0;
	int single_ret = 0;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = g_slist_nth_data(argv, 0);
	p1 = g_slist_nth_data(argv, 1);
	p2 = g_slist_nth_data(argv, 2);
	p3 = g_slist_nth_data(argv, 3);
	p4 = g_slist_nth_data(argv, 4);
	p5 = g_slist_nth_data(argv, 5);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int uid = (int)*p3;
	int idx_start = (int)*p4;
	int idx_end = (int)*p5;

	gboolean auth_ret = FALSE;
	int auth_uid = 0;
	get_dbus_caller_uid(invocation, &auth_uid);
	if (auth_uid == uid)
		auth_ret = authority_check_by_polkit(pAuthority,
											 invocation,
											 POLKIT_CLEAN_OWN_ACTION_ID);
	else
		auth_ret = authority_check_by_polkit(pAuthority,
											 invocation,
											 POLKIT_CLEAN_ADMIN_ACTION_ID);

	if (!auth_ret) {
		biometric_complete_enroll(object, invocation, -RetPermissionDenied);
		return;
	}

	if (drvid != -1)
	{
		bio_dev * dev = get_dev_by_drvid(drvid);
		ret = gdbus_clean_by_single_dev(dev, uid, idx_start, idx_end);
	} else {
		GList * l = bio_get_drv_list();
		single_ret = 0;
		ret = 0;
		while (l != NULL)
		{
			single_ret = gdbus_clean_by_single_dev(l->data, uid, idx_start, idx_end);
			if (ret == 0)
				ret = single_ret;
			l = l->next;
		}
		if (ret != 0)
			ret = -RetOpsError;

		// 系统层清理进行彻底的清理，免受当前不可用设备遗留的生物特征信息影响
		bio_print_info(_("System level cleaning of the specified range of biometric "
						 "features (all drivers)\n"));
		sqlite3 * db;
		db = bio_sto_connect_db();
		single_ret = bio_sto_clean_feature_info(db, uid, -1, NULL, idx_start, idx_end);
		bio_sto_disconnect_db(db);

		if (single_ret == 0)
			bio_print_info(_("System level Clean Result: Success\n"));
		else
		{
			bio_print_info(_("System level Clean Result: Failed\n"));
			ret = -RetOpsError;
		}
	}

	biometric_complete_clean(object, invocation, ret);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	free(p5);
	g_slist_free(argv);
}

static gboolean gdbus_clean(Biometric *object,
							GDBusMethodInvocation *invocation,
							const int drvid, const int uid,
							const int idx_start, const int idx_end)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	int *p5;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &uid, sizeof(const int));
	argv = g_slist_append(argv, p3);

	p4 = malloc(sizeof(const int));
	memcpy(p4, &idx_start, sizeof(const int));
	argv = g_slist_append(argv, p4);

	p5 = malloc(sizeof(const int));
	memcpy(p5, &idx_end, sizeof(const int));
	argv = g_slist_append(argv, p5);

	err = pthread_create(&thr, NULL, (void *)gdbus_clean_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

void gdbus_feature_rename_pt(GSList * argv)
{
	int ret = -RetOpsSuccess;

	Biometric ** p0;
	GDBusMethodInvocation ** p1;
	int * p2;
	int * p3;
	int * p4;
	char * p5;

	p0 = g_slist_nth_data(argv, 0);
	p1 = g_slist_nth_data(argv, 1);
	p2 = g_slist_nth_data(argv, 2);
	p3 = g_slist_nth_data(argv, 3);
	p4 = g_slist_nth_data(argv, 4);
	p5 = g_slist_nth_data(argv, 5);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int uid = (int)*p3;
	int idx = (int)*p4;
	char * new_name = p5;

	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		biometric_complete_stop_ops(object, invocation, -RetNoSuchDevice);
		return;
	}

	if (pthread_mutex_trylock(&dev_dbus_mutex[drvid]) != 0) {
		biometric_complete_stop_ops(object, invocation, -RetDeviceBusy);
		return;
	}

	bio_print_debug("Rename %s feature, UID = %d, INDEX = %d, NewName = %s\n",
					dev->device_name, uid, idx, new_name);
	ret = bio_ops_feature_rename(dev, uid, idx, new_name);
	pthread_mutex_unlock(&dev_dbus_mutex[drvid]);
	bio_print_info("Rename result = %d\n", ret);

	int ops_result;
	ops_result = bio_get_ops_result(dev);

	if (ops_result == OPS_RENAME_SUCCESS)
		biometric_complete_rename(object, invocation, -RetOpsSuccess);
	else
		biometric_complete_rename(object, invocation, -RetOpsError);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	free(p4);
	free(p5);
	g_slist_free(argv);
}

static gboolean gdbus_feature_rename(Biometric *object,
									 GDBusMethodInvocation *invocation,
									 const int drvid, const int uid,
									 int idx, const char * new_name)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;
	int *p4;
	char * p5;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &uid, sizeof(const int));
	argv = g_slist_append(argv, p3);

	p4 = malloc(sizeof(const int));
	memcpy(p4, &idx, sizeof(const int));
	argv = g_slist_append(argv, p4);

	p5 = bio_new_string((char *)new_name);
	argv = g_slist_append(argv, p5);

	err = pthread_create(&thr, NULL, (void *)gdbus_feature_rename_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

void gdbus_stop_ops_pt(GSList * argv) {
	int ret = -1;

	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;

	p0 = g_slist_nth_data(argv, 0);
	p1 = g_slist_nth_data(argv, 1);
	p2 = g_slist_nth_data(argv, 2);
	p3 = g_slist_nth_data(argv, 3);

	Biometric *object = (Biometric *)*p0;
	GDBusMethodInvocation *invocation = (GDBusMethodInvocation *)*p1;
	int drvid = (int)*p2;
	int waiting_sec = (int)*p3;

	bio_dev * dev = get_dev_by_drvid(drvid);
	if (dev == NULL) {
		biometric_complete_stop_ops(object, invocation, -RetNoSuchDevice);
		return;
	}

	bio_print_debug("===========\n");
	ret = bio_ops_stop_ops(dev, waiting_sec);
	bio_print_info("ret = %d\n", ret);
	if (ret != 0)
		ret = -RetOpsError;

	biometric_complete_stop_ops(object, invocation, ret);

	free(p0);
	free(p1);
	free(p2);
	free(p3);
	g_slist_free(argv);
}

static gboolean gdbus_stop_ops(Biometric *object,
							  GDBusMethodInvocation *invocation,
							  const int drvid, const int waiting_sec)
{
	int err = 0;
	pthread_t thr;

	GSList *argv = NULL;
	Biometric **p0;
	GDBusMethodInvocation **p1;
	int *p2;
	int *p3;

	p0 = malloc(sizeof(void *));
	memcpy(p0, &object, sizeof(void *));
	argv = g_slist_append(argv, p0);

	p1 = malloc(sizeof(void *));
	memcpy(p1, &invocation, sizeof(void *));
	argv = g_slist_append(argv, p1);

	p2 = malloc(sizeof(const int));
	memcpy(p2, &drvid, sizeof(const int));
	argv = g_slist_append(argv, p2);

	p3 = malloc(sizeof(const int));
	memcpy(p3, &waiting_sec, sizeof(const int));
	argv = g_slist_append(argv, p3);

	err = pthread_create(&thr, NULL, (void *)gdbus_stop_ops_pt, argv);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	}

	return true;
}

static void gdbus_signal_status_changed(int drvid, int type)
{
	switch (type) {
	case STATUS_TYPE_DEVICE:
		gettimeofday(&timestamp_dev_status[drvid], NULL);
		break;
	case STATUS_TYPE_OPERATION:
		gettimeofday(&timestamp_ops_result[drvid], NULL);
		break;
	case STATUS_TYPE_NOTIFY:
		gettimeofday(&timestamp_notify_mid[drvid], NULL);
		break;

	default:
		break;
	}

	biometric_emit_status_changed(pSkeleton, drvid, type);
}

static void gdbus_usb_device_hot_plug_callback(int drvid, int action,
											   int dev_num_now)
{
	biometric_emit_usbdevice_hot_plug(pSkeleton, drvid, action, dev_num_now);
}

static void bus_acquired_cb(GDBusConnection *connection,
							const gchar *bus_name,
							gpointer user_data)
{
	GError *pError = NULL;

	/** Second step: Try to get a connection to the given bus. */
	pSkeleton = biometric_skeleton_new();

	/** Third step: Attach to dbus signals. */
	g_signal_connect(pSkeleton, "handle-check-app-api-version",
					 G_CALLBACK(gdbus_check_app_api_version), NULL);
	g_signal_connect(pSkeleton, "handle-get-drv-list",
					 G_CALLBACK(gdbus_get_drv_list), NULL);
	g_signal_connect(pSkeleton, "handle-get-dev-list",
					 G_CALLBACK(gdbus_get_dev_list), NULL);
	g_signal_connect(pSkeleton, "handle-get-feature-list",
					 G_CALLBACK(gdbus_get_feature_list), NULL);
	g_signal_connect(pSkeleton, "handle-update-status",
					 G_CALLBACK(gdbus_update_status), NULL);
	g_signal_connect(pSkeleton, "handle-get-dev-mesg",
					 G_CALLBACK(gdbus_get_dev_mesg), NULL);
	g_signal_connect(pSkeleton, "handle-get-ops-mesg",
					 G_CALLBACK(gdbus_get_ops_mesg), NULL);
	g_signal_connect(pSkeleton, "handle-get-notify-mesg",
					 G_CALLBACK(gdbus_get_notify_mesg), NULL);
	g_signal_connect(pSkeleton, "handle-enroll",
					 G_CALLBACK(gdbus_enroll), NULL);
	g_signal_connect(pSkeleton, "handle-verify",
					 G_CALLBACK(gdbus_verify), NULL);
	g_signal_connect(pSkeleton, "handle-identify",
					 G_CALLBACK(gdbus_identify), NULL);
	g_signal_connect(pSkeleton, "handle-capture",
					 G_CALLBACK(gdbus_capture), NULL);
	g_signal_connect(pSkeleton, "handle-search",
					 G_CALLBACK(gdbus_search), NULL);
	g_signal_connect(pSkeleton, "handle-clean",
					 G_CALLBACK(gdbus_clean), NULL);
	g_signal_connect(pSkeleton, "handle-rename",
					 G_CALLBACK(gdbus_feature_rename), NULL);
	g_signal_connect(pSkeleton, "handle-stop-ops",
					 G_CALLBACK(gdbus_stop_ops), NULL);

	bio_status_changed_callback = gdbus_signal_status_changed;
	bio_usb_device_hot_plug_callback = gdbus_usb_device_hot_plug_callback;

	/** Fourth step: Export interface skeleton. */
	(void) g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(pSkeleton),
											  connection,
											  UKUI_GDBUS_BIO_OBJECT_PATH,
											  &pError);
	if(pError != NULL){
		bio_print_error(_("Error: Failed to export object. Reason: %s\n"), pError->message);
		g_error_free(pError);
		g_main_loop_quit(pLoop);
	}
}

static void name_acquired_cb(GDBusConnection *connection,
									 const gchar *bus_name,
									 gpointer user_data)
{
	bio_print_notice(_("name_acquired_cb called, Acquired bus name: %s\n"), UKUI_GDBUS_BIO_BUS_NAME);
}

static void name_lost_cb(GDBusConnection *connection,
							 const gchar *bus_name,
							 gpointer user_data)
{
	bio_print_info("bus_name = %s\n", bus_name);
	if(connection == NULL) {
		bio_print_error(_("name_lost_cb called, Error: Failed to connect to dbus\n"));
	} else {
		bio_print_error(_("name_lost_cb called, Error: Failed to obtain bus name: %s\n"), UKUI_GDBUS_BIO_BUS_NAME);
	}

	g_main_loop_quit(pLoop);
}

void* run(void* para)
{
	bio_print_info(_("run called in the server\n"));
		/** Start the Main Event Loop which manages all available sources of events */
	g_main_loop_run( pLoop );

	return ((void*)0);
}

int thread_create(void)
{
	int err;
	pthread_t thr;

	err = pthread_create(&thr, NULL, run, NULL);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	} else {
		bio_print_info(_("New thread created: %s\n"), strerror(err));
	}

	return err;
}

bool InitBioDBusCommServer(void)
{
	bool bRet = TRUE;

	/** init for usage of "g" types */
//	g_type_init();

	bio_print_notice(_("InitDBusCommunicationServer: Server started\n"));

	/** create main loop, but do not start it. */
	pLoop = g_main_loop_new(NULL, FALSE);

	/** first step: connect to dbus */
	(void)g_bus_own_name(UKUI_GDBUS_BIO_BUS,
						UKUI_GDBUS_BIO_BUS_NAME,
						G_BUS_NAME_OWNER_FLAGS_REPLACE|G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
						&bus_acquired_cb,
						&name_acquired_cb,
						&name_lost_cb,
						NULL,
						NULL);

	pAuthority = polkit_authority_get_sync (NULL, NULL);
	return bRet;
}

int InitBioDevice() {
	int ret = 0;
	GList * drv_list = NULL;

	ret  = bio_init();
	if (ret != 0)
		return -1;

	/*
	 * Initialize driver list and device list.
	 *
	 * The initialization driver list only loads the driver and initializes
	 * the associated configuration.
	 *
	 * Initializing the device list will detect the device.
	 *
	 * 初始化驱动链表、设备链表
	 * 初始化驱动链表只加载驱动和初始化相关配置
	 * 初始化设备链表会探测设备
	 */
	drv_list = bio_driver_list_init();
	bio_device_list_init(drv_list);

	bio_print_drv_list(BIO_PRINT_LEVEL_NOTICE);
	bio_print_dev_list(BIO_PRINT_LEVEL_NOTICE);

	return 0;
}

void UnInitBioDevice() {
	GList * l = NULL;
	int i = 0;

	l = bio_get_dev_list();
	for (i = 0; l != NULL; i++) {
		bio_dev * dev = l->data;
		dev->ops_close(dev);
		l = l->next;
	}

	bio_free_dev_list();
	bio_free_drv_list();

	bio_close();
}

void ReInitBioDevice() {
	UnInitBioDevice();
	InitBioDevice();
}

void monitor_usb_hotplug(void* para) {
	int rc =0;

	while (1) {
			rc = libusb_handle_events (NULL);
			if (rc < 0)
				bio_print_error(_("libusb_handle_events failed: %s\n"), libusb_error_name(rc));
		}
}

int main()
{
	int i = 0;
	int ret = 0;
	int uid = getuid();
	int pid = getpid();
	char * pid_dir_list[PID_DIR_NUM] = {PID_DIR_0, PID_DIR_1};
	char * pid_dir = NULL;
	char pid_file[MAX_PATH_LEN] = {0};
	char pid_string[MAX_PID_STRING_LEN] = {0};

	if (uid != 0)
	{
		bio_print_error(_("The biometric authentication server failed to start, "
						  "requiring administrator permission to start\n"));
		return -1;
	}

	for (i = 0; i < PID_DIR_NUM; i++)
	{
		struct stat dir_stat;

        if (access(pid_dir_list[i], F_OK) == 0) {
            stat(pid_dir_list[i], &dir_stat);
            if (S_ISDIR(dir_stat.st_mode)) {
                pid_dir = pid_dir_list[i];
                break;
            }
        }
	}
	if (pid_dir == NULL)
		pid_dir = "/";

	snprintf(pid_file, MAX_PATH_LEN, "%s/%s.pid", pid_dir, AUTH_SERVER_NAME);
	int pid_file_fd = open(pid_file, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (pid_file_fd < 0)
	{
		bio_print_error(_("Can not open PID file: %s\n"), pid_file);
		return -1;
	}

	ret = flock(pid_file_fd, LOCK_EX | LOCK_NB);
	if (ret < 0)
	{
		bio_print_error(_("Biometric authentication server is running, "
						  "Do not restart it repeatedly\n"));
		return -1;
	}

	snprintf(pid_string, MAX_PID_STRING_LEN, "%d\n", pid);
	ret = write(pid_file_fd, pid_string, strlen(pid_string));
	if (ret != strlen(pid_string))
	{
		bio_print_warning(_("There is an exception to write PID file: %s\n"),
						  pid_file);
	}

	setlocale (LC_ALL, "");
	bindtextdomain(DOMAIN_NAME, LOCALEDIR);
	textdomain(DOMAIN_NAME);

	mesg_nodev = _("No such device");
	mesg_devbusy = _("Device busy");

	ret = InitBioDevice();
	if (ret != 0)
	{
		bio_print_error(_("Initial biometric recognition framework failed\n"));
		return -1;
	}

	int err;
	pthread_t thr;

	err = pthread_create(&thr, NULL, (void *)monitor_usb_hotplug, NULL);

	if (err != 0) {
		bio_print_error(_("Can't create thread: %s\n"), strerror(err));
	} else {
		bio_print_info(_("New thread created: %s\n"), strerror(err));
	}

	for (i = 0; i < MAX_DEVICES; i++) {
		pthread_mutex_init(&dev_dbus_mutex[i], NULL);
		gettimeofday(&timestamp_dev_status[i], NULL);
		gettimeofday(&timestamp_ops_result[i], NULL);
		gettimeofday(&timestamp_notify_mid[i], NULL);
	}

	InitBioDBusCommServer();

	g_main_loop_run(pLoop);
	g_main_loop_unref(pLoop);

	UnInitBioDevice();

	flock(pid_file_fd, LOCK_UN);
	close(pid_file_fd);

	return 0;
}
