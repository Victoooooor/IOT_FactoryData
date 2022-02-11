#include "client.h"

//初始化数据接收空间
static char tempbuf[512] = {0};
static cJSON *parsed_json,*json_dsp,*json_cmd;

//显示二级缓存
static char data_str[20] = {0};

int server_connect(char* ip, int port)
{
    int re;
    int  fd = 0;
    struct sockaddr_in addr;
    struct timeval timeout;
    memset(&addr, 0,sizeof(addr));
    //配置连接设置
    addr.sin_port = htons((short)port);
    if(addr.sin_port==0){
        fprintf(stderr,"PORT FORMAT ERROR\n");
        goto err;
    }

    addr.sin_addr.s_addr = inet_addr(ip);
    if(addr.sin_addr.s_addr==IPADDR_NONE){
        fprintf(stderr,"IP FORMAT ERROR\n");
        goto err;
    }
    addr.sin_family = AF_INET;
    //生成socket
    fd = lwip_socket(AF_INET,SOCK_STREAM,0);
    if (fd < 0){
        fprintf(stderr,"fail to creat socket errno = %d \r\n", errno);
        goto err;
    }

    fprintf(stderr, "Target Server fd=%d, ip=%s, port=%d\n", fd, ip, addr.sin_port);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    //设置socket
    re = lwip_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,(char *)&timeout, sizeof(timeout));
    if (re <  0){
        fprintf(stderr,"Set Socket options failed, Err: %d \r\n", errno);
        goto err;
    }
    //尝试连接
    re = lwip_connect(fd, (const struct sockaddr*) &addr, sizeof(addr));
    if (re !=  0){
        fprintf(stderr,"TCP Connection failed, Err: %d \r\n", errno);
        goto err;
    }
    event_publish(EVENT_TCP_CONNECTED,NULL);//发起连接成功事件
    return fd;
err:
    close(fd);
    aos_msleep(500);
    event_publish(EVENT_NETMGR_DHCP_SUCCESS,NULL);//发起连接失败事件
    return -1;
}

static void data_send(void* argc, void*  argv){
    //检查环形缓存
    if(ringbuf_is_empty(send_ringbuf))  return;

    //读取socket描述符
    int* fd = (int*) argv;
    printf("FROM %d\r\n", *fd);
    //从环形缓存中读取数据转存入发送缓存
    unsigned short num_packets = (512-send_ringbuf->freesize) / sizeof(packet);
    unsigned short *pack_len = buf+4;
    *pack_len = num_packets * sizeof(packet);
    unsigned short data_len;
    for (int i =0; i <  num_packets; i++){
        ringbuf_pop(send_ringbuf,buf+OVERHEAD+(i*sizeof(packet)),&data_len);
        if(data_len!=sizeof(packet))
            fprintf(stderr,"size unmatched......\n");
    }
    //生成校验
    unsigned short total_Len = *pack_len+OVERHEAD+get_sum(buf+2,*pack_len+OVERHEAD-2);
    //传输数据
    unsigned short ret = lwip_write(*fd, buf, total_Len);
    fprintf(stderr,"in data_send: %d\t%d\n", ret, data_len);
    fprintf(stderr,"checksum: %d\n",check_sum(buf,ret));
    if(ret != total_Len){
        // send failed
        send_state = false;
    }
}

static void data_recv(void* argc, void*  argv){
    int* fd = (int*) argv;
    int ret = lwip_recv(*fd, tempbuf,512,MSG_DONTWAIT); //不阻塞
    printf("net read length:%d FROM %d\r\n", ret, *fd);
    if(ret > 0){
        tempbuf[ret] = '\0';
        fprintf(stderr,"msg: %s\r\n",tempbuf);
        //json解包
        parsed_json = cJSON_Parse(tempbuf);
        if(parsed_json!=NULL){
            //执行显示内容
            json_dsp = cJSON_GetObjectItem(parsed_json,"dsp");
            if(json_dsp != NULL){
                if(json_dsp->type & (cJSON_String|cJSON_Raw))
                    _disp(2,json_dsp->valuestring,1);
                else if(json_dsp->type & cJSON_Number){
                    char temp_number[50] = {0};
                    snprintf(temp_number,50,"%.2E",json_dsp->valuedouble);
                    _disp(2,temp_number,1);
                }
                    
            }
            //支持执行服务端发送指令
            json_cmd = cJSON_GetObjectItem(parsed_json,"cmd");
            if(json_cmd != NULL){
                ;
            }
        }
        else{
            fprintf(stderr,"parsed NULL\r\n");
        }  
    }
}

static void data_disp(void* argc, void*  argv){
    int *data = (int*) argv;
    snprintf(data_str, 20, "%d\0",*data);
    _disp(1,data_str,1);
}

void data_collect(const int fd){
    //重置变量
    packet data;
    bool edge=0;
    
    send_state = true;
    s1=false;
    state=-1;//人工调用toggle_handler +=1，用于同步OLED
    toggle_handler(NULL);
    
    int uart_fd = -1;
    int port_id = 2;
    char dev_name[16] = {0};
    char rfid_data_buf[50] = {0};
    snprintf(dev_name, sizeof(dev_name), "/dev/ttyUART%d", port_id);
    fprintf(stderr, "opening device:%s\r\n", dev_name);

    uart_fd = open(dev_name, 0);
    if (uart_fd < 0) {
        fprintf(stderr, "open uart error\r\n");
        _disp(0,"OPEN_UART F",1);
        aos_msleep(10000);
        aos_reboot();
    }

    //初始化环形缓存
    byte buf_ring[512]={0};
    k_ringbuf_t ring;
    ringbuf_init(&ring,buf_ring,512,RINGBUF_TYPE_FIX,sizeof(packet));
    if(!ringbuf_is_empty(&ring))
        ringbuf_reset(&ring);
    send_ringbuf = &ring;


    //初始化1Hz数据发送
    aos_timer_t send_timer;
    if(aos_timer_create(&send_timer,data_send,&fd,1000,AOS_TIMER_REPEAT)){
        fprintf(stderr, "send_failed\n");
        _disp(0, "send_failed",1);
        aos_msleep(1000);
        aos_reboot();
    }

    //初始化0.5Hz数据接收
    aos_timer_t recv_timer;
    if(aos_timer_create(&recv_timer,data_recv,&fd,2000,AOS_TIMER_REPEAT)){
        fprintf(stderr, "recv_failed\n");
        _disp(0, "recv_failed",1);
        aos_msleep(1000);
        aos_reboot();
    }
    
    //初始化屏幕数据显示
    int realtime = 0;
    aos_timer_t disp_timer;
    if(aos_timer_create(&disp_timer,data_disp,&realtime,250,AOS_TIMER_REPEAT|AOS_TIMER_AUTORUN)){
        fprintf(stderr, "disp_failed\n");
        _disp(0, "disp_failed",1);
        aos_msleep(1000);
        aos_reboot();
    }

    //偏移数据指针
    unsigned short *header= buf, 
        *Device_ID = buf+2,
        *pack_len = buf+4,
        *checksum = buf+(OVERHEAD+sizeof(packet));
    *header = PACKET_HEAD;
    *Device_ID = DEVICE_CODE;
    *pack_len = sizeof(packet);    

    //初始化开关量PIN
    gpio_dev_t tg = {
        .config = INPUT_PULL_DOWN,
        .port = TOGGLE,
    };

    gpio_dev_t tg2 = {
        .config = INPUT_PULL_DOWN,
        .port = SWITCH,
    };
    
    hal_gpio_init(&tg);
    hal_gpio_init(&tg2);

    int ret = sizeof(buf);
    int ret_ADC[NUM_PIN] = {0};

    //初始电压数据采集
    adc_dev_t v1;
    adc_dev_t v2;
    adc_dev_t v3;

    v1.port = TEST_ADC_PORT_ADC0;
    v1.config.sampling_cycle = 1000;
    v2.port = TEST_ADC_PORT_ADC1;
    v2.config.sampling_cycle = 1000;
    v3.port = TEST_ADC_PORT_ADC2;
    v3.config.sampling_cycle = 1000;
    
    ret_ADC[0] = hal_adc_init(&v1);
    ret_ADC[1] = hal_adc_init(&v2);
    ret_ADC[2] = hal_adc_init(&v3);

    if (ret_ADC[0] || ret_ADC[1] || ret_ADC[2]){
        _disp(0, "ADC Failed",1);
        aos_msleep(10000);
        aos_reboot();
    }

    //初始USB-TTL pipeline [nonblocking]
    ret = ioctl(uart_fd, IOC_UART_SET_CFLAG, B19200 | CS8);
    if (ret != 0) {
        close(uart_fd);
        fprintf(stderr, "ioctl uart error\r\n");
        _disp(0, "UART Failed",1);
        aos_msleep(10000);
        aos_reboot();
    }

    //绑定开关量触发handler
    hal_gpio_enable_irq(&tg,IRQ_TRIGGER_RISING_EDGE,toggle_handler,NULL);
    hal_gpio_enable_irq(&tg2,IRQ_TRIGGER_RISING_EDGE,toggle_handler2,NULL);
    aos_timer_start(&send_timer);//开始数据传输
    aos_timer_start(&recv_timer);//开始数据接收

    while(send_state)//send_state为
    {   
        /* sampling speed
         * 数据采样频率
        */
        if(s1){
            aos_msleep(50);
            if(!edge){  
                edge = 1;
                switch(state){
                case 0:
                    _disp(0,"NK-7001",0);
                    break;
                case 1:
                    _disp(0,"HD-1202E",0);
                    break;
                case 2:
                    _disp(0,"Model-520-2",0);
                    break;
                default:
                    break;
                };
            }
        }
        else{
            aos_msleep(1000);
            if(edge){
               switch(state){
                case 0:
                    _disp(0,"NK-7001",1);
                    break;
                case 1:
                    _disp(0,"HD-1202E",1);
                    break;
                case 2:
                    _disp(0,"Model-520-2",1);
                    break;
                default:
                    break;
                };
                edge = 0;
            }
            
        }

        data.state = (s1<<2) | state;
        hal_adc_value_get(&v1, &(data.ADC[0]),1000);
        hal_adc_value_get(&v2, &(data.ADC[1]),1000);
        hal_adc_value_get(&v3, &(data.ADC[2]),1000);
        //转换数据
        data.ADC[0] = Electro_Meter(data.ADC[0]);
        ringbuf_push(&ring, &data, sizeof(packet));
        realtime = data.ADC[0];
        //micro USB 数据读写示例
        ret = read(uart_fd, rfid_data_buf, sizeof(rfid_data_buf));
        if (ret > 0) {
            printf("read length:%d\r\n", ret);
            for (int i = 0; i < ret; i++) {
                printf("%02x ", rfid_data_buf[i]);
            }
            printf("\r\n");
            write(uart_fd,rfid_data_buf,ret);
        }
        else{
            printf("reading\r\n");
        }

    }
    
    //停止数据发送
    aos_timer_stop(&send_timer);
    aos_timer_free(&send_timer);
    
    //停止数据接收
    aos_timer_stop(&recv_timer);
    aos_timer_free(&recv_timer);

    //停止更新屏幕数据
    aos_timer_stop(&disp_timer);
    aos_timer_free(&disp_timer);

    //释放内存
    fprintf(stderr, "TCP_SEND failed\n");
    _disp(0,"Send Failed",1);
    lwip_close(fd);
    close(fd);

    hal_gpio_clear_irq(&tg);
    hal_gpio_clear_irq(&tg2);
    hal_gpio_finalize(&tg);
    hal_gpio_finalize(&tg2);

    hal_adc_finalize(&v1);
    hal_adc_finalize(&v2);
    hal_adc_finalize(&v3);
    _disp(0, "finalized",1);
    // aos_free(ret_ADC); //this line is toxic, causes memory problem & system reboots
    event_publish(EVENT_NETMGR_DHCP_SUCCESS,NULL);
    return;
}

void toggle_handler(void* arg){
    fprintf(stderr,"TOGGGGLE: %d\n",state);
    if (s1)
        return;
    state++;
    state%=3;
    switch(state){
        case 0:
            _disp(0,"NK-7001",1);
            ctrl_set(1);
            break;
        case 1:
            _disp(0,"HD-1202E",1);
            ctrl_set(0);
            break;
        case 2:
            _disp(0,"Model-520-2",1);
            ctrl_set(1);
            break;
        default:
            break;
    };
}

void toggle_handler2(void* arg){
    fprintf(stderr,"SWIIIITCH: %d\n",s1);
    if(s1)
        s1=false;
    else{
        s1 = true;

    }
        

}

unsigned short get_sum(byte* check, const int len){
    int temp = 0;
    for(int i = 0; i < len/4; i++){
        *(check+len+i) = 0;
        temp = 0;
        for(int j = 0; j < 4; j++){
            temp +=  *(check+j*len/4+i);
        }
        *(check+len+i) = temp % 0xFF;
        *(check+len+i) = ~*(check+len+i);
    }
    return len/4;
}

int check_sum(byte* sent, unsigned short recvd){
    unsigned short* start = (unsigned short*)sent;
	while (recvd && *start != 0xFFFF) {
		recvd--;
	}
	if (recvd <= 16) {
		return -1;
	}

	byte* check = (byte*)(start + 1);
	unsigned short* data_len = start + 2;
	unsigned short check_len = (*data_len + 4) / 4;
	for (int i = 0; i < check_len; i++) {
		int sum = 0;
		for (int j = 0; j < 5; j++) {
			sum += *(check + j * check_len + i);
		}
		sum %= 0xFF;
		if (sum) {
			return 1;
		}
	}
	return 0;
}

int Electro_Meter(int measured) {
	if (slope == .0) {
		slope = (V_high - V_low) / ((double)(M_high - M_low));
		offset = (double)(V_high) - slope * M_high;
        slope2 = (V_high2 - V_low2) / ((double)(M_high2 - M_low2));
		offset2 = (double)(V_high2) - slope2 * M_high2;
	}
    if(state == 0 | state == 2)
	    return (int)(measured * slope + offset);
    else if(state == 1)
        return (int)(measured * slope2 + offset2);
    return 0;
}