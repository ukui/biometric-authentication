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

#include <glib.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <errno.h>

#include "biometric_common.h"
#include "biometric_intl.h"
#include "biometric_storage.h"

const unsigned char kFirstBitMask = 128;// 1000000
const unsigned char kSecondBitMask = 64;// 0100000
const unsigned char kThirdBitMask = 32;	// 0010000
const unsigned char kFourthBitMask = 16;// 0001000
const unsigned char kFifthBitMask = 8;	// 0000100

// UUID_BUF_LEN = 32(UUID_LEN) + 4(Symbol '-') + 1(terminator '\0') = 37
#define UUID_BUF_LEN	37
#define UUID_DEFAULT	"01234567-0123-0123-0123-0123456789AB"


/****************** 内部函数声明 ******************/
int internal_utf8_char_len(char firstByte);
int internal_utf8_str_len(char *str);
int internal_utf8_str_width(char *str);

char * internal_newstr(char * old_str);
int internal_create_dir(const char *new_dir);

void internal_get_uuid_by_uid(int uid, char * uuid);
int internal_upgrade_db_format_from_null(sqlite3 *db);
int internal_create_eigen_info_table(sqlite3 *db);
int internal_create_datebase_info(sqlite3 *db);

int bio_sto_create_table(sqlite3 *db);


/****************** 外部函数实现 ******************/
char * bio_sto_new_str(char * old_str)
{
	return internal_newstr(old_str);
}

/* 输出特征信息结构体 */
void print_feature_info(feature_info * info_list) {
	int uid = 0;
	int olduid = -1;
	int biotype = 0;
	int oldbiotype = -1;
	char * driver = NULL;
	char * olddriver = NULL;
	int idx = 0;
	int oldidx = -1;

	int ustr_l = 0;
	int bstr_l = 0;
	int dstr_l = 0;
	int istr_l = 0;

#define SL 1024
	char ustr[SL];
	char bstr[SL];
	char dstr[SL];
	char istr[SL];

	feature_info * info = NULL;
	feature_sample *sample = NULL;

	if (info_list == NULL)
		return ;

	info = info_list;
	sample = info->sample;
	while(info != NULL){
		uid = info->uid;
		biotype = info->biotype;
		driver = info->driver;
		idx = info->index;

		if (olduid != uid) {
			olduid = uid;
			oldbiotype = -1;
			olddriver = NULL;
			oldidx = -1;

			sprintf(ustr, "U:%04d,", uid);
			ustr_l = internal_utf8_str_width(ustr);
		} else {
			memset(ustr, ' ', SL);
			ustr[ustr_l] = '\0';
		}

		if (oldbiotype != biotype) {
			oldbiotype = biotype;
			olddriver = NULL;
			oldidx = -1;

			sprintf(bstr, "B:%02d,", biotype);
			bstr_l = internal_utf8_str_width(bstr);
		} else {
			memset(bstr, ' ', SL);
			bstr[bstr_l] = '\0';
		}

		if ( (olddriver == NULL) || strcmp(olddriver, driver) != 0) {
			olddriver = driver;
			oldidx = -1;

			sprintf(dstr, "D:%s,", olddriver);
			dstr_l = internal_utf8_str_width(dstr);
		} else {
			memset(dstr, ' ', SL);
			dstr[dstr_l] = '\0';
		}

		if (oldidx != idx) {
			oldidx = idx;
			sprintf(istr, "I:%04d(%s),", idx, info->index_name);
			istr_l = internal_utf8_str_width(istr);
		} else {
			memset(istr, ' ', SL);
			istr[istr_l] = '\0';
		}

		bio_print_debug("%s %s %s %s S:%04d, D:%.16s\n",
						ustr, bstr, dstr, istr, sample->no, sample->data);

		sample = sample->next;
		if (sample == NULL){
			info = info->next;
			if (info != NULL)
				sample = info->sample;
		}
	}
#undef SL
}

sqlite3 *bio_sto_connect_db()
{
	sqlite3 *db = NULL;
	int ret = -1;
	bool need_create = false;

	ret = access(DB_PATH, F_OK);
	if (ret != 0)
	{
		need_create = true;

		ret = access(DB_DIR, F_OK);
		if (ret != 0)
		{
			internal_create_dir(DB_DIR);
		}

		/* 创建数据库 */
		ret = sqlite3_open_v2(DB_PATH, &db,
							  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if (ret != SQLITE_OK)
		{
			bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
			return NULL;
		}
		sqlite3_close_v2(db);

		/* 设置默认权限：0600（只允许root用户读写） */
		ret = chmod(DB_PATH, S_IRUSR | S_IWUSR);
		if (ret != 0)
			bio_print_error(_("Unable to set database(%s) permissions. ERROR"
							  "[%d]: %s\n"), DB_PATH, errno, strerror(errno));
	}

	ret = sqlite3_open_v2(DB_PATH, &db,
							  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
							  NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		return NULL;
	}

	if (need_create) {
		ret = bio_sto_create_table(db);
		if (ret != 0) {
			sqlite3_close_v2(db);
			return NULL;
		}
	}

	return db;
}

void bio_sto_disconnect_db(sqlite3 *db) {
	sqlite3_close_v2(db);
}

feature_sample * bio_sto_new_feature_sample(int no, char * data) {
	feature_sample * new_sample = malloc(sizeof(feature_sample));

	if (new_sample == NULL) {
		bio_print_error(_("Unable to allocate memory\n"));
	} else {
		new_sample->no = no;
		if (data == NULL)
			new_sample->data = NULL;
		else
			new_sample->data = internal_newstr(data);
		new_sample->next = NULL;
	}
	return new_sample;
}

void bio_sto_free_feature_sample(feature_sample * sample) {
	if (sample != NULL) {
		if (sample->data != NULL)
			free(sample->data);
		free(sample);
	}
}

void bio_sto_free_feature_sample_list(feature_sample * sample_list) {
	feature_sample * sample;

	while (sample_list != NULL) {
		sample = sample_list;
		sample_list = sample_list->next;
		bio_sto_free_feature_sample(sample);
	}
}

feature_info * bio_sto_new_feature_info(int	uid,
														int biotype,
														char *driver,
														int	index,
														char *index_name)
{
	feature_info * info = NULL;
	info = malloc(sizeof(feature_info));
	if (info == NULL)
		goto FREE_INFO;
	memset(info, 0, sizeof(feature_info));

	if (driver == NULL)
		info->driver = NULL;
	else
		info->driver = internal_newstr(driver);
	if (info->driver == NULL)
		goto FREE_DRIVER;

	if (index_name == NULL)
		info->index_name = NULL;
	else
		info->index_name = internal_newstr(index_name);
	if (info->index_name == NULL)
		goto FREE_INDEX_NAME;

	info->uid = uid;
	info->biotype = biotype;
	info->index = index;
	info->sample = NULL;
	info->next = NULL;

	return info;

FREE_INDEX_NAME:
	free(info->driver);

FREE_DRIVER:
	free(info);

FREE_INFO:
	bio_print_error(_("Unable to allocate memory\n"));
	return NULL;
}

void bio_sto_free_feature_info(feature_info * info) {
	if (info != NULL) {
		if (info->driver != NULL)
			free(info->driver);
		if (info->index_name != NULL)
			free(info->index_name);
		bio_sto_free_feature_sample_list(info->sample);
		free(info);
	}
}

void bio_sto_free_feature_info_list(feature_info * info_list) {
	feature_info * info;

	while (info_list != NULL) {
		info = info_list;
		info_list = info_list->next;
		bio_sto_free_feature_info(info);
	}
}

feature_info *bio_sto_get_feature_info(sqlite3 *db,
													   int uid,
													   int biotype,
													   char *driver,
													   int index_start,
													   int index_end)
{
	feature_info *info_list = NULL;
	feature_info *info = NULL;
	feature_sample *sample = NULL;

	sqlite3_stmt *stmt = NULL;
	char uuid[UUID_BUF_LEN] = {0};
	char user_uuid[UUID_BUF_LEN] = {0};
	int ret = -1;
	int l = 0;

	char old_uuid[UUID_BUF_LEN] = {0};
	int old_uid = -1;
	int old_bio_type = -1;
	char *old_driver = NULL;
	int idx = -1;
	int old_idx = -1;


	char *base_sql_str = "SELECT ID, UID, UUID, BioType, Driver, EigenIndex, "
						 "  EigenIndexName, SampleNo, EigenData "
						 "FROM EIGEN_INFO "
						 "WHERE EigenIndex >= :index_start";
	char *uid_sql_str = " AND UID = :uid ";
	char *biotype_sql_str = " AND BioType = :biotype ";
	char *driver_sql_str = " AND Driver = :driver ";
	char *idx_end_sql_str = " AND EigenIndex <= :index_end ";
	char *order_sql_str = " ORDER BY UID, UUID, BioType, Driver, EigenIndex, "
						  "EigenIndexName, SampleNo ";
	char *all_sql_str = NULL;

	l = strlen(base_sql_str) + strlen(uid_sql_str) + strlen(biotype_sql_str) +
			strlen(driver_sql_str) + strlen(idx_end_sql_str) +
			strlen(order_sql_str) + 1;
	all_sql_str = malloc(l);
	if (all_sql_str == NULL) {
		bio_print_error(_("Unable to allocate memory\n"));
		return NULL;
	}
	memset(all_sql_str, 0, l);

	l = sprintf(all_sql_str, "%s", base_sql_str);
	if (uid != -1)
		l += sprintf(all_sql_str + l, "%s", uid_sql_str);
	if (biotype != -1)
		l += sprintf(all_sql_str + l, "%s", biotype_sql_str);
	if (driver != NULL)
		l += sprintf(all_sql_str + l, "%s", driver_sql_str);
	if (index_end != -1)
		l += sprintf(all_sql_str + l, "%s", idx_end_sql_str);
	l += sprintf(all_sql_str + l, "%s", order_sql_str);
	bio_print_debug("get sql : %s\n", all_sql_str);

	ret = sqlite3_prepare_v2(db, all_sql_str, -1, &stmt, NULL);
	free(all_sql_str);
	if (ret != SQLITE_OK) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		return NULL;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":uid");
	sqlite3_bind_int64(stmt, idx, uid);

	idx = sqlite3_bind_parameter_index(stmt, ":biotype");
	sqlite3_bind_int64(stmt, idx, biotype);

	idx = sqlite3_bind_parameter_index(stmt, ":driver");
	sqlite3_bind_text(stmt, idx, driver, -1, SQLITE_TRANSIENT);

	idx = sqlite3_bind_parameter_index(stmt, ":index_start");
	sqlite3_bind_int64(stmt, idx, index_start);

	idx = sqlite3_bind_parameter_index(stmt, ":index_end");
	sqlite3_bind_int64(stmt, idx, index_end);

	/* 为了之后处理方便，创建表头空白项 */
	info_list = malloc(sizeof(feature_info));
	if (info_list == NULL) {
		bio_print_error(_("Unable to allocate memory\n"));
		sqlite3_finalize(stmt);
		return NULL;
	}
	memset(info_list, 0, sizeof(feature_info));
	info = info_list;

	while( sqlite3_step(stmt) == SQLITE_ROW ) {
		uid = sqlite3_column_int(stmt, 1);
		strncpy(uuid, (char *)sqlite3_column_text(stmt, 2), UUID_BUF_LEN);
		biotype = sqlite3_column_int(stmt, 3);
		driver = (char *)sqlite3_column_text(stmt, 4);
		idx = sqlite3_column_int(stmt, 5);

		/* 条件判断成功，代表uid、biotype、driver或index任意一个值有变化，
		 * 需要新建表项存储 */
		if ( (old_uid < uid) || (old_bio_type < biotype) ||
			 (strcmp(old_driver, driver) != 0) || (old_idx < idx) ||
			 (strcmp(old_uuid, uuid) != 0) )
		{
			if (old_uid != uid)
			{
				// UID变化，UUID也需要跟着变化
				internal_get_uuid_by_uid(uid, user_uuid);
				old_uid = uid;
			}

			// 检测数据库中的UUID是否与当前UID的UUID匹配，不匹配则弃用该条记录
			if (strcmp(uuid, user_uuid) != 0)
			{
				continue;
			}

			old_uid = uid;
			old_bio_type = biotype;
			old_driver = internal_newstr(driver);
			old_idx = idx;
			strncpy(old_uuid, uuid, UUID_BUF_LEN);

			info->next = malloc(sizeof(feature_info));
			info = info->next;
			if (info == NULL) {
				/* malloc失败，释放资源 */
				bio_sto_free_feature_info_list(info_list);
				return NULL;
			}
			memset(info, 0, sizeof(feature_info));

			info->uid = uid;
			info->biotype = biotype;
			info->driver = old_driver;
			info->index = idx;
			info->index_name = internal_newstr((char *)sqlite3_column_text(stmt, 6));

			sample = malloc(sizeof(feature_sample));
			if (sample == NULL) {
				bio_sto_free_feature_info_list(info_list);
				return NULL;
			}
			memset(sample, 0, sizeof(feature_sample));
			info->sample = sample;
		} else {
			/* 检测数据库中的UUID是否与当前UID的UUID匹配 */
			if (strcmp(uuid, user_uuid) != 0)
			{
				continue;
			}

			sample->next = malloc(sizeof(feature_sample));
			sample = sample->next;
			if (sample == NULL) {
				bio_sto_free_feature_info_list(info_list);
				return NULL;
			}
			memset(sample, 0, sizeof(feature_sample));
		}

		sample->dbid = sqlite3_column_int64(stmt, 0);
		sample->no = sqlite3_column_int(stmt, 7);
		sample->data = internal_newstr((char *)sqlite3_column_text(stmt, 8));
	}

	/* 移除表头空白项 */
	info = info_list->next;
	free(info_list);
	info_list = info;

	sqlite3_finalize(stmt);
	return info_list;
}

int bio_sto_set_feature_info(sqlite3 *db, feature_info * info_list)
{
	feature_info *info = NULL;
	feature_sample *sample = NULL;

	sqlite3_stmt *stmt = NULL;
	char uuid[UUID_BUF_LEN] = {0};
	int ret = -1;
	int idx = -1;
	int old_uid = -1;

	char *all_sql_str = "INSERT INTO [EIGEN_INFO] (ID, UID, UUID, BioType, Driver, "
						"  EigenIndex, EigenIndexName, SampleNo, EigenData) "
						"VALUES (NULL, :uid, :uuid, :biotype, :driver, :idx, "
						"  :idx_name, :sno, :sdata);";
	ret = sqlite3_prepare_v2(db, all_sql_str, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		return -1;
	}

	info = info_list;
	if (info == NULL)
		return 0;

	sample = info->sample;
	while (info != NULL) {

		if (old_uid != info->uid)
		{
			old_uid = info->uid;
			internal_get_uuid_by_uid(info->uid, uuid);
		}

		idx = sqlite3_bind_parameter_index(stmt, ":uid");
		sqlite3_bind_int64(stmt, idx, info->uid);

		idx = sqlite3_bind_parameter_index(stmt, ":uuid");
		sqlite3_bind_text(stmt, idx, uuid, -1, SQLITE_TRANSIENT);

		idx = sqlite3_bind_parameter_index(stmt, ":biotype");
		sqlite3_bind_int64(stmt, idx, info->biotype);

		idx = sqlite3_bind_parameter_index(stmt, ":driver");
		sqlite3_bind_text(stmt, idx, info->driver, -1, SQLITE_TRANSIENT);

		idx = sqlite3_bind_parameter_index(stmt, ":idx");
		sqlite3_bind_int64(stmt, idx, info->index);

		idx = sqlite3_bind_parameter_index(stmt, ":idx_name");
		sqlite3_bind_text(stmt, idx, info->index_name, -1, SQLITE_TRANSIENT);

		idx = sqlite3_bind_parameter_index(stmt, ":sno");
		sqlite3_bind_int64(stmt, idx, sample->no);

		idx = sqlite3_bind_parameter_index(stmt, ":sdata");
		sqlite3_bind_text(stmt, idx, sample->data, -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -2;
		}
		sqlite3_reset(stmt);

		sample = sample->next;
		if (sample == NULL){
			info = info->next;
			if (info != NULL)
				sample = info->sample;
		}
	}

	sqlite3_finalize(stmt);
	return 0;
}

int bio_sto_clean_feature_info(sqlite3 *db, int uid, int biotype,
									   char *driver, int index_start,
									   int index_end)
{
	sqlite3_stmt *stmt = NULL;
	int ret = -1;
	int idx = -1;
	int l = 0;
	char *base_sql_str = "DELETE FROM [EIGEN_INFO] WHERE "
						 "EigenIndex >= :index_start";
	char *uid_sql_str = " AND UID = :uid ";
	char *biotype_sql_str = " AND BioType = :biotype ";
	char *driver_sql_str = " AND Driver = :driver ";
	char *idx_end_sql_str = " AND EigenIndex <= :index_end ";
	char *all_sql_str = NULL;

	l = strlen(base_sql_str) + strlen(uid_sql_str) + strlen(biotype_sql_str) +
			strlen(driver_sql_str) + strlen(idx_end_sql_str) + 1;
	all_sql_str = malloc(l);
	if (all_sql_str == NULL) {
		bio_print_error(_("Unable to allocate memory\n"));
		return -1;
	}
	memset(all_sql_str, 0, l);

	l = sprintf(all_sql_str, "%s", base_sql_str);
	if (uid != -1)
		l += sprintf(all_sql_str + l, "%s", uid_sql_str);
	if (biotype != -1)
		l += sprintf(all_sql_str + l, "%s", biotype_sql_str);
	if (driver != NULL && driver[0] != '\0')
		l += sprintf(all_sql_str + l, "%s", driver_sql_str);
	if (index_end != -1)
		l += sprintf(all_sql_str + l, "%s", idx_end_sql_str);
	bio_print_debug("Clean sql : %s\n", all_sql_str);

	ret = sqlite3_prepare_v2(db, all_sql_str, -1, &stmt, NULL);
	free(all_sql_str);
	if (ret != SQLITE_OK) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		return -2;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":uid");
	sqlite3_bind_int64(stmt, idx, uid);

	idx = sqlite3_bind_parameter_index(stmt, ":biotype");
	sqlite3_bind_int64(stmt, idx, biotype);

	idx = sqlite3_bind_parameter_index(stmt, ":driver");
	sqlite3_bind_text(stmt, idx, driver, -1, SQLITE_TRANSIENT);

	idx = sqlite3_bind_parameter_index(stmt, ":index_start");
	sqlite3_bind_int64(stmt, idx, index_start);

	idx = sqlite3_bind_parameter_index(stmt, ":index_end");
	sqlite3_bind_int64(stmt, idx, index_end);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -2;
	}

	sqlite3_finalize(stmt);
	return 0;
}

int bio_sto_update_feature_info_by_dbid(sqlite3 *db, unsigned long long dbid,
										int uid, int biotype, char *driver,
										int index, char * index_name,
										int sample_no)
{
	sqlite3_stmt *stmt = NULL;
	int ret = -1;
	int idx = -1;
	char *all_sql_str = "UPDATE [EIGEN_INFO] SET UID = :uid, BioType = :biotype, "
						"Driver = :driver, EigenIndex = :index, "
						"EigenIndexName = :index_name, SampleNo = :sample_no "
						"WHERE ID = :dbid";

	bio_print_debug("Update sql : %s\n", all_sql_str);

	ret = sqlite3_prepare_v2(db, all_sql_str, -1, &stmt, NULL);

	if (ret != SQLITE_OK) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		return -2;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":uid");
	sqlite3_bind_int64(stmt, idx, uid);

	idx = sqlite3_bind_parameter_index(stmt, ":biotype");
	sqlite3_bind_int64(stmt, idx, biotype);

	idx = sqlite3_bind_parameter_index(stmt, ":driver");
	sqlite3_bind_text(stmt, idx, driver, -1, SQLITE_TRANSIENT);

	idx = sqlite3_bind_parameter_index(stmt, ":index");
	sqlite3_bind_int64(stmt, idx, index);

	idx = sqlite3_bind_parameter_index(stmt, ":index_name");
	sqlite3_bind_text(stmt, idx, index_name, -1, SQLITE_TRANSIENT);

	idx = sqlite3_bind_parameter_index(stmt, ":sample_no");
	sqlite3_bind_int64(stmt, idx, sample_no);

	idx = sqlite3_bind_parameter_index(stmt, ":dbid");
	sqlite3_bind_int64(stmt, idx, dbid);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -2;
	}

	sqlite3_finalize(stmt);
	return 0;
}

int bio_sto_check_and_upgrade_db_format(sqlite3 *db)
{
	char sql_cmd[MAX_PATH_LEN] = {0};

	char **dbResult = NULL;
	char *errmsg = NULL;
	sqlite3_stmt *stmt = NULL;
	int ret = 0;
	int nRow = 0;
	int nColumn = 0;
	int formatMajor = 0;
	int formatMinor = 0;
	int formatFunc = 0;

	bio_print_debug(_("Check database format version ...\n"));

	snprintf(sql_cmd, MAX_PATH_LEN,
			 "SELECT name FROM sqlite_master WHERE type = 'table' AND name = '%s';",
			 TABLE_DATABASE_FORMAT);

	ret = sqlite3_get_table(db, sql_cmd, &dbResult, &nRow, &nColumn, &errmsg);
	if (ret != SQLITE_OK) {
		bio_print_error(_("Find Table '%s' Error: %s\n"),
						TABLE_DATABASE_FORMAT, sqlite3_errmsg(db));
		sqlite3_free(errmsg);
		sqlite3_free_table(dbResult);

		return -1;
	}
	sqlite3_free(errmsg);
	sqlite3_free_table(dbResult);

	if (nRow == 0)
	{
		formatMajor = 0;
		formatMinor = 0;
		formatFunc = 0;
	} else {
		snprintf(sql_cmd, MAX_PATH_LEN,
				 "SELECT * FROM %s;", TABLE_DATABASE_FORMAT);
		ret = sqlite3_prepare_v2(db, sql_cmd, -1, &stmt, NULL);
		if (ret != SQLITE_OK) {
			bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
			return -1;
		}

		if ( sqlite3_step(stmt) == SQLITE_ROW )
		{

			formatMajor = sqlite3_column_int(stmt, 1);
			formatMinor = sqlite3_column_int(stmt, 2);
			formatFunc = sqlite3_column_int(stmt, 3);
		}

		sqlite3_finalize(stmt);
	}
	bio_print_notice(_("Database format version is %d.%d.%d\n"),
					 formatMajor, formatMinor, formatFunc);

	if (formatMajor == 0 && formatMinor == 0 && formatFunc == 0)
	{
		ret = internal_upgrade_db_format_from_null(db);
		if (ret < 0)
		{
			bio_print_error(_("Failed to upgrade database format：version "
							  "%d.%d.%d to %d.%d.%d\n"),
							formatMajor, formatMinor, formatFunc,
							DATABASE_FORMAT_MAJOR, DATABASE_FORMAT_MINOR,
							DATABASE_FORMAT_FUNC);
			return -2;
		}

		return 0;
	}

	if (formatMajor == 1 && formatMinor == 1 && formatFunc >= 0)
	{
		bio_print_notice(_("The database format is compatible with the "
						   "current framework\n"));
		return 0;
	}

	bio_print_error(_("Incompatible version of the database format: %d.%d.%d. "
					  "Version required for the current framework: %d.%d.x"),
					formatMajor, formatMinor, formatFunc,
					DATABASE_FORMAT_MAJOR, DATABASE_FORMAT_MINOR,
					DATABASE_FORMAT_FUNC);

	return -1;
}

/****************** 内部函数实现 ******************/

/* 获取UTF-8的字符宽度 */
int internal_utf8_char_len(char firstByte)
{
	int offset = 1;

	if(firstByte & kFirstBitMask)
	{
		/* 第一个字节值超大于127，超出了ASCII码的范围 */

		if(firstByte & kThirdBitMask)
		{
			/* 第一个字节大于224，字符长度至少为3字节 */

			if(firstByte & kFourthBitMask)
				offset = 4; // 第一个字节大于240，字符长度为4字节
			else
				offset = 3;
		} else {
			offset = 2;
		}
	}
	return offset;
}

/* 获取UTF-8字符串长度 */
int internal_utf8_str_len(char *str)
{
	int len = 0;
	int l = 0;
	char *p = str;

	while (*p != '\0') {
		len ++;
		l = internal_utf8_char_len(*p);
		p = p + l;
	}
	return len;
}

/* 获取UTF-8字符串输出宽度 */
int internal_utf8_str_width(char *str)
{
	int len = 0;
	int l = 0;
	char *p = str;

	while (*p != '\0') {
		len ++;
		l = internal_utf8_char_len(*p);
		/* 特殊字符占用两个字符宽度 */
		if (l > 2)
			len ++;
		p = p + l;
	}
	return len;
}

/* 拷贝字符串 */
char * internal_newstr(char * old_str)
{
	char * str = NULL;

	if (old_str == NULL) {
		str = malloc(1);
		if (str != NULL)
			*str = '\0';
	} else {
		int l = strlen(old_str) + 1;
		str = malloc(l);
		if (str != NULL)
			strncpy(str, old_str, l);
	}

	return str;
}

/* 递归创建目录 */
int internal_create_dir(const char *new_dir)
{
	int i = 0;
	int len = 0;
	char dir_name[256];

	len = strlen(new_dir);
	strcpy(dir_name, new_dir);
	if (dir_name[len - 1] != '/')
		strcat(dir_name, "/");

	len = strlen(dir_name);

	for(i = 1; i < len; i++) {
		if (dir_name[i] == '/') {
			dir_name[i] = 0;
			if (access(dir_name, F_OK) != 0) {
				if (mkdir(dir_name, 0755) == -1) {
					bio_print_error(_("Create Biometric Datebase Directory (%s)"
									  " Error\n"), new_dir);
					return -1;
				}
			}

			dir_name[i] = '/';
		}
	}

	return 0;
}

/* 通过UID获取UUID */
void internal_get_uuid_by_uid(int uid, char * uuid)
{
	int ret;
	struct passwd *pwd = NULL;
	uuid_t uu;
	char user_conf_file[MAX_PATH_LEN] = {0};
	char uuid_buf[UUID_BUF_LEN] = {0};
	FILE *fd;

	pwd = getpwuid(uid);
	if (pwd == NULL)
	{
		snprintf(uuid, sizeof(UUID_DEFAULT), UUID_DEFAULT);
		return ;
	}

	if (access(pwd->pw_dir, F_OK) != 0)
	{
		snprintf(uuid, sizeof(UUID_DEFAULT), UUID_DEFAULT);
		return ;
	}

	snprintf(user_conf_file, MAX_PATH_LEN, "%s/.biometric_auth/", pwd->pw_dir);
	ret = access(user_conf_file, F_OK);
	if (ret != 0) {
			internal_create_dir(user_conf_file);
			ret = chown(user_conf_file, pwd->pw_uid, pwd->pw_gid);
			if (ret != 0)
				bio_print_error(_("Failure to modify the owner and group of "
								  "%s. ERROR[%d]: %s\n"), user_conf_file,
								errno, strerror(errno));
	}

	snprintf(user_conf_file, MAX_PATH_LEN,
			 "%s/.biometric_auth/UUID", pwd->pw_dir);
	ret = access(user_conf_file, F_OK);
	if (ret != 0) {
		// UUID文件不存在，则生成一个新的UUID，并保存到"~/.biometric_auth/UUID"中
		uuid_generate(uu);
		uuid_unparse_lower(uu, uuid);

		fd = fopen(user_conf_file, "w");
		fprintf(fd, "%s\n", uuid);
		fclose(fd);

		ret = chown(user_conf_file, pwd->pw_uid, pwd->pw_gid);
		if (ret != 0)
			bio_print_error(_("Failure to modify the owner and group of %s. "
							  "ERROR[%d]: %s\n"), user_conf_file,
							errno, strerror(errno));
	} else {
		char * ret_string = NULL;
		fd = fopen(user_conf_file, "r");
		ret_string = fgets(uuid_buf, UUID_BUF_LEN, fd);
		fclose(fd);

		if (ret_string == NULL)
			ret = -1;
		else
			ret = uuid_parse(uuid_buf, uu);

		// 读取或者解析UUID失败，则代表UUID无效，重新生成一个新的UUID
		if (ret != 0)
		{
			uuid_generate(uu);
			uuid_unparse_lower(uu, uuid);

			fd = fopen(user_conf_file, "w");
			fprintf(fd, "%s\n", uuid);
			fclose(fd);

			ret = chown(user_conf_file, pwd->pw_uid, pwd->pw_gid);
			if (ret != 0)
				bio_print_error(_("Failure to modify the owner and group of "
								  "%s. ERROR[%d]: %s\n"), user_conf_file,
								errno, strerror(errno));
		} else {
			uuid_unparse_lower(uu, uuid);
		}
	}
}

/* 升级数据库，从最原始版本到当前版本 */
int internal_upgrade_db_format_from_null(sqlite3 *db)
{
	sqlite3_stmt *stmt = NULL;

	int uid = 0;
	int ret = 0;
	char uuid[UUID_BUF_LEN] = {0};
	char sql_cmd[MAX_PATH_LEN] = {0};

	bio_print_notice(_("Upgrade database ...\n"));

	// 开启事物模式
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

	// 重命名原始表，重命名后备份不删除，以防数据损坏时无法修复
	snprintf(sql_cmd, MAX_PATH_LEN,
			 "ALTER TABLE EIGEN_INFO RENAME TO EIGEN_INFO_VERSION_0_0_0;");
	ret = sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("Rename Table 'EIGEN_INFO' to "
						  "'EIGEN_INFO_VERSION_0_0_0' Error: %s\n"),
						sqlite3_errmsg(db));

		// 回滚数据
		sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

		return -1;
	}

	// 创建当前格式的 EIGEN_INFO 表
	ret = internal_create_eigen_info_table(db);
	if (ret != 0)
	{
		// 回滚数据
		sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

		return -1;
	}

	// 迁移数据
	snprintf(sql_cmd, MAX_PATH_LEN,
			 "INSERT INTO EIGEN_INFO (ID, UID, BioType, Driver, EigenIndex, "
			 "  EigenIndexName, SampleNo, EigenData) "
			 "SELECT ID, UID, BioType, Driver, "
			 "  EigenIndex, EigenIndexName, SampleNo, EigenData "
			 "FROM EIGEN_INFO_VERSION_0_0_0;");
	ret = sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("Failed to migrate data from the original table: %s\n"),
						sqlite3_errmsg(db));

		// 回滚数据
		sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

		return -1;
	}

	// 更新UUID
	snprintf(sql_cmd, MAX_PATH_LEN,
			 "SELECT UID FROM  %s GROUP BY UID order by UID;",
			 TABLE_EIGEN_INFO);
	ret = sqlite3_prepare_v2(db, sql_cmd, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));

		// 回滚数据
		sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

		return -1;
	}

	while ( sqlite3_step(stmt) == SQLITE_ROW )
	{
		char sql_cmd[MAX_PATH_LEN] = {0};
		uid = sqlite3_column_int(stmt, 0);
		internal_get_uuid_by_uid(uid, uuid);
		printf("UID = %d, UUID = %s\n", uid, uuid);

		bio_print_debug(_("Update UUID=%s to database(UID=%d) ...\n"), uuid, uid);
		snprintf(sql_cmd, MAX_PATH_LEN,
				 "UPDATE %s SET UUID = \"%s\" WHERE UID = %d;",
				 TABLE_EIGEN_INFO, uuid, uid);

		ret = sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
		if (ret != SQLITE_OK) {
			bio_print_error(_("Update column \"UUID\" to Table \"%s\" "
							  "Error: %s\n"),
							TABLE_EIGEN_INFO, sqlite3_errmsg(db));

			// 回滚数据
			sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

			return -1;
		}
	}
	sqlite3_finalize(stmt);

	ret = internal_create_datebase_info(db);
	if (ret < 0)
	{
		// 回滚数据
		sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);

		return -1;
	}

	// 迁移完成，关闭事物模式，提交数据
	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

	return 0;
}

int internal_create_eigen_info_table(sqlite3 *db)
{
	int ret;
	char *sql_cmd = NULL;

	sql_cmd = "CREATE TABLE IF NOT EXISTS " TABLE_EIGEN_INFO "( " \
			  "	ID				INTEGER	PRIMARY KEY AUTOINCREMENT NOT NULL," \
			  "	UID				INTEGER	NOT NULL," \
			  "	UUID			TEXT," \
			  "	BioType			INTEGER	NOT NULL," \
			  "	Driver			TEXT	NOT NULL," \
			  "	EigenIndex		INTEGER	NOT NULL," \
			  "	EigenIndexName	TEXT	NOT NULL," \
			  "	SampleNo		INTEGER	NOT NULL," \
			  "	EigenData		TEXT	NOT NULL" \
			  ")";
	bio_print_info(_("Create Table %s:\n%s\n"), TABLE_EIGEN_INFO, sql_cmd);

	ret = sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("Create Table '%s' Error: %s\n"), TABLE_EIGEN_INFO,
						sqlite3_errmsg(db));
		return -1;
	}

	return 0;
}

int internal_create_datebase_info(sqlite3 *db)
{
	int ret, idx;
	char *sql_cmd = NULL;
	sqlite3_stmt *stmt = NULL;

	sql_cmd = "CREATE TABLE IF NOT EXISTS " TABLE_DATABASE_FORMAT "( " \
			  "	ID				INTEGER	PRIMARY KEY AUTOINCREMENT NOT NULL," \
			  "	FormatMajor		INTEGER	NOT NULL," \
			  "	FormatMinor		INTEGER	NOT NULL," \
			  "	FormatFunction	INTEGER	NOT NULL" \
			  ")";
	bio_print_info(_("Create Table %s:\n%s\n"), TABLE_DATABASE_FORMAT, sql_cmd);

	ret = sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("Create Table '%s' Error: %s\n"),
						TABLE_DATABASE_FORMAT, sqlite3_errmsg(db));
		return -1;
	}

	sql_cmd = "INSERT INTO [" TABLE_DATABASE_FORMAT "] (ID, FormatMajor, "
			  " FormatMinor, FormatFunction)  VALUES (NULL, :FormatMajor, "
			  " :FormatMinor, :FormatFunction);";
	bio_print_info(_("Set Database Format(%d.%d.%d):\n%s\n"),
				   DATABASE_FORMAT_MAJOR,
				   DATABASE_FORMAT_MINOR,
				   DATABASE_FORMAT_FUNC,
				   sql_cmd);

	ret = sqlite3_prepare_v2(db, sql_cmd, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		return -1;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":FormatMajor");
	sqlite3_bind_int64(stmt, idx, DATABASE_FORMAT_MAJOR);

	idx = sqlite3_bind_parameter_index(stmt, ":FormatMinor");
	sqlite3_bind_int64(stmt, idx, DATABASE_FORMAT_MINOR);

	idx = sqlite3_bind_parameter_index(stmt, ":FormatFunction");
	sqlite3_bind_int64(stmt, idx, DATABASE_FORMAT_FUNC);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		bio_print_error(_("sqlite3 prepare err: %s\n"), sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return -1;
	}
	sqlite3_finalize(stmt);

	return 0;
}

int bio_sto_create_table(sqlite3 *db)
{
	int ret;

	ret = internal_create_eigen_info_table(db);
	if (ret != 0)
		return -1;

	ret = internal_create_datebase_info(db);
	if (ret != 0)
		return -1;

	return 0;
}

