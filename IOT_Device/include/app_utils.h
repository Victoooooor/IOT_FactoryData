#ifndef APP_UTILS_H
#define APP_UTILS_H
    #include "netmgr.h"

    #define EVENT_TCP_CONNECTED 534
    #define EVENT_TCP_FAILED 535


    #ifndef PACKET_HEAD
        #define PACKET_HEAD 0xFFFF
    #endif

    #ifndef DEVICE_CODE //设备号码，用于区分设备，需保证设备不共享同一号码
        #define DEVICE_CODE 0x000F
    #endif

    //服务端地址，应更改为固定域名
    #define SERVER_IP "192.168.0.172"
    #define SERVER_PORT 27014

    typedef unsigned char byte;
    typedef struct packet{
        unsigned short state;
        short ADC[3];
    } packet;

    /*在WIFI_CONNECT_VAR定义时加入wifi连接信息*/
    #ifdef WIFI_CONNECT_VAR
        const netmgr_hdl_t loadenv=49;
        const netmgr_connect_params_t envs=
            {
                .params.wifi_params.ssid="hekto@",
                .params.wifi_params.pwd="si2020mu0831",
            };
    #endif
#endif