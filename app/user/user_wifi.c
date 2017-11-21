/*
 * user_wifi.c
 *
 *  Created on: 2017-07-15
 *      Author: yannanxiu
 */

#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "gpio.h"

#include "user_smartconfig.h"
#include "user_wifi.h"


// Wi-Fi �¼��ص�ʹ��
#define WIFI_EVENT_ENABLE		0


static struct ip_info g_ipConfig;
static os_timer_t g_wifi_start_timer;

/*************************************************************/

/*
 * ������get_station_ip
 * ˵������ȡIP��ַ
 */
u32 ICACHE_FLASH_ATTR
get_station_ip(void)
{
	return g_ipConfig.ip.addr;
}

/*
 * ������user_smartconfig_init
 * ˵����smartconfig��ʼ��
 */
static void ICACHE_FLASH_ATTR
user_smartconfig_init(void)
{
	//esptouch_set_timeout(30);
	smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
	smartconfig_start(smartconfig_done);
	os_printf("[INFO] smartconfig start!\r\n");

	// Ӧ�÷��� SC_STATUS_FIND_CHANNEL ��ʼ��
	//user_smartconfig_led_timer_init();
	//GPIO_HIGH(WIFI_STATUS_LED_PIN);
}


/*
 * ������wifi_start_timer_cb
 * ˵�����ϵ��smartconfig���ڻص�
 */
static void ICACHE_FLASH_ATTR
wifi_start_timer_cb(void *arg)
{
	static u8 s_smartconig_flag = 0;

//	os_timer_disarm(&g_wifi_start_timer);	// ֹͣ�ö�ʱ��
	wifi_get_ip_info(STATION_IF, &g_ipConfig);		// ��ȡstation��Ϣ
	u8 wifi_status = wifi_station_get_connect_status();

	wifi_station_set_reconnect_policy(TRUE);	// AP�Ͽ��Զ�����
	wifi_station_set_auto_connect(TRUE);		// �ϵ��Զ�����

	// ��WiFi���ӳɹ���
	if (STATION_GOT_IP == wifi_status && g_ipConfig.ip.addr != 0){
		os_timer_disarm(&g_wifi_start_timer);	// ֹͣ�ö�ʱ��
		os_printf("[INFO] Wi-Fi Connected!\r\n");
	}else{
		os_printf("[INFO] Wi-Fi not Connect!\r\n");
#if 0
		if(0==s_smartconig_flag){	// û�л�ȡ��smartconfig
			os_printf("[INFO] smartconfig stop!\r\n");
			smartconfig_stop();
			s_smartconig_flag = 1;		// ��־���ٽ���ô���
		}else{
			os_printf("[ERROR] Wi-Fi Connect Fail!\r\n");
		}
#endif
		wifi_station_disconnect();
		wifi_station_connect();		// ���Խ�������
	}
}


#if WIFI_EVENT_ENABLE
/*
 * ������wifi_handle_event_cb
 * ˵����WiFi�¼��ص���������ʱû��
 */
void ICACHE_FLASH_ATTR
wifi_handle_event_cb(System_Event_t *evt)
{
	os_printf("event %x\n", evt->event);
	switch (evt->event) {
		case EVENT_STAMODE_CONNECTED:
			os_printf("connect to ssid %s, channel %d\n",
			evt->event_info.connected.ssid,
			evt->event_info.connected.channel);
		break;
		case EVENT_STAMODE_DISCONNECTED:
			os_printf("disconnect from ssid %s, reason %d\n",
			evt->event_info.disconnected.ssid,
			evt->event_info.disconnected.reason);
		break;
		case EVENT_STAMODE_AUTHMODE_CHANGE:
			os_printf("mode: %d -> %d\n",
			evt->event_info.auth_change.old_mode,
			evt->event_info.auth_change.new_mode);
		break;
		case EVENT_STAMODE_GOT_IP:
		os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
			IP2STR(&evt->event_info.got_ip.ip),
			IP2STR(&evt->event_info.got_ip.mask),
			IP2STR(&evt->event_info.got_ip.gw));
			os_printf("\n");

			// TODO:

		break;
			case EVENT_SOFTAPMODE_STACONNECTED:
			os_printf("station: " MACSTR "join, AID = %d\n",
			MAC2STR(evt->event_info.sta_connected.mac),
			evt->event_info.sta_connected.aid);
		break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
			os_printf("station: " MACSTR "leave, AID = %d\n",
			MAC2STR(evt->event_info.sta_disconnected.mac),
			evt->event_info.sta_disconnected.aid);
		break;
		default:
		break;
	}
}
#endif

/*
 * ������wifi_connect
 * ˵����
 */
void ICACHE_FLASH_ATTR
wifi_connect(void)
{
	wifi_set_opmode(STATION_MODE);		// ����ΪSTATIONģʽ
	wifi_station_disconnect();

	wifi_station_set_reconnect_policy(TRUE);	// ���� AP ʧ�ܻ�Ͽ����Ƿ�������
	wifi_station_set_auto_connect(TRUE);		// �ر��Զ�����

	// ��������SmartConfig
	user_smartconfig_init();

	// һ��ʱ�����Wi-Fi����״̬
	os_timer_disarm(&g_wifi_start_timer);
	os_timer_setfn(&g_wifi_start_timer, (os_timer_func_t *)wifi_start_timer_cb, NULL);
	os_timer_arm(&g_wifi_start_timer, 20000, 0);	// 20������һ�λص�

	// ����smartconfig�Ĵ��ڣ���û��Ƕ�ʱ���Wi-FI����״̬
	// ע�� Wi-Fi �¼�
#if WIFI_EVENT_ENABLE
	wifi_set_event_handler_cb(wifi_handle_event_cb);
#endif
}


