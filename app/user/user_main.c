#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "gpio.h"
#include "cJSON.h"


os_timer_t test_timer; 
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


void ICACHE_FLASH_ATTR
mytcp_recv(void *arg, char *pusrdata, unsigned short length)
{
	int		iArraySize = 0, index = 0;
	char	szRet[1460], tmp[10];
	char	*out;
	cJSON	*json, *root, *outJosn;
	cJSON	*itemArray, *item;

	os_memset(szRet, 0, sizeof(szRet));
	struct espconn *pesp_conn = arg;

	json=cJSON_Parse(pusrdata);
	if (!json) {
		os_sprintf(szRet, "Error before: [%s]\n",cJSON_GetErrorPtr());
	}
	else
	{
		do {
			item=cJSON_GetObjectItem(json, "method");
			if(!item) {
				os_sprintf(szRet, "json data have not an method");
				break ;
			}
			itemArray=cJSON_GetObjectItem(json, "params");
			if(!itemArray || itemArray->type != cJSON_Array ) {
				os_sprintf(szRet, "params is not an array");
				break ;
			}
			if(os_strcmp(item->valuestring, "get_prop") == 0) {
				if(GPIO_INPUT_GET(2)==1)
					os_strcat(szRet, "{\"id\":-1,\"method\":\"get_prop\",\"power\":\"on\"}");
				else
					os_strcat(szRet, "{\"id\":-1,\"method\":\"get_prop\",\"power\":\"off\"}");
			}
			else if(os_strcmp(item->valuestring, "set_power") == 0) {
				item = cJSON_GetArrayItem(itemArray, 0);
				if (NULL == item)
				{
					os_sprintf(szRet, "params is null");
					break ;
				}
				if(item->type==cJSON_String && os_strcmp(item->valuestring, "on") == 0)
				{
					GPIO_OUTPUT_SET(GPIO_ID_PIN(2),1);
					os_strcat(szRet, "{\"id\":1, \"result\":[\"on\", \"\", \"100\"]}");
				}
				else if(item->type==cJSON_String && os_strcmp(item->valuestring, "off") == 0)
				{
					GPIO_OUTPUT_SET(GPIO_ID_PIN(2),0);
					os_strcat(szRet, "{\"id\":1, \"result\":[\"off\", \"\", \"100\"]}");
				}
				else
				{
					os_strcat(szRet, "{\"id\":1, \"result\":[\"off\", \"\", \"100\"]}");
				}
			}
			else if(os_strcmp(item->valuestring, "set_bright") == 0) {
				;
			}
			else if(os_strcmp(item->valuestring, "set_hsv") == 0) {
				;
			}
			else if(os_strcmp(item->valuestring, "start_cf") == 0) {
				;
			}
			else if(os_strcmp(item->valuestring, "set_name") == 0) {
				;
			}
			else {
				os_sprintf(szRet, "error method[%s]", item->valuestring);
			}
		}while(0);
		cJSON_Delete(json);
	}
	espconn_send(pesp_conn, szRet, os_strlen(szRet));
	return;
}

void ICACHE_FLASH_ATTR
mytcp_recon(void *arg, sint8 err)
{
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", pesp_conn->proto.tcp->remote_ip[0],
			pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
			pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err);
}

void ICACHE_FLASH_ATTR
mytcp_discon(void *arg)
{
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d disconnect\n", pesp_conn->proto.tcp->remote_ip[0],
				pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
				pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port);
}

void ICACHE_FLASH_ATTR
mytcp_listen(void *arg)
{
	uint32_t keeplive;
	struct espconn *pesp_conn = arg;
#if 0
//	keeplive = 2147483647; 
	espconn_set_opt(pesp_conn, ESPCONN_KEEPALIVE);
	keeplive = 6000; 
	espconn_set_keepalive(pesp_conn, ESPCONN_KEEPIDLE, &keeplive); 
	keeplive = 6000; 
	espconn_set_keepalive(pesp_conn, ESPCONN_KEEPINTVL, &keeplive); 
	keeplive = 6000; 
	espconn_set_keepalive(pesp_conn, ESPCONN_KEEPCNT, &keeplive); 
#endif
	espconn_regist_recvcb(pesp_conn, mytcp_recv);
	espconn_regist_reconcb(pesp_conn, mytcp_recon);
	espconn_regist_disconcb(pesp_conn, mytcp_discon);
}

void ICACHE_FLASH_ATTR
mytcp_server(int port, espconn_connect_callback serListen)
{
	LOCAL struct espconn esp_conn_ser;
	LOCAL esp_tcp esptcp;

	esp_conn_ser.type = ESPCONN_TCP;
	esp_conn_ser.state = ESPCONN_NONE;
	esp_conn_ser.proto.tcp = &esptcp;
	esp_conn_ser.proto.tcp->local_port = port;

	espconn_regist_connectcb(&esp_conn_ser, serListen);

	espconn_accept(&esp_conn_ser);
	espconn_regist_time(&esp_conn_ser, 60, 0);
}

void ICACHE_FLASH_ATTR
user_udp_send(void)
{
	char szData[1024];
	LOCAL struct espconn esp_conn_udp;
	LOCAL esp_udp user_udp;

	esp_conn_udp.type=ESPCONN_UDP; 
	esp_conn_udp.state = ESPCONN_NONE;
	esp_conn_udp.proto.udp = &user_udp;
	esp_conn_udp.proto.udp->remote_port=1982;
	esp_conn_udp.proto.udp->remote_ip[0]=224;
	esp_conn_udp.proto.udp->remote_ip[1]=0;
	esp_conn_udp.proto.udp->remote_ip[2]=0;
	esp_conn_udp.proto.udp->remote_ip[3]=50;
	espconn_create(&esp_conn_udp);

	struct ip_info ipconfig;
	os_memset(&ipconfig, 0, sizeof(struct ip_info));
	wifi_get_ip_info(STATION_IF, &ipconfig); 

	os_memset(szData, 0, sizeof(szData));
	os_sprintf(szData, "did: did\nLocation: http://%d.%d.%d.%d:9988\npower: %s\nbright: bright\nmodel: model\nhue: hue\nsat: sat\nname: name", 
		IP2STR(&ipconfig.ip), GPIO_INPUT_GET(2)==0?"off":"on");
	espconn_sent(&esp_conn_udp,szData,os_strlen(szData));
}

void ICACHE_FLASH_ATTR
user_udp_sent_cb(void){

	os_timer_disarm(&test_timer);
	os_timer_setfn(&test_timer,user_udp_send,NULL);
	os_timer_arm(&test_timer,60000,NULL);
}

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

/*
 * wifi callbak 
 */
void wifi_handle_even_cb(System_Event_t *evt)
{
	switch(evt->event){
		case EVENT_STAMODE_GOT_IP:
			os_printf("Have got IP!!\n");
			mytcp_server(9988, mytcp_listen);
			user_udp_sent_cb();
			user_udp_send();
			break;
		defaut: 
			os_printf("Have'n got IP\n");
			break;
	}
}

void ICACHE_FLASH_ATTR 
user_init(void)
{
	//	set wifi station  mode
	user_set_station_config();
	wifi_set_event_handler_cb(wifi_handle_even_cb);
	
	// start tcp server
//	mytcp_server(9988, mytcp_listen);
	
	os_delay_us(1000);

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

	// start udp client
//	user_udp_sent_cb();
	return;
}
