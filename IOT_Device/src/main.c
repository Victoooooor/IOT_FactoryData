/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */
#define WIFI_CONNECT_VAR

#include "main.h"

int application_start(int argc, char *argv[])
{
    oled_gpio_init();
    
    wifi_info info = {
        .hdl = loadenv,
        .envs = envs,
    };

    aos_set_log_level(AOS_LL_WARN);
    
    //初始化并配置事件服务
    event_service_init(NULL);
    netmgr_service_init(NULL);//初始化联网服务
    
    event_subscribe(EVENT_NETMGR_DHCP_SUCCESS, wifi_success_hdl, NULL); //EVENT_NETMGR_DHCP_SUCCESS为netmgr_connect自动发出
    event_subscribe(EVENT_TCP_FAILED, _wifi_connect,&info);

    LOGI("APP_START","Start WIFI_CON SSID: %s", envs.params.wifi_params.ssid);

    //开始屏幕刷新
    aos_timer_t disp_refresh;
    if(aos_timer_create(&disp_refresh,OLED_Refresh_GRAM,NULL,250,AOS_TIMER_REPEAT|AOS_TIMER_AUTORUN)){
        fprintf(stderr, "disp_failed\n");
        aos_msleep(1000);
        aos_reboot();
    }

    _wifi_connect(EVENT_TCP_FAILED,NULL,&info); //人工调用连接wifi并进入事件循环

    while (1) {
        aos_msleep(10000);
    };
}

void data_entry(void* data){
    data_collect(fd);
    return;
}

static void wifi_success_hdl(uint32_t event_id, const void *param, void *context)
{
    _disp(1,"WIFI_CONNECTED",1);
    if(event_id!=EVENT_NETMGR_DHCP_SUCCESS){
        LOGI("APP","WIFI CONN ERR");
        return;
    }
    fprintf(stderr,"TCP ENTRY POINT\n");
    fd = server_connect(SERVER_IP,SERVER_PORT); //连接服务端
    if(fd<0)
        return;
    aos_task_t d_t;
    fprintf(stderr,"checkpoint1: %d\n", fd);
    int ret = 
        aos_task_create(&d_t,"data transfer",data_entry,NULL,NULL,6048,AOS_DEFAULT_APP_PRI, AOS_TASK_AUTORUN);  //开始传输任务
    if(ret<0){
        fprintf(stderr,"Error: create task data_entry failed!\n");
        fprintf(stderr,"REBOOTING!\n");
        aos_reboot();
    }
    return;
}

static void _wifi_connect(uint32_t event_id, const void *param, void *context){
    _disp(1,"_wifi_connect",1);
    aos_msleep(500);    //防止因服务端连接失败而快速循环
    if(event_id != EVENT_TCP_FAILED){
        fprintf(stderr,"_wifi_connect call error! EVENT_IT: %d\r\n", event_id);
        return;
    }

    wifi_info* info = (wifi_info*) context;
    netmgr_conn_state_t wifi_state = netmgr_get_state(info->hdl);
    //检查网络连接状态
    if (wifi_state == CONN_STATE_CONNECTED || wifi_state == CONN_STATE_NETWORK_CONNECTED){
        event_publish(EVENT_NETMGR_DHCP_SUCCESS,NULL);
        return;
    }
    _disp(1,"CONNECTING WIFI",1);
    //连接网络
    int state=netmgr_connect(info->hdl,&(info->envs));
    while(state!=0){
        aos_msleep(500);
        LOGI("APP","retry connecting WIFI\n");
        state=netmgr_connect(info->hdl,&(info->envs));
        
    }

    LOGI("APP","WIFI connected!\n");
    return;

}

