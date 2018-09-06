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

#ifndef STROGE_H
#define STROGE_H

#include <sqlite3.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define _STR(s)	#s
#define DEF2STR(s)	_STR(s)

#ifdef BIO_DB_DIR
	#define DB_DIR	DEF2STR(BIO_DB_DIR)
#else
	#define DB_DIR	"/var/lib/biometric-auth/"
#endif

#ifdef BIO_DB_NAME
	#define DB_NAME	DEF2STR(BIO_DB_NAME)
#else
	#define DB_NAME	"biometric.db"
#endif

/* 定义数据库名称和数据库中表的名称 */
#define DB_PATH					DB_DIR DB_NAME
#define TABLE_EIGEN_INFO		"EIGEN_INFO"
#define TABLE_DATABASE_FORMAT	"DATABASE_FORMAT"

/* 定义数据库格式的版本号 */
#define DATABASE_FORMAT_MAJOR	1
#define DATABASE_FORMAT_MINOR	1
#define DATABASE_FORMAT_FUNC	0

/*
 * 特征采样数据链表
 *   记录单个特征部位的单次特征采样值的链表
 */
typedef struct feature_sample_t feature_sample;
struct feature_sample_t
{
	unsigned long long dbid;	/** 数据库中采样的ID（主键，自增长） **/
	int no;						/** 采样的序号 */
	char *data;					/** 采样的数据 */
	feature_sample *next;		/** 链表中下一个采样值 */
};

/*
 * 各特征部位信息链表
 *   记录单个特征部位详细信息的链表
 */
typedef struct feature_info_t feature_info;
struct feature_info_t
{
	int	uid;					/** 特征部位所属的用户ID */
	int biotype;				/** 特征部位的类型 */
	char *driver;				/** 特征对应的驱动名 */
	int	index;					/** 特征部位的索引值 */
	char *index_name;			/** 特征部位的名称，由用户设置 */
	feature_sample *sample;		/** 特征部位的采样值链表 */
	feature_info *next;			/** 链表中的下一个特征部位信息 */
};


/* 创建一份指定字符串的拷贝，返回拷贝地址 */
char * bio_sto_new_str(char * old_str);

/* 输出特征信息结构体 */
void print_feature_info(feature_info * info_list);

/*
 * 连接数据库
 *   连接数据库，数据库位置由DB_NAME宏指定
 *
 * @return	返回数据库句柄
 * @retval	NULL	连接失败
 *			其他		连接成功
 */
sqlite3 *bio_sto_connect_db();

/*
 * 断开数据库连接
 *
 * @param[in]	sqlite3 *db	需要关闭的数据库句柄
 * @return	无
 */
void bio_sto_disconnect_db(sqlite3 *db);

/*
 * 创建新的采样结构体
 *
 * @param[in]	no		采样编号
 * @param[in]	data	采样的数据
 * @return	返回采样结构体指针，NULL创建失败
 * @note	创建的采样结构体需要通过bio_sto_free_feature_sample释放
 */
feature_sample * bio_sto_new_feature_sample(int no, char * data);

/* 回收单项采样数据 */

/*
 * 回收单项采样数据
 *   回收单项采样数据空间资源
 *
 * @param[in]	sample	单项采样数据
 * @return	无
 */
void bio_sto_free_feature_sample(feature_sample * sample);

/* 回收采样数据链表 */
/*
 * 回收采样数据链表
 *   回收采样数据链表空间资源，一般由bio_sto_free_feature_info函数调用
 *
 * @param[in]	sample_list	采样数据的链表
 */
void bio_sto_free_feature_sample_list(feature_sample * sample_list);

/*
 * 创建新的特征结构体
 *
 * @param[in]	no		采样编号
 * @param[in]	data	采样的数据
 * @return		返回采样结构体指针，NULL创建失败
 * @note	创建的采样结构体需要通过bio_sto_free_feature_sample释放
 */
feature_info * bio_sto_new_feature_info(int	uid,
										int biotype,
										char *driver,
										int	index,
										char *index_name);
/*
 * 回收单个特征数据项
 *
 * @param[in]	info	单个特征数据项
 */
void bio_sto_free_feature_info(feature_info * info);

/*
 * 回收特征数据链表
 *
 * @param[in]	info_list	特征数据的链表
 */
void bio_sto_free_feature_info_list(feature_info * info_list);


/*
 * 获取特征信息链表
 *   获取指定范围内的特征信息数据（全闭区间[start, end]）
 *
 * @param[in]	db			指定数据库
 * @param[in]	uid			指定用户ID，-1代表获取所有用户
 * @param[in]	biotype		指定生物特征类型，-1代表获取所有生物类型
 * @param[in]	driver		指定驱动名，NULL代表获取所有驱动的特征类型
 * @param[in]	index_start	指定开始的索引
 * @param[in]	index_end	指定结束的索引，-1代表到最后
 * @return	返回特征信息链表，NULL代表在指定区间无特征存放
 * @note	获取的特征信息链表需要手动释放
 * @par 修改日志
 * JianglinXuan于2017-05-17创建
 */
feature_info *bio_sto_get_feature_info(sqlite3 *db,
									   int uid,
									   int biotype,
									   char *driver,
									   int index_start,
									   int index_end);

/*
 * 保存特征信息链表
 *   保存给定的特征信息数据
 *
 * @param[in]	db			指定数据库
 * @param[in]	info_list	需要保存的特征信息链表
 * @return	保存结果
 * @note	获取的特征信息链表需要手动释放
 */
int bio_sto_set_feature_info(sqlite3 *db, feature_info * info_list);

/*
 * 删除特征信息
 *   参数指定范围内的特征信息数据（全闭区间[start, end]）
 *
 * @param[in]	db			指定数据库
 * @param[in]	uid			指定用户ID，-1代表删除所有用户的指定特征信息
 * @param[in]	biotype		指定生物特征类型，-1代表删除所有生物类型特征信息
 * @param[in]	driver		指定驱动名，NULL代表获取所有驱动的特征类型
 * @param[in]	index_start	指定开始的索引
 * @param[in]	index_end	指定结束的索引，-1代表到最后
 * @return		删除操作的结果
 * @retval	0	操作成功
 */
int bio_sto_clean_feature_info(sqlite3 *db, int uid, int biotype,
									   char *driver, int index_start,
									   int index_end);

/*
 * 更新特征信息
 *   参数指定特征的ID及其更新值
 *
 * @param[in]	db			指定数据库
 * @param[in]	id			指定特征ID
 * @param[in]	uid			更新的用户ID
 * @param[in]	biotype		更新的生物特征类型
 * @param[in]	driver		指定驱动名
 * @param[in]	index		更新的索引编号
 * @param[in]	index_name	更新的索引编号
 * @param[in]	sample_no	更新的采样编号
 * @return		更新操作的结果
 * @retval	0	操作成功
 */
int bio_sto_update_feature_info_by_dbid(sqlite3 *db, unsigned long long dbid,
										int uid, int biotype, char *driver,
										int index, char * index_name,
										int sample_no);

/*
 * 检测并升级数据库
 *   检测数据库格式，并升级低版本的数据库格式
 *
 * @param[in]	db	指定数据库
 * @return		操作的结果
 * @retval	0	数据库格式兼容，或数据库格式已成功升级
 *			-1	检测数据库格式失败，或数据库格式不兼容且无法升级
 *			-2	升级数据库失败
 */
int bio_sto_check_and_upgrade_db_format(sqlite3 *db);

#ifdef  __cplusplus
}
#endif

#endif // STROGE_H
