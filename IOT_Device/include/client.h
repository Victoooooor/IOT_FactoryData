#include "aos/init.h"
#include "aos/hal/adc.h"
#include "aos/hal/gpio.h"
#include "hal_iomux_haas1000.h"
#include <lwip/sockets.h>
#include <lwip/tcp.h>
#include <uservice/eventid.h>
#include <stdbool.h>
#include "aos/hal/uart.h"
#include <cJSON.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include "aos/vfs.h"

#include <drivers/u_ld.h>
#include <drivers/ddkc_log.h>
#include <vfsdev/uart_dev.h>
#include <drivers/char/u_device.h>

#include "oled_display.h"
#include "app_utils.h"

#define TEST_ADC_PORT_ADC0 0   //ADC0 对应排针30脚
#define TEST_ADC_PORT_ADC1 1   //ADC1 对应排针15脚
#define TEST_ADC_PORT_ADC2 2   //ADC2 对应排针25脚

#define NUM_PIN 3

//开关量PIN
#define SWITCH	HAL_GPIO_PIN_P0_0
#define TOGGLE  HAL_GPIO_PIN_P0_1

//传输结构体overhead大小（byte）
#define OVERHEAD 6

//定义转换常量
#define V_low	-2000
#define V_high	2000
#define M_low	165
#define M_high	1735

#define V_low2	0
#define V_high2	8000
#define M_low2	0
#define M_high2	1600

double slope = .0;
double offset = .0;
double slope2 = .0;
double offset2 = .0;

/**
 * @brief 处理切换设备信号
 * 
 * @param arg NULL
 */
void toggle_handler(void* arg);

/**
 * @brief 处理开关量信号
 * 
 * @param arg NULL
 */
void toggle_handler2(void* arg);

static bool s1=false;   //开关量
static int state=0;     //设备切换
static k_ringbuf_t* send_ringbuf = NULL;    //环形缓存
static byte buf[512] = {0}; //缓存地址
static bool send_state = true;  //用于从发送线程结束数据采集线程

/**
 * @brief 调用lwip连接到位于ip:port服务端
 * 
 * @param ip 服务器ip
 * @param port 服务端端口
 * @return int <0 连接失败，>=0 连接成功并返回tcp socket文件描述符
 */
int server_connect(char* ip, int port);

/**
 * @brief 发送数据单流程
 * 
 * @param argc NULL
 * @param argv 发送对象文件描述符
 */
static void data_send(void* argc, void*  argv);

/**
 * @brief 接收指令/字符段流程
 * 
 * @param argc NULL
 * @param argv 接收socket对象文件描述符
 */
static void data_recv(void* argc, void*  argv);

/**
 * @brief 实时刷新第二行数据
 * 
 * @param argc NULL
 * @param argv 接收数据显示指针 int*
 */
static void data_disp(void* argc, void*  argv);

/**
 * @brief 从指定端口收集数据并发起1Hz频率数据传输，应在server_connect成功后调用
 * 
 * @param fd tcp连接使用的文件描述符
 */
void data_collect(const int fd);

/**
 * @brief 生成1补数校验
 * 
 * @param check 校验数据起始指针
 * @param len 校验数据长度
 * @return unsigned short 
 */
unsigned short get_sum(byte* check, const int len);

/**
 * @brief 1补数检验数据完整性
 * 
 * @param sent 接收数据起始位
 * @param recvd 接受数据长度
 * @return 
 * -1：未找到0xFFFF开始符
 * 0：校验成功
 * 1：数据错误
 */
int check_sum(byte* sent, unsigned short recvd);

/**
 * @brief 转换测量值到实际值
 * 
 * @param measured 测量电压
 * @return int 实际电压
 */
int Electro_Meter(int measured);