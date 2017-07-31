#include "osapi.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "mem.h"
#include "gpio.h"

/*
 * set wifi station
 */
void user_set_station_config(){
	struct station_config stationconf;

	char ssid[32]     = SID;
	char password[64] = PASSWD;

	stationconf.bssid_set = 0;   
	memset(stationconf.ssid, 0, sizeof(stationconf.ssid));
	memset(stationconf.password, 0, sizeof(stationconf.password));

	memcpy(stationconf.ssid, ssid, sizeof(ssid));
	memcpy(stationconf.password, password, sizeof(password));

	wifi_station_set_config(&stationconf);
	return ;
}


void wifi_handle_even_cb(System_Event_t *evt)
{
	switch(evt->event_id){
		case EVENT_STAMODE_GOT_IP:
//			flag_sta_conip = TRUE;
			os_printf("Have got IP!!\n");
			break;
		defaut: 
			os_printf("Have'n got IP\n");
			break;
	}
}


void espconn_ser_timer_cb(void *timer_arg)
{
	user_tcp_conn.proto.tcp = &user_tcp;
	user_tcp_conn.type      = ESPCONN_TCP;
	user_tcp_conn.state     = ESPCONN_NONE;

	os_timer_disarm(&ser_timer);

	user_tcp_conn.proto.tcp->local_port = espconn_port();
	os_printf("The local port is %d\n",user_tcp_conn.proto.tcp->local_port);

	sint8 ret = espconn_accept(&user_tcp_conn);
	os_printf("%d\n", ret);
	if(!ret){
		os_printf("Begin to listen!!!\n");
	}
	else{
		os_printf("Fail to listen!!!\n");
		os_timer_setfn(&ser_timer, espconn_ser_timer_cb, NULL);
		os_timer_arm(&ser_timer, 900, 0);
	}
}

 
void espconn_recv_data_cb(void *arg, char *pdata, unsigned short len)
{
	uint8 *pDat;
	const char str[] = "Light ON";

	pDat = (uint8 *)malloc(len + 1);
	memcpy(pDat, pdata, len);
	*(pDat+len) = 0;
	// pDat[len] = 0;

	os_printf("The receiver data is [%s]",pDat);
	os_printf("\n\n");

	if(memcmp(pDat, str, sizeof(str)) == 0) {
		os_printf("Now Light is Run!\n");
		espconn_send(&user_tcp_conn, "Now Light is Run!\n", 18);
	}
	free(pDat);
}

 
void espconn_ser_timer_cb(void *timer_arg)
{
	user_tcp_conn.proto.tcp = &user_tcp;
	user_tcp_conn.type      = ESPCONN_TCP;
	user_tcp_conn.state     = ESPCONN_NONE;


	os_timer_disarm(&ser_timer);
	user_tcp_conn.proto.tcp->local_port = espconn_port();
	os_printf("The local port is %d\n",user_tcp_conn.proto.tcp->local_port);

	sint8 ret = espconn_accept(&user_tcp_conn);
	os_printf("%d\n", ret);
	if(!ret){
		os_printf("Begin to listen!!!\n");
	}
	else{
		os_printf("Fail to listen!!!\n");
		os_timer_setfn(&ser_timer, espconn_ser_timer_cb, NULL);
		os_timer_arm(&ser_timer, 900, 0);
	}
}

void ICACHE_FLASH_ATTR user_udp_send(void){
	char hwaddr[6];
	char DeviceBuffer[40]={0};
	const char udp_remote_ip[1]={255,255,255,255};

	user_udp_espconn.type=ESPCONN_UDP; 
	user_udp_espconn.proto.udp= (esp_udp*)os_zalloc(sizeof(esp_udp)); 
	os_memset(user_udp_espconn.proto.udp, 0, sizeof(esp_udp));
	user_udp_espconn.proto.udp->local_port=2525; 
	user_udp_espconn.proto.udp->remote_port=1982;
	os_memcpy(user_udp_espconn.proto.udp->remote_ip,udp_remote_ip)
	espconn_create(&user_udp_espconn);

	wifi_get_macaddr(STATION_IF,hwaddr);
	os_sprintf(DeviceBuffer,"设备MAC地址为"MACSTR"!!\r\n",MAC2STR(hwaddr));
	espconn_sent(&user_udp_espconn,DeviceBuffer,os_strlen(DeviceBuffer));
}

void ICACHE_FLASH_ATTR user_udp_sent_cb(void *arg){
	os_printf("发送成功！");
	os_timer_disarm(&test_timer);
	os_timer_setfn(&test_timer,user_udp_send,NULL);
	os_timer_arm(&test_timer,1000,NULL);
}

static void gpio_intr_handler()
{
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

	if(gpio_status & (BIT(2)))
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);
	}
}

void ICACHE_FLASH_ATTR user_init(void)
{
	//	set wifi station  mode
	user_set_station_config();
	//	set wifi connect ap callback;
	wifi_set_event_handler_cb(wifi_handle_even_cb);
	unit8 wifi_sat = wifi_station_get_connect_status();
	if(wifi_sat!=STATION_GOT_IP){
		os_printf("Wifi connect fail[%d]", wifi_sat);
		return;
	}
	os_printf("Wifi connect success!");
	
	// start tcp server
	espconn_ser_timer_cb();
	espconn_regist_recvcb(&user_tcp_conn, espconn_recv_data_cb);
//	espconn_regist_sentcb(&user_tcp_conn, espconn_sent_cb);
	
	os_timer_disarm(&ser_timer);
	os_timer_setfn(&ser_timer, espconn_ser_timer_cb, NULL);
	os_timer_arm(&ser_timer, 900, 0);

	//setting led light
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO2);

	gpio_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(2));
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
	ETS_GPIO_INTR_DISABLE();
	ETS_GPIO_INTR_ATTACH(&gpio_intr_handler, NULL);
	gpio_pin_intr_state_set(GPIO_ID_PIN(2), GPIO_PIN_INTR_NEGEDGE);
	ETS_GPIO_INTR_ENABLE();
}