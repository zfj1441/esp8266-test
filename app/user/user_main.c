#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "gpio.h"


LOCAL int keystat = 0;

/*
 * set wifi station
 */
void user_set_station_config(){
	struct station_config stConf;
	stConf.bssid_set = 0;
	os_memcpy(stConf.ssid, SSID, 32);
	os_memcpy(stConf.password, PASSWORD, 64);
	wifi_set_opmode(STATION_MODE);
	wifi_station_set_config(&stConf);
	return ;
}

/*
 * wifi callbak 
 */
void wifi_handle_even_cb(System_Event_t *evt)
{
	switch(evt->event){
		case EVENT_STAMODE_GOT_IP:
			os_printf("Have got IP!!\n");
			break;
		defaut: 
			os_printf("Have'n got IP\n");
			break;
	}
}

void ICACHE_FLASH_ATTR
webserver_recv(void *arg, char *pusrdata, unsigned short length)
{
	;
}

void ICACHE_FLASH_ATTR
webserver_recon(void *arg, sint8 err)
{
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", pesp_conn->proto.tcp->remote_ip[0],
			pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
			pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err);
}

void ICACHE_FLASH_ATTR
webserver_discon(void *arg)
{
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d disconnect\n", pesp_conn->proto.tcp->remote_ip[0],
				pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
				pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port);
}

void ICACHE_FLASH_ATTR
webserver_listen(void *arg)
{
	struct espconn *pesp_conn = arg;

	espconn_regist_recvcb(pesp_conn, webserver_recv);
	espconn_regist_reconcb(pesp_conn, webserver_recon);
	espconn_regist_disconcb(pesp_conn, webserver_discon);
}

void ICACHE_FLASH_ATTR
tcp_server(int port, espconn_connect_callback serListen)
{
	LOCAL struct espconn esp_conn_ser;
	LOCAL esp_tcp esptcp;

	esp_conn_ser.type = ESPCONN_TCP;
	esp_conn_ser.state = ESPCONN_NONE;
	esp_conn_ser.proto.tcp = &esptcp;
	esp_conn_ser.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn_ser, serListen);

	sint8 ret = espconn_accept(&esp_conn_ser);
	os_printf("%d\n", ret);
	if(!ret){
		os_printf("Begin to listen!!!\n");
	}
	else{
		os_printf("Fail to listen!!!\n");
	}
}


#if 0
void ICACHE_FLASH_ATTR
user_udp_send(void)
{
	char hwaddr[6];
	char DeviceBuffer[40]={0};
	LOCAL char udp_remote_ip[1]={255,255,255,255};

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

void ICACHE_FLASH_ATTR
user_udp_sent_cb(void *arg){
	os_timer_disarm(&test_timer);
	os_timer_setfn(&test_timer,user_udp_send,NULL);
	os_timer_arm(&test_timer,1000,NULL);
}
#endif

void ICACHE_FLASH_ATTR
gpio_intr_handler()
{
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
	if(gpio_status & (BIT(0)))
	{
		//disable interrupt
		gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_DISABLE);

		//clear interrupt status
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(0));

		if (keystat == 0) {
			keystat = 1;
			gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_POSEDGE);
		} else {
			if(GPIO_INPUT_GET(2)==0) {
				GPIO_OUTPUT_SET(GPIO_ID_PIN(2),1);
			}
			else {
				GPIO_OUTPUT_SET(GPIO_ID_PIN(2),0);
			}
			keystat = 0;
			gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_NEGEDGE);
		}
	}
}

void ICACHE_FLASH_ATTR 
user_init(void)
{
	//	set wifi station  mode
	user_set_station_config();
//	set wifi connect ap callback;
//	wifi_set_event_handler_cb(wifi_handle_even_cb);
	uint8 wifi_sat = wifi_station_get_connect_status();
	if(wifi_sat!=STATION_GOT_IP){
		os_printf("Wifi connect fail[%d]", wifi_sat);
		return;
	}
	os_printf("Wifi connect success!");
	
	// start tcp server
	tcp_server(9988, webserver_listen);
	
	os_delay_us(10000);

	gpio_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(2),0);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
	ETS_GPIO_INTR_DISABLE();
	ETS_GPIO_INTR_ATTACH(&gpio_intr_handler, NULL);
	gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_NEGEDGE);
	ETS_GPIO_INTR_ENABLE();
	return;
}
