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


/**********************************************************/
#define GPIO_HIGH(x) 	GPIO_OUTPUT_SET(x, 1)
#define GPIO_LOW(x) 	GPIO_OUTPUT_SET(x, 0)
// ����ȡ��
#define GPIO_REV(x) 	GPIO_OUTPUT_SET(x, 1-GPIO_INPUT_GET(x))
/**********************************************************/


// Wi-Fi �¼��ص�ʹ��
#define WIFI_EVENT_ENABLE		0
// Wi-Fi ״̬�ƶ���
#define WIFI_STATUS_LED_PIN		0

// Wi-Fi ��ʱ���
#if WiFi_CHECK_TIMER_ENABLE
	#define WiFi_CHECK_TIMER_INTERVAL	2000
	static os_timer_t g_wifi_check_timer;

	#define WiFi_LED_INTERVAL	200
	static os_timer_t g_wifi_led_timer;
#endif


static struct ip_info g_ipConfig;
static os_timer_t g_wifi_start_timer;
static os_timer_t g_smartconig_led_timer;

/*************************************************************/


#if WiFi_LED_STATUS_TIMER_ENABLE
/*
 * ������wifi_led_timer_cb
 * ˵����LED�ƶ�ʱ�ص�
 */
static void ICACHE_FLASH_ATTR
wifi_led_timer_cb(void *arg)
{
	//wifi_get_ip_info(STATION_IF, &g_ipConfig);		// ��ȡstation��Ϣ
	u8 status = wifi_station_get_connect_status();

	if(status == STATION_GOT_IP){
		GPIO_LOW(WIFI_STATUS_LED_PIN);
	}else{
		GPIO_REV(WIFI_STATUS_LED_PIN);
	}
}
#else
	#define HUMITURE_WIFI_LED_IO_MUX 	PERIPHS_IO_MUX_GPIO0_U
	#define HUMITURE_WIFI_LED_IO_NUM 	0
	#define HUMITURE_WIFI_LED_IO_FUNC 	FUNC_GPIO0
#endif

void ICACHE_FLASH_ATTR
wifi_status_led_init(void)
{
	static u8 s_wifi_led_init_flag = 0;
	// ȷ��ֻ��ʼ��һ��
	if(1==s_wifi_led_init_flag){
		return;
	}

	// ʹ�� GPIO0 ��Ϊ WiFi ״̬ LED
#if WiFi_LED_STATUS_TIMER_ENABLE
	os_timer_disarm(&g_wifi_start_timer);
	os_timer_setfn(&g_wifi_start_timer, (os_timer_func_t *)wifi_led_timer_cb, NULL);
	os_timer_arm(&g_wifi_start_timer, WiFi_LED_INTERVAL, 1);
#else
	wifi_status_led_install(HUMITURE_WIFI_LED_IO_NUM,
			HUMITURE_WIFI_LED_IO_MUX,
			HUMITURE_WIFI_LED_IO_FUNC
		);
#endif
	s_wifi_led_init_flag = 1;
}

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
 * ������smartconfig_led_timer_cb
 * ˵����smartconfig״̬�ƶ�ʱ���ص�����
 */
static void ICACHE_FLASH_ATTR
smartconfig_led_timer_cb(void *arg)
{
	GPIO_REV(WIFI_STATUS_LED_PIN);
}

/*
 * ������user_smartconfig_led_timer_init
 * ˵����smartconfig LED״̬�ƶ�ʱ��ʼ��
 */
void ICACHE_FLASH_ATTR
user_smartconfig_led_timer_init(void)
{
	os_timer_disarm(&g_smartconig_led_timer);
	os_timer_setfn(&g_smartconig_led_timer, (os_timer_func_t *)smartconfig_led_timer_cb, NULL);
	os_timer_arm(&g_smartconig_led_timer, 1000, 1);
}

/*
 * ������user_smartconfig_led_timer_stop
 * ˵����smartconfig LED״̬�ƶ�ʱֹͣ
 */
void ICACHE_FLASH_ATTR
user_smartconfig_led_timer_stop(void)
{
	os_timer_disarm(&g_smartconig_led_timer);
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
	GPIO_HIGH(WIFI_STATUS_LED_PIN);
}

#if WiFi_CHECK_TIMER_ENABLE

/*
 * ������wifi_check_timer_cb
 * ˵����Wi-Fi��鶨ʱ�ص�
 */
static void ICACHE_FLASH_ATTR
wifi_check_timer_cb(void)
{
	u8 status = wifi_station_get_connect_status();
	//os_printf("[INFO] wifi_connect_cb\r\n");

	// TODO:
	if(status == STATION_GOT_IP){

	}else{

	}
}

/*
 * ������wifi_check_timer_init
 * ˵����һ��ʱ�����Wi-Fi����״̬
 */
void ICACHE_FLASH_ATTR
wifi_check_timer_init(void)
{
	static u8 s_flag = 0;
	// ȷ��ֻ��ʼ��һ��
	if(1==s_flag){
		return;
	}
	os_timer_disarm(&g_wifi_check_timer);
	os_timer_setfn(&g_wifi_check_timer, (os_timer_func_t *)wifi_check_timer_cb, NULL);
	os_timer_arm(&g_wifi_check_timer, WiFi_CHECK_TIMER_INTERVAL, 1);	// ÿ2������һ�λص�

}

#endif

/*
 * ������wifi_start_timer_cb
 * ˵�����ϵ��smartconfig���ڻص�
 */
static void ICACHE_FLASH_ATTR
wifi_start_timer_cb(void *arg)
{
	static u8 s_smartconig_flag = 0;

	os_timer_disarm(&g_wifi_start_timer);	// ֹͣ�ö�ʱ��
	wifi_get_ip_info(STATION_IF, &g_ipConfig);		// ��ȡstation��Ϣ
	u8 wifi_status = wifi_station_get_connect_status();

	// ֹͣsmartconfig��led����˸
	user_smartconfig_led_timer_stop();
	// Wi-Fi ״̬�Ƴ�ʼ��
	wifi_status_led_init();
	// Wi-Fi ���״̬
	wifi_check_timer_init();

	wifi_station_set_reconnect_policy(TRUE);	// AP�Ͽ��Զ�����
	wifi_station_set_auto_connect(TRUE);		// �ϵ��Զ�����

	// ��WiFi���ӳɹ���
	if (STATION_GOT_IP == wifi_status && g_ipConfig.ip.addr != 0){
		os_printf("[INFO] Wi-Fi Connected!\r\n");
	}else{
		if(0==s_smartconig_flag){	// û�л�ȡ��smartconfig
			os_printf("[INFO] smartconfig stop!\r\n");
			smartconfig_stop();
			s_smartconig_flag = 1;		// ��־���ٽ���ô���
		}else{
			os_printf("[ERROR] Wi-Fi Connect Fail!\r\n");
		}
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

#if SMARTCONFIG_DISABLE
	wifi_station_set_reconnect_policy(TRUE);
	wifi_station_set_auto_connect(TRUE);
	wifi_station_connect();
	// Wi-Fi ״̬�Ƴ�ʼ��
	wifi_status_led_init();
	// Wi-Fi ���״̬
	wifi_check_timer_init();
#else
	wifi_station_set_reconnect_policy(FALSE);	// ���� AP ʧ�ܻ�Ͽ����Ƿ�������
	wifi_station_set_auto_connect(FALSE);		// �ر��Զ�����

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
#endif
}


