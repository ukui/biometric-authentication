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

#ifndef BIOMETRIC_COMM_H
#define BIOMETRIC_COMM_H

#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <glib.h>
#include <libusb.h>
#include <pthread.h>

#include "biometric_storage.h"

#include <libfprint-2/fprint.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* 定义动态驱动ID的起止范围 */
#define BIO_DRVID_STATIC_MAX		99
#define BIO_DRVID_DYNAMIC_START		(BIO_DRVID_STATIC_MAX + 1)
#define BIO_DRVID_DYNAMIC_MAX		1023

/* 定义输出消息的等级，与Linux内核中printk的等级定义一致 */
#define BIO_PRINT_LEVEL_EMERG	0
#define BIO_PRINT_LEVEL_ALERT	1
#define BIO_PRINT_LEVEL_CRIT	2
#define BIO_PRINT_LEVEL_ERROR	3	// 错误信息
#define BIO_PRINT_LEVEL_WARN	4	// 警告信息
#define BIO_PRINT_LEVEL_NOTICE	5	// 提示信息
#define BIO_PRINT_LEVEL_INFO	6	// 运行时信息
#define BIO_PRINT_LEVEL_DEBUG	7	// 调试信息

/* 定义输出控制等级及开关（环境变量） */
#define BIO_PRINT_LEVEL_DEFAULT	BIO_PRINT_LEVEL_NOTICE	// 默认输出信息等级
#define BIO_PRINT_LEVEL_ENV		"BIO_PRINT_LEVEL"	// 控制信息等级的环境变量
#define BIO_PRINT_LEVEL_COLOR	"BIO_PRINT_COLOR"	// 控制信息颜色的环境变量

/* 定义输出消息的颜色 */
#define BIO_PRINT_COLOR_BLACK	"\033[01;30m"
#define BIO_PRINT_COLOR_RED		"\033[01;31m"
#define BIO_PRINT_COLOR_GREEN	"\033[01;32m"
#define BIO_PRINT_COLOR_YELLOW	"\033[01;33m"
#define BIO_PRINT_COLOR_BLUE	"\033[01;34m"
#define BIO_PRINT_COLOR_PURPLE	"\033[01;35m"
#define BIO_PRINT_COLOR_CYAN	"\033[01;36m"
#define BIO_PRINT_COLOR_WHITE	"\033[01;36m"
#define BIO_PRINT_COLOR_END		"\033[0m"

/* 定义文件路径最大长度 */
#define MAX_PATH_LEN 1024

/* 定义默认的操作超时时间 */
#define BIO_OPS_DEFAULT_TIMEOUT 30

/* 版本信息 */
typedef struct api_version_t api_version;
struct api_version_t
{
	int major;
	int minor;
	int function;
};

/* 生物特征类型 */
typedef enum {
	BioT_FingerPrint,		/** 指纹 **/
	BioT_FingerVein,		/** 指静脉 **/
	BioT_Iris,				/** 虹膜 **/
	BioT_Face,				/** 人脸 **/
	BioT_VoicePrint,		/** 声纹 **/
}BioType;

/* 存储方式 */
typedef enum {
	StoT_Device,			/** 存储在硬件中 **/
	StoT_OS,				/** 存储在操作系统中 **/
	StoT_Mix,
}StoType;

/* 特征值的类型 */
typedef enum {
	EigT_Data,				/** 原始数据 **/
	EigT_Eigenvalue,		/** 特征值 **/
	EigT_Eigenvector,		/** 特征向量 **/
}EigType;

/* 验证方式 */
typedef enum {
	VerT_Hardware,			/** 硬件验证 **/
	VerT_Software,			/** 软件验证 **/
	VerT_Mix,				/** 混合验证 **/
	VerT_Other
}VerType;

/* 识别方式 */
typedef enum {
	IdT_Hardware,			/** 硬件识别 **/
	IdT_Software,			/** 软件识别 **/
	IdT_Mix,				/** 混合识别 **/
	IdT_Other
}IdType;

/* 设备的总线类型 */
typedef enum {
	BusT_Serial,			/** 串口 **/
	BusT_USB,				/** USB接口 **/
	BusT_PCIE,				/** PCIE接口 **/
	BusT_Any = 100,			/** 任意总线类型，即不限总线类型 **/
	BusT_Other,				/** 其他总线类型 **/
}BusType;

/* 操作定义，老旧接口，将会在未来版本中移除 */
typedef enum {
	ACTION_START,			/** 开始操作 **/
	ACTION_STOP,			/** 中断操作 **/
}OpsActions;

/* USB设备热插拔行为 */
typedef enum {
	USB_HOT_PLUG_ATTACHED = 1,			/** USB设备接入 **/
	USB_HOT_PLUG_DETACHED = -1,			/** USB设备拔出 **/
}USBHotPlugActions;

/* 串口设备相关信息 */
struct Serial_Info {
	int fd;
	char path[MAX_PATH_LEN];
	unsigned long driver_data;
};

/* USB设备硬件信息 */
struct usb_id {
	uint16_t idVendor;
	uint16_t idProduct;
	char * description;
};

/* USB设备额外的驱动信息 */
struct USB_INFO {
	const struct usb_id * id_table;
	libusb_hotplug_callback_handle callback_handle[2];
	unsigned long driver_data;
};

/* PCI设备硬件信息 */
struct pci_id {
	uint16_t idVendor;
	uint16_t idProduct;
	char * description;
};

/* PCI设备额外的驱动信息 */
struct PCI_INFO {
	const struct pci_id * const id_table;
};

/* 生物特征识别设备的信息汇总 */
struct BioInfo{
	BioType biotype;		/** 生物特征类型 **/
	StoType stotype;		/** 存储类型 **/
	EigType eigtype;		/** 特征类型 **/
	VerType vertype;		/** 验证类型 **/
	IdType idtype;			/** 识别类型 **/
	BusType bustype;		/** 总线类型 **/
	void *biodata;			/** 具体各特征私有信息 **/
};

/* 定义操作类型 */
typedef enum {
	OPS_TYPE_COMM = 0,
	OPS_TYPE_OPEN,
	OPS_TYPE_ENROLL,
	OPS_TYPE_VERIFY,
	OPS_TYPE_IDENTIFY,
	OPS_TYPE_CAPTURE,
	OPS_TYPE_SEARCH,
	OPS_TYPE_CLEAN,
	OPS_TYPE_GET_FLIST,
	OPS_TYPE_RENAME,
    OPS_TYPE_CLOSE,
    OPS_TYPE_ENROLL_PROCESS,
}BioOpsType;

/*
 * 定义设备当前的状态
 * 一般来说，设备只会处于两种状态：设备正在处理中，设备空闲
 */
typedef enum {
	DEVS_COMM_IDLE = OPS_TYPE_COMM * 100,	/** 空闲状态 **/
	DEVS_COMM_DOING,						/** 设备正在处理中 **/
	DEVS_COMM_STOP_BY_USER,					/** 设备操作被终止 **/
	DEVS_COMM_DISABLE,						/** 设备被禁用 **/
	DEVS_COMM_MAX,

	DEVS_OPEN_DOING = OPS_TYPE_OPEN * 100 + 1,	/** 正在打开设备 **/
	DEVS_OPEN_STOP_BY_USER,						/** 打开操作被终止 **/
	DEVS_OPEN_MAX,

	DEVS_ENROLL_DOING = OPS_TYPE_ENROLL * 100 + 1,	/** 正在录入 **/
	DEVS_ENROLL_STOP_BY_USER,						/** 打开操作被终止 **/
	DEVS_ENROLL_MAX,

	DEVS_VERIFY_DOING = OPS_TYPE_VERIFY * 100 + 1,	/** 正在认证 **/
	DEVS_VERIFY_STOP_BY_USER,						/** 打开操作被终止 **/
	DEVS_VERIFY_MAX,

	DEVS_IDENTIFY_DOING = OPS_TYPE_IDENTIFY * 100 + 1,	/** 正在识别指定特征 **/
	DEVS_IDENTIFY_STOP_BY_USER,							/** 打开操作被终止 **/
	DEVS_IDENTIFY_MAX,

	DEVS_CAPTURE_DOING = OPS_TYPE_CAPTURE * 100 + 1,	/** 正在捕获信息 **/
	DEVS_CAPTURE_STOP_BY_USER,							/** 打开操作被终止 **/
	DEVS_CAPTURE_MAX,

	DEVS_SEARCH_DOING = OPS_TYPE_SEARCH * 100 + 1,	/** 正在搜索指定特征 **/
	DEVS_SEARCH_STOP_BY_USER,						/** 打开操作被终止 **/
	DEVS_SEARCH_MAX,

	DEVS_CLEAN_DOING = OPS_TYPE_CLEAN * 100 + 1,	/** 正在清理特征数据 **/
	DEVS_CLEAN_STOP_BY_USER,						/** 打开操作被终止 **/
	DEVS_CLEAN_MAX,

	DEVS_GET_FLIST_DOING = OPS_TYPE_GET_FLIST * 100 + 1,	/** 正在获取特征列表 **/
	DEVS_GET_FLIST_STOP_BY_USER,							/** 获取特征列表操作被终止 **/
	DEVS_GET_FLIST_MAX,

	DEVS_RENAME_DOING = OPS_TYPE_RENAME * 100 + 1,	/** 正在重命名特征 **/
	DEVS_RENAME_STOP_BY_USER,						/** 重命名特征操作被终止 **/
	DEVS_RENAME_MAX,

	DEVS_CLOSE_DOING = OPS_TYPE_CLOSE * 100 + 1,	/** 正在关闭设备 **/
	DEVS_CLOSE_STOP_BY_USER,						/** 打开操作被终止 **/
	DEVS_CLOSE_MAX,
}DevStatus;

/*
 * 定义各种操作结果
 */
typedef enum {
	OPS_COMM_SUCCESS = OPS_TYPE_COMM * 100,	/** 空闲状态 **/
	OPS_COMM_FAIL,							/** 操作失败 **/
	OPS_COMM_NO_MATCH = OPS_COMM_FAIL,		/** 不匹配 **/
	OPS_COMM_ERROR,							/** 通用操作错误 **/
	OPS_COMM_STOP_BY_USER,					/** 用户取消 **/
	OPS_COMM_TIMEOUT,						/** 操作超时 **/
	OPS_COMM_OUT_OF_MEM,					/** 无法分配内存 **/
	OPS_COMM_MAX,

	OPS_OPEN_SUCCESS = OPS_TYPE_OPEN * 100,	/** 打开设备完成 **/
	OPS_OPEN_FAIL,							/** 打开设备失败 **/
	OPS_OPEN_ERROR,							/** 打开设备遇到错误 **/
	OPS_OPEN_MAX,

	OPS_ENROLL_SUCCESS = OPS_TYPE_ENROLL * 100,	/** 录入信息成功 **/
	OPS_ENROLL_FAIL,							/** 录入失败 **/
	OPS_ENROLL_ERROR,							/** 录入过程中遇到错误 **/
	OPS_ENROLL_STOP_BY_USER,					/** 录入被用户中断 **/
	OPS_ENROLL_TIMEOUT,							/** 操作超时 **/
	OPS_ENROLL_MAX,

	OPS_VERIFY_MATCH = OPS_TYPE_VERIFY * 100,	/** 认证匹配 **/
	OPS_VERIFY_NO_MATCH,						/** 认证不匹配 **/
	OPS_VERIFY_ERROR,							/** 认证过程中遇到错误 **/
	OPS_VERIFY_STOP_BY_USER,					/** 认证被用户中断 **/
	OPS_VERIFY_TIMEOUT,							/** 操作超时 **/
	OPS_VERIFY_MAX,

	OPS_IDENTIFY_MATCH = OPS_TYPE_IDENTIFY * 100,	/** 识别到指定特征 **/
	OPS_IDENTIFY_NO_MATCH,							/** 未识别出指定特征 **/
	OPS_IDENTIFY_ERROR,								/** 识别过程中遇到错误 **/
	OPS_IDENTIFY_STOP_BY_USER,						/** 识别被用户中断 **/
	OPS_IDENTIFY_TIMEOUT,							/** 操作超时 **/
	OPS_IDENTIFY_MAX,

	OPS_CAPTURE_SUCCESS = OPS_TYPE_CAPTURE * 100,	/** 捕获成功 **/
	OPS_CAPTURE_FAIL,								/** 捕获失败 **/
	OPS_CAPTURE_ERROR,								/** 捕获过程中遇到错误 **/
	OPS_CAPTURE_STOP_BY_USER,						/** 捕获被用户中断 **/
	OPS_CAPTURE_TIMEOUT,							/** 操作超时 **/
	OPS_CAPTURE_MAX,

	OPS_SEARCH_MATCH = OPS_TYPE_SEARCH * 100,	/** 搜索到指定特征 **/
	OPS_SEARCH_NO_MATCH,						/** 未搜索到指定特征 **/
	OPS_SEARCH_ERROR,							/** 搜索过程中遇到错误 **/
	OPS_SEARCH_STOP_BY_USER,					/** 搜索被用户中断 **/
	OPS_SEARCH_TIMEOUT,							/** 操作超时 **/
	OPS_SEARCH_MAX,

	OPS_CLEAN_SUCCESS = OPS_TYPE_CLEAN * 100,	/** 清理特征成功 **/
	OPS_CLEAN_FAIL,								/** 清理失败 **/
	OPS_CLEAN_ERROR,							/** 清理过程中遇到错误 **/
	OPS_CLEAN_STOP_BY_USER,						/** 清理被用户中断 **/
	OPS_CLEAN_TIMEOUT,							/** 操作超时 **/
	OPS_CLEAN_MAX,

	OPS_GET_FLIST_SUCCESS = OPS_TYPE_GET_FLIST * 100,	/** 获取特征列表完成 **/
	OPS_GET_FLIST_FAIL,									/** 获取特征列表失败 **/
	OPS_GET_FLIST_ERROR,								/** 获取特征列表过程中遇到错误 **/
	OPS_GET_FLIST_STOP_BY_USER,							/** 获取特征列表被用户中断 **/
	OPS_GET_FLIST_TIMEOUT,								/** 获取特征列表超时 **/
	OPS_GET_FLIST_MAX,

	OPS_RENAME_SUCCESS = OPS_TYPE_RENAME * 100,	/** 重命名特征完成 **/
	OPS_RENAME_FAIL,							/** 重命名特征失败 **/
	OPS_RENAME_ERROR,							/** 重命名特征过程中遇到错误 **/
	OPS_RENAME_STOP_BY_USER,					/** 重命名特征被用户中断 **/
	OPS_RENAME_TIMEOUT,							/** 重命名特征超时 **/
	OPS_RENAME_MAX,

	OPS_CLOSE_SUCCESS = OPS_TYPE_CLOSE * 100,	/** 关闭设备完成 **/
	OPS_CLOSE_FAIL,								/** 关闭设备失败 **/
	OPS_CLOSE_ERROR,							/** 关闭设备过程中遇到错误 **/
	OPS_CLOSE_MAX,
}OpsResult;

/*
 * 定义各种面向用户的提示消息
 */
typedef enum {
	NOTIFY_COMM_IDLE = OPS_TYPE_COMM * 100,		/** 空闲状态 **/
	NOTIFY_COMM_SUCCESS = NOTIFY_COMM_IDLE,		/** 操作成功 **/
	NOTIFY_COMM_FAIL,							/** 操作失败 **/
	NOTIFY_COMM_NO_MATCH = NOTIFY_COMM_FAIL,	/** 不匹配 **/
	NOTIFY_COMM_ERROR,							/** 操作过程中遇到错误 **/
	NOTIFY_COMM_STOP_BY_USER,					/** 用户中断 **/
	NOTIFY_COMM_TIMEOUT,						/** 操作超时 **/
	NOTIFY_COMM_DISABLE,						/** 设备不可用 **/
	NOTIFY_COMM_OUT_OF_MEM,
	NOTIFY_COMM_UNSUPPORTED_OPS,				/** 不支持的操作 **/
	NOTIFY_COMM_MAX,

	NOTIFY_OPEN_SUCCESS = OPS_TYPE_OPEN * 100,	/** 打开设备完成 **/
	NOTIFY_OPEN_FAIL,							/** 打开设备失败 **/
	NOTIFY_OPEN_ERROR,							/** 打开设备过程中遇到错误 **/
	NOTIFY_OPEN_MAX,

	NOTIFY_ENROLL_SUCCESS = OPS_TYPE_ENROLL * 100,	/** 录入信息成功 **/
	NOTIFY_ENROLL_FAIL,								/** 录入失败 **/
	NOTIFY_ENROLL_ERROR,							/** 录入过程中遇到错误 **/
	NOTIFY_ENROLL_STOP_BY_USER,						/** 录入被用户中断 **/
	NOTIFY_ENROLL_TIMEOUT,							/** 操作超时 **/
	NOTIFY_ENROLL_MAX,

	NOTIFY_VERIFY_MATCH = OPS_TYPE_VERIFY * 100,	/** 认证匹配 **/
	NOTIFY_VERIFY_NO_MATCH,							/** 认证不匹配 **/
	NOTIFY_VERIFY_ERROR,							/** 认真过程中遇到错误 **/
	NOTIFY_VERIFY_STOP_BY_USER,						/** 认证被用户中断 **/
	NOTIFY_VERIFY_TIMEOUT,							/** 操作超时 **/
	NOTIFY_VERIFY_MAX,

	NOTIFY_IDENTIFY_MATCH = OPS_TYPE_IDENTIFY * 100,	/** 识别到指定特征 **/
	NOTIFY_IDENTIFY_NO_MATCH,							/** 未识别出指定特征 **/
	NOTIFY_IDENTIFY_ERROR,								/** 识别过程中遇到错误 **/
	NOTIFY_IDENTIFY_STOP_BY_USER,						/** 识别被用户中断 **/
	NOTIFY_IDENTIFY_TIMEOUT,							/** 操作超时 **/
	NOTIFY_IDENTIFY_MAX,

	NOTIFY_CAPTURE_SUCCESS = OPS_TYPE_CAPTURE * 100,	/** 捕获成功 **/
	NOTIFY_CAPTURE_FAIL,								/** 捕获失败 **/
	NOTIFY_CAPTURE_ERROR,								/** 捕获过程中遇到错误 **/
	NOTIFY_CAPTURE_STOP_BY_USER,						/** 捕获被用户中断 **/
	NOTIFY_CAPTURE_TIMEOUT,								/** 操作超时 **/
	NOTIFY_CAPTURE_MAX,

	NOTIFY_SEARCH_MATCH = OPS_TYPE_SEARCH * 100,	/** 搜索到指定特征 **/
	NOTIFY_SEARCH_NO_MATCH,							/** 未搜索到指定特征 **/
	NOTIFY_SEARCH_ERROR,							/** 搜索过程中遇到错误 **/
	NOTIFY_SEARCH_STOP_BY_USER,						/** 搜索被用户中断 **/
	NOTIFY_SEARCH_TIMEOUT,							/** 操作超时 **/
	NOTIFY_SEARCH_MAX,

	NOTIFY_CLEAN_SUCCESS = OPS_TYPE_CLEAN * 100,	/** 清理特征成功 **/
	NOTIFY_CLEAN_FAIL,								/** 清理失败 **/
	NOTIFY_CLEAN_ERROR,								/** 清理过程中遇到错误 **/
	NOTIFY_CLEAN_STOP_BY_USER,						/** 清理被用户中断 **/
	NOTIFY_CLEAN_TIMEOUT,							/** 操作超时 **/
	NOTIFY_CLEAN_MAX,

	NOTIFY_GET_FLIST_SUCCESS = OPS_TYPE_GET_FLIST * 100,	/** 获取特征列表完成 **/
	NOTIFY_GET_FLIST_FAIL,									/** 获取特征列表失败 **/
	NOTIFY_GET_FLIST_ERROR,									/** 获取特征列表过程中遇到错误 **/
	NOTIFY_GET_FLIST_STOP_BY_USER,							/** 获取特征列表被用户中断 **/
	NOTIFY_GET_FLIST_TIMEOUT,								/** 获取特征列表超时 **/
	NOTIFY_GET_FLIST_MAX,

	NOTIFY_RENAME_SUCCESS = OPS_TYPE_RENAME * 100,		/** 重命名特征完成 **/
	NOTIFY_RENAME_FAIL,									/** 重命名特征失败 **/
	NOTIFY_RENAME_ERROR,								/** 重命名特征过程中遇到错误 **/
	NOTIFY_RENAME_INCOMPLETE,							/** 重命名特征不完全 **/

	NOTIFY_CLOSE_SUCCESS = OPS_TYPE_CLOSE * 100,		/** 关闭设备完成 **/
	NOTIFY_CLOSE_FAIL,									/** 关闭设备失败 **/
	NOTIFY_CLOSE_ERROR,									/** 关闭设备过程中遇到错误 **/
	NOTIFY_CLOSE_MAX,
}NotifyMID;

//typedef enum {
//    ENROLL_PROCESS_START = OPS_TYPE_ENROLL_PROCESS,         /** 未按压手指，即录入阶段为0% **/
//    ENROLL_PROCESS_ONE,                                     /** 第一次按压手指，即录入阶段为20% **/
//    ENROLL_PROCESS_TWO,                                     /** 第二次按压手指，即录入阶段为40% **/
//    ENROLL_PROCESS_THREE,                                   /** 第三次按压手指，即录入阶段为60% **/
//    ENROLL_PROCESS_FOUR,                                    /** 第四次按压手指，即录入阶段为80% **/
//    ENROLL_PROCESS_FIVE,                                    /** 第五次按压手指，即录入阶段为100% **/
//    ENROLL_PROCESS_MAX,
//}EnrollProcess;

/* 定义状态类型，用于通知服务层设备状态的改变 */
typedef enum {
	STATUS_TYPE_DEVICE = 0,
	STATUS_TYPE_OPERATION,
    STATUS_TYPE_NOTIFY
}StatusType;

/*
 * 生物特征识别框架核心结构体，用于记录驱动与设备相关信息
 * 正常情况下，一台机器上同一型号设备只会接入一台，所以框架将驱动与设备简化成一个结构体
 */
typedef struct bio_dev_t bio_dev;
struct bio_dev_t {
	// 设备、驱动基本信息
	int driver_id;			/** 驱动ID **/
	char *device_name;		/** 设备名 **/
	char *full_name;		/** 设备全称 **/
	api_version drv_api_version;
	int enable;				/** 设备是否被禁用 **/
	int dev_num;			/** 设备数量 **/
	struct BioInfo bioinfo;	/** 设备的相关类型 **/

	// 总线信息
	struct Serial_Info	serial_info;
	struct USB_INFO		usb_info;
	struct PCI_INFO		pci_info;

	// 设备、驱动私有信息
	void *dev_priv;			/** 设备驱动私有结构体 **/
	void *bio_priv;			/** 各生物信息私有结构体 **/

	// 边界值
	int max_user;
	int max_sample;
	int sample_times;

	/*
	 * 记录设备的状态信息：
	 *
	 * DevStatus dev_status 用于记录设备当前状态：“使用中”或者“空闲”
	 * OpsResult ops_result 用于记录对设备操作的结果
	 *                      注意：各个设备操作函数的返回值只是操作结果的辅助信息，最终的
	 *                           结果判断是依据ops_result的值来确定的。
	 * NotifyMID notify_mid 用于人机交互，记录需要展示给用户看的消息的ID
	 */
	volatile DevStatus dev_status;	/** 设备状态 **/
	volatile OpsResult ops_result;	/** 操作结果 **/
	volatile NotifyMID notify_mid;	/** 给用户的消息ID **/

	// 指向驱动的句柄，由框架使用，驱动请勿修改该值
	void * plugin_handle;

	/* 以下函数指针定义了对驱动或设备的操作集合 */

	/* 驱动初始化及资源回收函数 */

	/*
	 * 驱动配置
	 *   为驱动设置相关属性，并设置各种操作的回调函数
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	conf	框架配置文件
	 * @return		配置结果，成功返回0，失败返回负值
	 */
	int (*ops_configure)(bio_dev *dev, GKeyFile * conf);

	/*
	 * 驱动初始化
	 *   对驱动进行初始化，所有驱动初始化相关操作都放入本函数中处理。例如公共存储空间的分配、
	 * 系统环境探测、驱动全局变量的初始化等驱动相关的操作放入本函数中实现。
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		初始化结果，成功返回0，失败返回负值
	 */
	int (*ops_driver_init)(bio_dev *dev);

	/*
	 * 驱动资源回收
	 *   对驱动配置(ops_configure)、初始化(ops_driver_init)过程中申请的资源进行释放
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		无
	 * @attention	资源回收函数无返回值，如若碰到资源释放失败的异常或错误，需要将相关错误
	 *				信息使用"bio_print_error"函数输出
	 */
	void (*ops_free)(bio_dev *dev);

	/*
	 * 设备发现
	 *   检测系统环境中接入驱动支持的设备数量
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		探测到的设备数量。如果过程中遇到错误，则返回负值
	 */
	int (*ops_discover)(bio_dev *dev);

	/* 设备初始化及资源回收函数 */

	/*
	 * 设备打开函数，用于初始化设备
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		操作结果，成功返回0，失败返回负值
	 * @attention	在前端每次调用设备进行捕获、录入、认证、识别等操作后都会执行一次
	 */
	int (*ops_open)(bio_dev *dev);

	/*
	 * 设备关闭函数，用于关闭硬件及释放ops_open函数申请的资源
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		操作结果，成功返回0，失败返回负值
	 * @attention	在前端每次调用设备进行捕获、录入、认证、识别等操作后都会执行一次
	 */
	void (*ops_close)(bio_dev *dev);

	/* 设备功能性操作 */

	/*
	 * 特征捕获函数，用来捕获生物特征信息
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	action	属于老旧接口中的参数，现已弃用。
	 *						为了保持API的兼容性，暂时保留该形参。
	 * @return		操作结果，成功返回0，失败返回负值
	 */
	char * (*ops_capture)(bio_dev *dev, OpsActions action);

	/*
	 * 特征录入函数，用来录入用户的生物特征信息
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	action	属于老旧接口中的参数，现已弃用。
	 *						为了保持API的兼容性，暂时保留该形参。
	 * @param[in]	uid		特征的用户ID
	 * @param[in]	idx		特征的索引值（每个用户的索引值单独从0开始）
	 * @param[in]	bio_idx_name	特征的名称
	 * @return		操作结果，成功返回0，失败返回负值
	 */
	int (*ops_enroll)(bio_dev *dev, OpsActions action, int uid, int idx,
					  char * bio_idx_name);

	/*
	 * 特征验证函数，使用当前生物特征与指定特征对比，判断是否是同一个特征
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	action	属于老旧接口中的参数，现已弃用。
	 *						为了保持API的兼容性，暂时保留该形参。
	 * @param[in]	uid		特征的用户ID
	 * @param[in]	idx		特征的索引值（每个用户的索引值单独从0开始）
	 * @return		返回操作结果，>=0为操作未出错，<0为操作过程中出错
	 * @attention	是否验证匹配，框架通过获取设备的ops_result值来判断。ops_result值
	 *				为OPS_VERIFY_MATCH则匹配，值为OPS_VERIFY_NO_MATCH代表不匹配，其
	 *				他值代表验证过程中出现的错误。
	 */
	int (*ops_verify)(bio_dev *dev, OpsActions action, int uid, int idx);

	/*
	 * 特征识别函数，使用当前生物特征与指定范围的特征比对，识别出当前特征与指定范围内的哪个特征匹配
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	action	属于老旧接口中的参数，现已弃用。
	 *						为了保持API的兼容性，暂时保留该形参。
	 * @param[in]	uid		特征的用户ID
	 * @param[in]	idx_start	特征索引最小值
	 * @param[in]	idx_end		特征索引最大值
	 * @return		返回操作结果，>=0匹配特征的用户ID，<0为识别操作失败或未找到匹配的特征
	 * @attention	是否识别到特征，框架通过获取设备的ops_result值来判断。ops_result值
	 *				为OPS_IDENTIFY_MATCH则识别到，值为OPS_IDENTIFY_NO_MATCH代表未识
	 *				别到，其他值代表识别过程中出现的错误。
	 */
	int (*ops_identify)(bio_dev *dev, OpsActions action, int uid,
						int idx_start, int idx_end);

	/*
	 * 特征搜索函数，使用当前生物特征与指定范围的特征比对，搜索出指定范围内所有匹配的特征
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	action	属于老旧接口中的参数，现已弃用。
	 *						为了保持API的兼容性，暂时保留该形参。
	 * @param[in]	uid		特征的用户ID
	 * @param[in]	idx_start	特征索引最小值
	 * @param[in]	idx_end		特征索引最大值
	 * @return		特征信息列表，包含指定范围内所有匹配的特征信息
	 * @attention	是否搜索到特征，框架通过获取设备的ops_result值来判断。ops_result值
	 *				为OPS_SEARCH_MATCH则搜索到，值为OPS_SEARCH_NO_MATCH代表未搜索到，
	 *				其他值代表搜索过程中出现的错误。
	 */
	feature_info * (*ops_search)(bio_dev *dev, OpsActions action, int uid,
					  int idx_start, int idx_end);

	/*
	 * 特征清理（删除）函数，删除指定范围内的所有特征
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	action	属于老旧接口中的参数，现已弃用。
	 *						为了保持API的兼容性，暂时保留该形参。
	 * @param[in]	uid		特征的用户ID
	 * @param[in]	idx_start	特征索引最小值
	 * @param[in]	idx_end		特征索引最大值
	 * @return		操作结果，成功返回0，失败返回负值
	 */
	int (*ops_clean)(bio_dev *dev, OpsActions action, int uid,
					 int idx_start, int idx_end);

	/*
	 * 获取指定设备的特征列表
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	action	属于老旧接口中的参数，现已弃用。
	 *						为了保持API的兼容性，暂时保留该形参。
	 * @param[in]	uid		特征的用户ID
	 * @param[in]	idx_start	特征索引最小值
	 * @param[in]	idx_end		特征索引最大值
	 * @return		特征信息列表，包含指定范围内所有匹配的特征信息
	 */
	feature_info * (*ops_get_feature_list)(bio_dev *dev, OpsActions action,
										   int uid, int idx_start, int idx_end);

	/*
	 * 重命名指定特征
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	uid		特征的用户ID
	 * @param[in]	idx		特征的索引
	 * @param[in]	new_name	特征的新名称
	 * @return		操作结果，成功返回0，失败返回负值
	 */
	int (*ops_feature_rename)(bio_dev *dev, int uid, int idx, char * new_name);

	/*
	 * 中断设备的当前操作
	 *  由于生物特征识别是一个人机交互过程，所以在此期间内用户有可能随时取消操作。
	 *  在用户取消操作的时，本接口会被调用，驱动需要在本接口中实现终止设备当前操作的功能。
	 * @param[in]	dev		驱动和设备的结构体
	 * @param[in]	waiting_ms	终止操作的等待时间，单位毫秒。
	 * @return		返回终止操作是否成功，0终止成功，<0终止失败
	 * @attention	终止操作不能无限期等待，在超过waiting_ms毫秒后，需返回“终止失败”。
	 */
	int (*ops_stop_by_user)(bio_dev *dev, int waiting_ms);

	/*
	 * 状态变更的回调函数
	 *   该函数在每一次通过框架提供的API修改dev_status、ops_result、notify_mid三个变量
	 * 的时候调用，驱动可以不用实现本函数。
	 *
	 * @param[in]	drvid	驱动的ID
	 * @param[in]	type	变更状态的类型，类型定义参见"StatusType"
	 * @return		无
	 */
	void (*ops_status_changed_callback)(int drvid, int type);

	/*
	 * 获取设备状态的文本消息
	 *   将dev_status状态码转换成状态提示字符串。
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		设备状态的文本消息
	 * @attention	该函数只需匹配自定义的状态码，由框架定义的状态码框架会自行转换。
	 *				因此遇到非自定义的状态码，直接返回NULL即可。
	 */
	const char * (*ops_get_dev_status_mesg)(bio_dev *dev);

	/*
	 * 获取操作结果的文本消息
	 *   将ops_result操作结果码转换成操作结果提示字符串。
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		操作结果的文本消息
	 * @attention	该函数只需匹配自定义的结果码，由框架定义的结果码框架会自行转换。
	 *				因此遇到非自定义的结果码，直接返回NULL即可。
	 */
	const char * (*ops_get_ops_result_mesg)(bio_dev *dev);

	/*
	 * 获取面向用户的提示消息
	 *   将notify_mid消息ID转换提示字符串。
	 *
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		面向用户的提示信息
	 * @attention	该函数只需匹配自定义的消息ID，由框架定义的消息ID框架会自行转换。
	 *				因此遇到非自定义的消息ID，直接返回NULL即可。
	 */
	const char * (*ops_get_notify_mid_mesg)(bio_dev *dev);

	/*
	 * 框架支持的设备插入时的回调函数
	 *   对于USB设备，设备插入且框架检测到是驱动支持的设备型号时调用。如果设备插拔不需要额外
	 * 的操作，该函数接口可以置为NULL。
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		无
	 */
	void (*ops_attach)(bio_dev *dev);

	/*
	 * 框架支持的设备拔出时的回调函数
	 *   对于USB设备，设备拔出且框架检测到是驱动支持的设备型号时调用。如果设备插拔不需要额外
	 * 的操作，该函数接口可以置为NULL。
	 * @param[in]	dev		驱动和设备的结构体
	 * @return		无
	 */
	void (*ops_detach)(bio_dev *dev);
};

/* 定义状态改变时回调函数的函数指针 */
typedef void (*BIO_STATUS_CHANGED_CALLBACK)(int drvid, int type);

/* 定义USB热插拔时回调函数的函数指针 */
typedef void (*BIO_USB_DEVICE_HOT_PLUG_CALLBACK)(int drvid, int action, int dev_num_now);

/*
 * 设置框架通用的操作超时时间，单位：毫秒
 *
 * @param[in]	t	超时时间
 */
void bio_set_ops_timeout_ms(int t);

/*
 * 获取框架通用的操作超时时间，单位：毫秒
 *
 * @return	int	超时时间
 */
int bio_get_ops_timeout_ms();

/*
 * 释放驱动链表
 *   释放由bio_driver_list_init()函数创建的设备结构体，包含链表的数据域资源
 * @param[in]	GList*	drv_list	设备结构体链表
 */
void bio_free_driver(GList * drv_list);

/*
 * 释放设备链表
 *   释放由bio_device_list_init()函数创建的设备结构体，不包含链表的数据域资源
 * @param[in]	GList*	dev_list	设备结构体链表
 */
void bio_free_device(GList * dev_list);

/* 捕获特征 */
char * bio_ops_capture(bio_dev *dev);

/* 录入特征 */
int bio_ops_enroll(bio_dev *dev, int uid, int idx, char * idx_name);

/* 验证特征 */
int bio_ops_verify(bio_dev *dev, int uid, int idx);

/* 识别特征 */
int bio_ops_identify(bio_dev *dev, int uid, int idx_start, int idx_end);

/* 搜索特征 */
feature_info * bio_ops_search(bio_dev *dev, int uid, int idx_start, int idx_end);

/* 清理（删除）特征 */
int bio_ops_clean(bio_dev *dev, int uid, int idx_start, int idx_end);

/* 获取特征列表 */
feature_info * bio_ops_get_feature_list(bio_dev *dev, int uid,
										int idx_start, int idx_end);

/* 重命名特征 */
int bio_ops_feature_rename(bio_dev *dev, int uid, int idx, char * new_name);

/* 终止当操作 */
int bio_ops_stop_ops(bio_dev *dev, int waiting_sec);

/*
 * 驱动是否被启用
 *
 * @param[in]	dev		设备结构体
 * @param[in]	conf	配置文件
 * @return		驱动是否被启用
 * @retval		TRUE	启用
 *				FALSE	禁用
 */
int bio_dev_is_enable(bio_dev * dev, GKeyFile * conf);

/*
 * 设置串口的设备节点路径
 *   形如：/dev/ttyXXX
 *
 * @param[in]	dev		设备结构体
 * @param[in]	conf	配置文件
 * @return		设置是否成功，成功返回0，出错或不成返回负数
 */
int bio_dev_set_serial_path(bio_dev * dev, GKeyFile * conf);

/* 判断操作是否被用户终止 */
bool bio_is_stop_by_user(bio_dev * dev);

/*
 * Base64编码
 *   将二进制数据以Base64编码方式编码
 *
 * @param[in]	bin_data	二进制数据
 * @param[in]	base64		base64编码后，数据存放位置
 * @param[in]	bin_len		二进制数据长度
 * @return		Base64编码后，数据长度
 */
int bio_base64_encode(unsigned char * bin_data, char * base64, int bin_len );

/*
 * Base64解码
 *   将以Base64编码方式编码的数据解码为二进制数据
 * @param[in]	base64		base64编码数据缓冲区
 * @param[in]	bin_data	base64解码后，二进制数据存放位置
 * @return		Base64解码后，二进制数据长度
 */
int bio_base64_decode(char * base64, unsigned char * bin_data);

/*
 * 获取、设置设备状态
 *
 * bio_get_dev_status	获取设备状态
 * bio_set_dev_status	设置设备状态（相对值），该函数会根据当前设备状态(dev_status)来修
 *						正输入值。
 *						例如，设备正在录入(enroll)，设置的状态为"DEVS_COMM_STOP_BY_USER"，
 *						最终的状态为"DEVS_ENROLL_STOP_BY_USER"
 * bio_set_dev_abs_status 设置设备状态（绝对值），该函数会不会修正输入值。
 *						例如，设备正在录入(enroll)，设置的状态为"DEVS_COMM_STOP_BY_USER"，
 *						最终的状态为"DEVS_COMM_STOP_BY_USER"
 */
int bio_get_dev_status(bio_dev * dev);
void bio_set_dev_status(bio_dev * dev, int status);
void bio_set_dev_abs_status(bio_dev * dev, int status);

/*
 * 获取、设置操作结果
 *
 * bio_get_ops_result	获取操作结果
 * bio_set_ops_result	设置操作结果（相对值），该函数会根据当前设备状态(dev_status)来修
 *						正输入值。
 *						例如，设备正在录入(enroll)，设置的操作结果为"OPS_COMM_ERROR"，
 *						最终的操作结果为"OPS_ENROLL_ERROR"
 * bio_set_ops_abs_result 设置操作结果（绝对值），该函数会不会修正输入值。
 *						例如，设备正在录入(enroll)，设置的操作结果为"OPS_COMM_ERROR"，
 *						最终的操作结果为"OPS_COMM_ERROR"
 */
int bio_get_ops_result(bio_dev * dev);
void bio_set_ops_result(bio_dev * dev, int result);
void bio_set_ops_abs_result(bio_dev * dev, int result);

/*
 * 获取、设置提示消息ID
 *
 * bio_get_notify_mid	获取提示消息ID
 * bio_set_notify_mid	设置提示消息ID（相对值），该函数会根据当前设备状态(dev_status)来修
 *						正输入值。
 *						例如，设备正在录入(enroll)，设置的提示消息ID为"NOTIFY_COMM_TIMEOUT"，
 *						最终的提示消息ID为"NOTIFY_ENROLL_TIMEOUT"
 * bio_set_notify_abs_mid 设置提示消息ID（绝对值），该函数会不会修正输入值。
 *						例如，设备正在录入(enroll)，设置的提示消息ID为"NOTIFY_COMM_TIMEOUT"，
 *						最终的提示消息ID为"NOTIFY_COMM_TIMEOUT"
 */
int bio_get_notify_mid(bio_dev * dev);
void bio_set_notify_mid(bio_dev * dev, int status);
void bio_set_notify_abs_mid(bio_dev * dev, int status);

/* 一次性设置设备状态、操作结果、提示消息ID（相对值） */
void bio_set_all_status(bio_dev * dev,
						int dev_status, int ops_result, int notify_mid);

/* 一次性设置设备状态、操作结果、提示消息ID（绝对值） */
void bio_set_all_abs_status(bio_dev * dev,
							int dev_status, int ops_result, int notify_mid);

/* 获取设备状态的文本消息 */
const char * bio_get_dev_status_mesg(bio_dev *dev);

/* 获取操作结果的文本消息 */
const char * bio_get_ops_result_mesg(bio_dev *dev);

/* 获取面向用户的提示消息 */
const char * bio_get_notify_mid_mesg(bio_dev *dev);

/* 获取空闲的Driver ID */
int bio_get_empty_driver_id();

/*
 * 获取当前的驱动链表
 * @return	GList *
 */
GList * bio_get_drv_list(void);

/*
 * 获取当前的设备链表
 * @return	GList *
 */
GList * bio_get_dev_list(void);

/* 打印驱动列表 */
void bio_print_drv_list(int print_level);

/* 打印设备列表 */
void bio_print_dev_list(int print_level);

/* 释放驱动链表及其指向的资源 */
void bio_free_drv_list(void);

/* 释放设备链表及其指向的资源 */
void bio_free_dev_list(void);

char * bio_new_string(char * old_str);

/*
 * 查询空闲索引号
 *   在数据库中查询指定用户指定范围的空闲索引号
 * @param[in]	dev		设备结构体
 * @param[in]	uid		用户ID
 * @param[in]	start	开始的索引值
 * @param[in]	end		结束的索引值，-1代表不限大小
 * @return		空闲的索引号，-1代表指定范围索引全被占用
 */
int bio_common_get_empty_index(bio_dev *dev, int uid, int start, int end);

/* 数据库中查询空闲采样编号 */
int bio_common_get_empty_sample_no(bio_dev *dev, int start, int end);

/* 通用的USB设备探测方式 */
int bio_common_detect_usb_device(bio_dev *dev);

/* 通用的信息输出函数 */
int bio_vprint_by_level(int print_level, FILE * __restrict stream,
						const char * __restrict format, va_list args);
int bio_print_by_level(int print_level, FILE * stream, char * format, ...);
int bio_print_debug(const char *format, ...);
int bio_print_info(const char * format, ...);
int bio_print_notice(const char * format, ...);
int bio_print_warning(const char *format, ...);
int bio_print_error(const char * format, ...);

typedef void (*BIO_STATUS_CHANGED_CALLBACK)(int drvid, int type);
typedef void (*BIO_USB_DEVICE_HOT_PLUG_CALLBACK)(int drvid, int action, int dev_num_now);

/*
 * 打开及初始化生物识别框架
 *
 * @return	int	返回初始化结果
 * @retval	0	成功
 *			其他	失败
 */
int bio_init(void);

/*
 * 关闭生物识别框架，释放资源
 *
 * @return	int	返回初始化结果
 * @retval	0	成功
 *			其他	失败
 */
int bio_close(void);

/*
 * 初始化驱动链表
 * @return	返回初始化后的驱动链表
 */
GList * bio_driver_list_init(void);

/*
 * 初始化设备列表
 * 搜索系统中支持生物识别的设备，构建设备链表
 *
 * @return	GList*	GList链表，data指针指向bio_dev结构体
 * @note	设备链表的data指针与驱动链表的data指针指向同一个bio_dev结构体，设备链表只
 *			记录当前有驱动并且已连接的设备
 */
GList * bio_device_list_init(GList *);

/* 声明全局变量 */
extern GList * bio_drv_list;
extern GList * bio_dev_list;
extern BIO_STATUS_CHANGED_CALLBACK bio_status_changed_callback;
extern BIO_USB_DEVICE_HOT_PLUG_CALLBACK bio_usb_device_hot_plug_callback;

#ifdef  __cplusplus
}
#endif

#endif // BIOMETRIC_COMM_H

