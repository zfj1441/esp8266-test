/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *	 2014/1/1, v1.0 create this file.
*******************************************************************************/

#include "mem.h"

#include "ets_sys.h"  
#include "gpio.h"   
#include "osapi.h"   
#include "os_type.h"	
#include "user_interface.h"   
#include "espconn.h"   
#include "cJSON.h"
	 
#define ESPCONN_MAXMUTLI 5
#define ESPCONN_MAXLENGTH 1460

typedef struct _TransportInfo{
	struct espconn *EspconnListen;
	bool ListenFlag;
}TransportInfo;

TransportInfo TransportList[ESPCONN_MAXMUTLI];
LOCAL os_timer_t EspconnMutliSendTimer;

LOCAL os_timer_t timer;   
LOCAL int zt = 0;   			// GPIO stat
LOCAL int keystat = 0;

void Espconn_MutliSend(char* szSendData)
{
	uint8 MutliSendCount = 0;
	static uint32 MutliSendNum = 0;
	char SendBufferOfMutli[ESPCONN_MAXLENGTH];
//	os_memset(SendBufferOfMutli, MutliSendNum, ESPCONN_MAXLENGTH);
//	os_memcpy(SendBufferOfMutli, "hello world", 11);
	for (MutliSendCount = 0; MutliSendCount < ESPCONN_MAXMUTLI; MutliSendCount ++){
		if (TransportList[MutliSendCount].ListenFlag){			
			espconn_send(TransportList[MutliSendCount].EspconnListen, szSendData, ESPCONN_MAXLENGTH);			
		}
	}
}

void Espconn_MutliRemove(void *arg)
{
	TransportInfo *TransportInfoOfRemove = arg;
	uint8 TransportListCount = 0;
	for (TransportListCount = 0; TransportListCount < ESPCONN_MAXMUTLI; TransportListCount ++){
		if (TransportInfoOfRemove == &TransportList[TransportListCount]){			
		   TransportList[TransportListCount].ListenFlag = FALSE;
		   TransportList[TransportListCount].EspconnListen = NULL;
		   break;
		}
	}
}

void Espconn_MutliDisFn(void *arg)
{
	struct espconn *EspconnOfMutliDis = arg;
	if (EspconnOfMutliDis != NULL)
		Espconn_MutliRemove(EspconnOfMutliDis->reverse);
}

void Espconn_MutliRecvFn(void *arg, char *pdata, unsigned short len)
{
	int	iArraySize = 0, index = 0;
	char	szRet[1460], tmp[10];
	char	*out;
	cJSON	*json, *root, *outJosn;
	cJSON	*itemArray, *item;

	os_memset(szRet, 0, sizeof(szRet));
	struct espconn *EspconnOfMutliRecv = arg;
	json=cJSON_Parse(pdata);
	if (!json) {
		os_sprintf(szRet, "Error before: [%s]\n",cJSON_GetErrorPtr());
	}
	else
	{
		int j = 0;
		for(j = 0; j < 1; j++)
		{
			itemArray=cJSON_GetObjectItem(json, "params");
			if(!itemArray || itemArray->type != cJSON_Array )
			{
				os_sprintf(szRet, "params is not an array");
				break ;
			}
			iArraySize = cJSON_GetArraySize(itemArray);
			for (index = 0; index < iArraySize; index++)
			{
				item = cJSON_GetArrayItem(itemArray, index);
				if (NULL == item)  
				{  
					os_sprintf(szRet, "params is null");
					break ;
				}
				if(item->type==cJSON_String)
				{
					if(os_strcmp(item->valuestring, "power") == 0)
					{
						if(GPIO_INPUT_GET(2)==1)
							os_strcat(szRet, "{\"id\":-1, \"method\":\"get_prop\", \"power\":\"on\"}");
						else
							os_strcat(szRet, "{\"id\":-1, \"method\":\"get_prop\", \"power\":\"off\"}");
						break ;	// for test vr7jj
					}
				}
				else if(item->type == cJSON_Number)
				{
					os_memset(tmp, 0, sizeof(tmp));
					os_sprintf(tmp, "%d", item->valueint);
					os_strcat(szRet, tmp);
				}
				else
				{
					os_memset(szRet, 0, sizeof(szRet));
					os_sprintf(szRet, "array value is not string or int");
					break;
				}
			}
		}

//		out=cJSON_Print(json);
		cJSON_Delete(json);
//		os_memcpy(szRet, out, os_strlen(out)<=sizeof(szRet)?os_strlen(out):sizeof(szRet));
//		os_free(out);
	}
	Espconn_MutliSend(szRet);
}

void Espconn_MutliSendFn(void *arg)
{
	struct espconn *EspconnOfMutliSend = arg;
}

void Espconn_MutliReFn(void *arg, sint8 error)
{
	struct espconn *EspconnOfMutliRe = arg;
	if (EspconnOfMutliRe != NULL)
		Espconn_MutliRemove(EspconnOfMutliRe->reverse);
}

void Espconn_MutliListen(void *arg)
{
	struct espconn *EspconnOfMutliListen = arg;
	uint8 ListenCount = 0;
	
	if (EspconnOfMutliListen){
		for (ListenCount = 0; ListenCount < ESPCONN_MAXMUTLI; ListenCount ++){
			if (TransportList[ListenCount].ListenFlag == FALSE){
				TransportList[ListenCount].ListenFlag = TRUE;
				TransportList[ListenCount].EspconnListen = EspconnOfMutliListen;
				break;
			}
		}
		
		if (ListenCount >= ESPCONN_MAXMUTLI){
			espconn_disconnect(EspconnOfMutliListen);
			return;
		}
		EspconnOfMutliListen->reverse = &TransportList[ListenCount];
		espconn_regist_sentcb(EspconnOfMutliListen,Espconn_MutliSendFn);
		espconn_regist_recvcb(EspconnOfMutliListen,Espconn_MutliRecvFn);
		espconn_regist_disconcb(EspconnOfMutliListen,Espconn_MutliDisFn);
		espconn_regist_reconcb(EspconnOfMutliListen,Espconn_MutliReFn);
	}else
		return;
}

int8 Espconn_Mutli(uint32 port)
{
	struct espconn *MutliEspconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
	if(MutliEspconn != NULL){		
		MutliEspconn->proto.tcp = (esp_tcp*)os_zalloc(sizeof(esp_tcp));
		if (MutliEspconn->proto.tcp != NULL){
			MutliEspconn->type = ESPCONN_TCP;
			MutliEspconn->state = ESPCONN_NONE;
			MutliEspconn->proto.tcp->local_port = port;
			espconn_regist_connectcb(MutliEspconn,Espconn_MutliListen);
			os_memset(TransportList, 0, sizeof(TransportList));
			espconn_accept(MutliEspconn);
		}else{
			os_free(MutliEspconn);
			return ESPCONN_MEM;
		}
	}else
		return ESPCONN_MEM;
}

#if 0 
static
void gpio_intr_handler()
{
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
	if(gpio_status & (BIT(0)))
	{
		if(GPIO_INPUT_GET(2)==0) {
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2),1);
		}
		else {
			GPIO_OUTPUT_SET(GPIO_ID_PIN(2),0);
		}
	}
	return ;
}
#endif
static 
void gpio_intr_handler()
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


void ICACHE_FLASH_ATTR user_init(void)
{
	//wifi setting station mode
	struct station_config stConf;
	stConf.bssid_set = 0; 
	os_memcpy(stConf.ssid, SSID, 32);
	os_memcpy(stConf.password, PASSWORD, 64);
	wifi_set_opmode(STATION_MODE);
	wifi_station_set_config(&stConf);
	Espconn_Mutli(9988);

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


}
