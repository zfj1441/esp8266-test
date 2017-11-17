#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "espconn.h"
#define ESPCONN_MAXMUTLI 5
#define ESPCONN_MAXLENGTH 1460

typedef struct _TransportInfo{
	struct espconn *EspconnListen;
	bool ListenFlag;
}TransportInfo;

TransportInfo TransportList[ESPCONN_MAXMUTLI];
LOCAL os_timer_t EspconnMutliSendTimer;

void Espconn_MutliSend(void)
{
	uint8 MutliSendCount = 0;
	static uint32 MutliSendNum = 0;
	char SendBufferOfMutli[ESPCONN_MAXLENGTH];
	os_memset(SendBufferOfMutli, MutliSendNum, ESPCONN_MAXLENGTH);
	for (MutliSendCount = 0; MutliSendCount < ESPCONN_MAXMUTLI; MutliSendCount ++){
		if (TransportList[MutliSendCount].ListenFlag){			
			espconn_send(TransportList[MutliSendCount].EspconnListen, SendBufferOfMutli, ESPCONN_MAXLENGTH);			
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
	struct espconn *EspconnOfMutliRecv = arg;
	Espconn_MutliSend();
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
