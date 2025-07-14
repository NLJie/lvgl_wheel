/*
 * @Author: xiaozhi 
 * @Date: 2024-09-30 01:45:55 
 * @Last Modified by: xiaozhi
 * @Last Modified time: 2024-10-08 19:47:01
 */
#include "utils/wifimg.h"
#include "utils/includes.h"
#include <dirent.h>
#include <pthread.h>
#include "common/cli.h"
#include "common/wpa_ctrl.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/edit.h"
#include "utils/list.h"
#include "common/version.h"
#include "common/ieee802_11_defs.h"
#include "utils/base_utils.h"
#include "osal_thread.h"
#include "wpa_manager.h"

static osal_thread_t event_thread = NULL;

static struct wpa_ctrl *g_pstWpaCtrl = NULL;

static WPA_WIFI_STATUS_E g_wifi_status = WPA_WIFI_CLOSE;
static WPA_WIFI_CONNECT_STATUS_E g_connect_status = WPA_WIFI_INACTIVE;

connect_status_callback_fun connect_status_func = NULL;
wifi_status_callback_fun wifi_status_func = NULL;

static int wifi_send_command(const char *cmd, char *reply, size_t *reply_len)
{
    int ret;
    if (g_pstWpaCtrl == NULL) {
        printf("Not connected to wpa_supplicant - \"%s\" command dropped.\n", cmd);
        return -1;
    }
    ret = wpa_ctrl_request(g_pstWpaCtrl, cmd, strlen(cmd), reply, reply_len, NULL);
    if (ret == -2) {
        printf("'%s' command timed out.\n", cmd);
        return -2;
    } else if (ret < 0 || strncmp(reply, "FAIL", 4) == 0) {
		printf("'%s' command fail.\n", cmd);
        return -1;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
        reply[*reply_len] = '\0';
    }
    return 0;
}

static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static int wifi_command(char const *cmd, char *reply, size_t reply_len)
{
	pthread_mutex_lock(&mut);
	if(!cmd || !cmd[0]){
		pthread_mutex_unlock(&mut);
		return -1;
	}
	printf("do cmd %s\n", cmd);
	if (wifi_send_command(cmd, reply, &reply_len) != 0) {
		pthread_mutex_unlock(&mut);
		return -1;
	}
	pthread_mutex_unlock(&mut);
	return 0;
}

static void wpa_manager_wifi_on(){
	char cmdstr[200];
	if(base_utils_get_process_state("wpa_supplicant",14) == 1)
		return;
	sprintf(cmdstr,"ifconfig %s up", STA_IFNAME);
	base_utils_system(cmdstr);
	sprintf(cmdstr, "wpa_supplicant -i %s -c %s -B",STA_IFNAME,STA_CONFIG_PATH);
	base_utils_system(cmdstr);
}

static void wpa_manger_wifi_off(){
	char cmdstr[200];
	sprintf(cmdstr,"ifconfig %s down", STA_IFNAME);
	base_utils_system(cmdstr);
	sprintf(cmdstr,"/etc/init.d/wpa_supplicant stop"); 
	base_utils_system(cmdstr);
}

static int wpa_manager_connect_socket(){
	char au8Path[128] = {"\0"};
	if(g_pstWpaCtrl == NULL){
		sprintf(au8Path, "/etc/wifi/wpa_supplicant/sockets/%s",STA_IFNAME);
		g_pstWpaCtrl = wpa_ctrl_open(au8Path);
		if(g_pstWpaCtrl == NULL)
		{
			printf("\x1b[31m""wpa_ctrl_open failed:%s!\n",strerror(errno));
			return -1;
		}
	}
	printf("wpa_manager_connect_socket success\n");
	return wpa_ctrl_attach(g_pstWpaCtrl);
}

static void wpa_manager_disconnect_socket(){
	wpa_ctrl_detach(g_pstWpaCtrl);
	wpa_ctrl_close(g_pstWpaCtrl);
	g_pstWpaCtrl = NULL;
}

void wpa_manager_wifi_disconnect(){
	int ret;
	char au8ReplyBuf[2048] = {"\0"};
	size_t reply_len;
	char au8SsidBuf[128] = {"\0"};
	reply_len = sizeof(au8ReplyBuf)-1;
	ret = wifi_command(au8SsidBuf, au8ReplyBuf, reply_len);

	snprintf(au8SsidBuf, sizeof(au8SsidBuf)-1, "DISCONNECT");
	ret = wifi_command(au8SsidBuf, au8ReplyBuf, reply_len);

	snprintf(au8SsidBuf, sizeof(au8SsidBuf)-1, "REMOVE_NETWORK all");
	ret = wifi_command(au8SsidBuf, au8ReplyBuf, reply_len);

	snprintf(au8SsidBuf, sizeof(au8SsidBuf)-1, "SAVE_CONFIG");
	ret = wifi_command(au8SsidBuf, au8ReplyBuf, reply_len);

	g_connect_status = WPA_WIFI_DISCONNECT;
	if(connect_status_func != NULL)
		connect_status_func(g_connect_status);
}

void wpa_manger_wifi_status(){
	int ret;
	char au8ReplyBuf[2048] = {"\0"};
	size_t reply_len;
	char cmd[128] = {"\0"};
	reply_len = sizeof(au8ReplyBuf)-1;
	snprintf(cmd, sizeof(cmd)-1, "STATUS");
	ret = wifi_command(cmd, au8ReplyBuf, reply_len);
	if(ret == 0){
		if(strstr(au8ReplyBuf, "DISCONNECTED") != NULL){
			g_connect_status = WPA_WIFI_DISCONNECT;
		}else if(strstr(au8ReplyBuf, "COMPLETED") != NULL){
			g_connect_status = WPA_WIFI_CONNECT;
		}else if(strstr(au8ReplyBuf, "SCANNING") != NULL){
			g_connect_status = WPA_WIFI_SCANNING;
		}else if(strstr(au8ReplyBuf, "INACTIVE") != NULL){
			g_connect_status = WPA_WIFI_INACTIVE;
		}
		if(connect_status_func != NULL)
			connect_status_func(g_connect_status);
	}
}

int wpa_manager_wifi_connect(wpa_ctrl_wifi_info_t *wifi_info){
	int s32NetId = -1;
	char au8ReplyBuf[2048] = {"\0"};
	size_t reply_len;
	int ret;
	memset(au8ReplyBuf, '\0', sizeof(au8ReplyBuf));
	ret = wifi_command("REMOVE_NETWORK all", au8ReplyBuf, reply_len);

	memset(au8ReplyBuf, '\0', sizeof(au8ReplyBuf));
	ret = wifi_command("SAVE_CONFIG", au8ReplyBuf, reply_len);

	memset(au8ReplyBuf, '\0', sizeof(au8ReplyBuf));
	reply_len = sizeof(au8ReplyBuf)-1;
	ret = wifi_command("ADD_NETWORK", au8ReplyBuf, reply_len);
	if(ret == 0)
	{
		au8ReplyBuf[reply_len] = '\0';
    }
	s32NetId = atoi(au8ReplyBuf);
	/* wpa_ctrl_request SET_NETWORK */
	char au8SsidBuf[128] = {"\0"};
	snprintf(au8SsidBuf, sizeof(au8SsidBuf)-1, "SET_NETWORK %d ssid \"%s\"", s32NetId, wifi_info->ssid);
	memset(au8ReplyBuf, '\0', sizeof(au8ReplyBuf));
	reply_len = sizeof(au8ReplyBuf)-1;
	ret = wifi_command(au8SsidBuf, au8ReplyBuf, reply_len);
	if(ret == 0)
	{
		au8ReplyBuf[reply_len] = '\0';
		printf("reply_len:%ld, au8ReplyBuf:%s\n",reply_len, au8ReplyBuf);
	}
	else
    {
        return -1;
    }

	/* wpa_ctrl_request SET_NETWORK */
	char au8PskBuf[128] = {"\0"};
	snprintf(au8PskBuf, sizeof(au8PskBuf)-1, "SET_NETWORK %d psk \"%s\"", s32NetId, wifi_info->psw);
	memset(au8ReplyBuf, '\0', sizeof(au8ReplyBuf));
	reply_len = sizeof(au8ReplyBuf)-1;
	ret = wifi_command(au8PskBuf, au8ReplyBuf, reply_len);
	if(ret == 0)
	{
		au8ReplyBuf[reply_len] = '\0';
		printf("reply_len:%ld, au8ReplyBuf:%s\n",reply_len, au8ReplyBuf);
	}
	else
    {
        return -1;
    }

	 /* wpa_ctrl_request ENABLE_NETWORK */
	char au8EnableBuf[64] = {"\0"};
	snprintf(au8EnableBuf, sizeof(au8EnableBuf)-1, "ENABLE_NETWORK %d", s32NetId);
	memset(au8ReplyBuf, '\0', sizeof(au8ReplyBuf));
	reply_len = sizeof(au8ReplyBuf)-1;
	ret = wifi_command(au8EnableBuf, au8ReplyBuf, reply_len);
	if(ret == 0)
	{
		au8ReplyBuf[reply_len] = '\0';
		printf("reply_len:%ld, au8ReplyBuf:%s\n",reply_len, au8ReplyBuf);
	}
	else
    {
        return -1;
    }

	char au8ConnectBuf[128] = {"\0"};
	snprintf(au8ConnectBuf, sizeof(au8ConnectBuf)-1, "SELECT_NETWORK %d",s32NetId);
	ret = wifi_command(au8ConnectBuf, au8ReplyBuf, reply_len);
	if(ret == 0)
	{
		au8ReplyBuf[reply_len] = '\0';
		printf("reply_len:%ld, au8ReplyBuf:%s\n",reply_len, au8ReplyBuf);
	}
	return ret;
}

void *wpa_manager_event_thread(void *arg)
{
    char au8ReplyBuf[2048] = {"\0"};
    size_t reply_len;
    int ret,count = 0;
    char au8Cmdline[64] = {"\0"};
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	wpa_manager_wifi_on();
	while(wpa_manager_connect_socket() != 0){
		osal_thread_sleep(1000);
		count ++;
		if(count >= 10){
			printf("wpa_manager_connect_socket error\n");
			pthread_exit(NULL);
		}
	}
	g_wifi_status = WPA_WIFI_OPEN;
	if(wifi_status_func != NULL)
		wifi_status_func(g_wifi_status);
	wpa_manger_wifi_status();
	while(1)
	{
		if(g_pstWpaCtrl != NULL){
			if(wpa_ctrl_pending(g_pstWpaCtrl) > 0)
			{
				char buf[2048];
				size_t len = sizeof(buf) - 1;
				if(wpa_ctrl_recv(g_pstWpaCtrl, buf, &len) == 0)
				{
					buf[len] = '\0';
					printf("wpa_ctrl_recv %s\n",buf);
					if(strstr(buf, "CTRL-EVENT-CONNECTED") != NULL)
					{
						sprintf(au8Cmdline, "udhcpc -i %s -t 5 -T 2 -A 5 -q",STA_IFNAME);
						system(au8Cmdline);
						wifi_command("SAVE_CONFIG", au8ReplyBuf, reply_len);
						g_connect_status = WPA_WIFI_CONNECT;
						if(connect_status_func != NULL)
							connect_status_func(g_connect_status);
					}else if(strstr(buf, "CTRL-EVENT-DISCONNECTED") != NULL){
						g_connect_status = WPA_WIFI_DISCONNECT;
						if(connect_status_func != NULL)
							connect_status_func(g_connect_status);
					}else if(strstr(buf,"CTRL-EVENT-SSID-TEMP-DISABLED") != NULL){
						g_connect_status = WPA_WIFI_WRONG_KEY;
						if(connect_status_func != NULL)
							connect_status_func(g_connect_status);
					}
				}
			}else
        		osal_thread_sleep(10);

		}else{
	        osal_thread_sleep(10);
    	}
	}
	pthread_exit(NULL);
}

void wpa_manager_close(){
	wpa_manager_disconnect_socket();
	wpa_manger_wifi_off();
    if(event_thread != NULL)
		osal_thread_cancel(&event_thread);
	osal_thread_join(&event_thread, NULL); 
	event_thread = NULL;
	g_wifi_status = WPA_WIFI_CLOSE;
	g_connect_status = WPA_WIFI_INACTIVE;
	if(wifi_status_func != NULL)
		wifi_status_func(g_wifi_status);
	if(connect_status_func != NULL)
		connect_status_func(g_connect_status);
}


int wpa_manager_open(){
    int ret = OSAL_ERROR;
    if(event_thread != NULL){
        return 0;
    }
    ret = osal_thread_create(&event_thread,wpa_manager_event_thread, NULL);
    if(ret == OSAL_ERROR)
	{
		printf("create thread error\n");
		return -1;
	}
	return 0;
}

void wpa_manager_add_callback(wifi_status_callback_fun wifi_status_f,
								connect_status_callback_fun connect_status_f){
	wifi_status_func = wifi_status_f;						
	connect_status_func = connect_status_f;
}