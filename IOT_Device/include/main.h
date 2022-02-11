#include "aos/init.h"
#include "board.h"
#include <aos/errno.h>
#include <aos/kernel.h>
#include <k_api.h>
#include <stdio.h>
#include <stdlib.h>
#include "netmgr.h"
#include "app_utils.h"
#include <uservice/uservice.h>
#include <uservice/eventid.h>
#include "ulog/ulog.h"
#include "oled_display.h"

//自定义wifi_info结构体用于函数间传输
typedef struct wifi_pair{
    netmgr_hdl_t hdl;
    netmgr_connect_params_t envs;
}wifi_info;

static int fd;  //TCP连接的文件描述符

extern int server_connect(char* ip, int port);
extern void data_collect(int fd);                                       

//aos_task_create时转调data_collect
static void data_entry(void *data);

/**
 * @brief 尝试连接服务端并传输数据
 * 
 * @param event_id event调用id，应为EVENT_NETMGR_DHCP_SUCCESS
 * @param param NULL
 * @param context NULL
 * @return None
 */
static void wifi_success_hdl(uint32_t event_id, const void *param, void *context);

/**
 * @brief 检查wifi状态并重连
 * 
 * @param event_id event服务自动提供，人工调用需为 EVENT_TCP_FAILED
 * @param param event服务自动提供，可为NULL
 * @param context event服务调用中param，需提供wifi_into*
 * @return None
 */
static void _wifi_connect(uint32_t event_id, const void *param, void *context);