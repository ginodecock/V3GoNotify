/*
 * wifi.c
 *
 *  Created on: Dec 30, 2014
 *      Author: Minh
 */
#include "wifi.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"

#include "user_config.h"
#include "cmd.h"
#include "debug.h"
//#include "cgiwifi.h"

static ETSTimer WiFiLinker;
uint32_t wifiCb = NULL;
static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;


static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{
	struct ip_info ipConfig;

	os_timer_disarm(&WiFiLinker);
	wifi_get_ip_info(STATION_IF, &ipConfig);
	wifiStatus = wifi_station_get_connect_status();
	if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0)
	{
		//save ipaddress
		/*uint8_t *insertIPString;
		insertIPString = (uint8_t*)os_zalloc(32);
		os_sprintf(insertIPString,"%d.%d.%d.%d",IP2STR(&ipConfig.ip));
		
		config Webconfig;
		os_memset(&Webconfig, 0, sizeof(config));
		spi_flash_read(CONFIG_ADDRESS ,(uint32 *)(&Webconfig),sizeof(config));
		spi_flash_erase_sector(CONFIG_SECTOR);
		
		Webconfig.firstconfig = 5;
		os_strncpy((char*)Webconfig.ipaddress, insertIPString, 32);
		spi_flash_write(CONFIG_ADDRESS ,(uint32 *)&Webconfig,sizeof(config));
		
		INFO(Webconfig.ipaddress);*/
		
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
		os_timer_arm(&WiFiLinker, 2000, 0);
	}
	else
	{
		if(wifi_station_get_connect_status() == STATION_WRONG_PASSWORD)
		{
			INFO("STATION_WRONG_PASSWORD\r\n");
			wifi_station_connect();
		}
		else if(wifi_station_get_connect_status() == STATION_NO_AP_FOUND)
		{
			INFO("STATION_NO_AP_FOUND\r\n");
			wifi_station_connect();
		}
		else if(wifi_station_get_connect_status() == STATION_CONNECT_FAIL)
		{
			INFO("STATION_CONNECT_FAIL\r\n");
			wifi_station_connect();
		}
		else
		{
			INFO("STATION_IDLE\r\n");
			
		}

		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
		os_timer_arm(&WiFiLinker, 500, 0);
	}
	if(wifiStatus != lastWifiStatus){
		lastWifiStatus = wifiStatus;
		if(wifiCb){
			uint16_t crc = CMD_ResponseStart(CMD_WIFI_CONNECT, wifiCb, 0, 1);
			crc = CMD_ResponseBody(crc, (uint8_t*)&wifiStatus, 1);
			CMD_ResponseEnd(crc);
		}
	}
}

uint32_t ICACHE_FLASH_ATTR
WIFI_Connect(PACKET_CMD *cmd)
{
	struct station_config stationConf;
	uint8_t *ssid, *pwd;
	ssid = (uint8_t*)&cmd->args;
	pwd = ssid + *(uint16_t*)ssid + 2;
	INFO("WIFI_INIT\r\n");
	os_memset(&stationConf, 0, sizeof(struct station_config));
	os_memcpy(stationConf.ssid, ssid + 2, *(uint16_t*)ssid);
	os_memcpy(stationConf.password, pwd + 2, *(uint16_t*)pwd);
	
	
	
	INFO("WIFI: set ssid = %s, pass= %s\r\n", stationConf.ssid, stationConf.password);
	wifi_station_set_auto_connect(TRUE);
	
	wifi_fpm_do_wakeup();
	wifi_fpm_close();
	//wifi_set_opmode_current(3);
	wifi_set_opmode_current(1);
	/*struct ip_info info;
		wifi_station_dhcpc_stop();
		IP4_ADDR(&info.ip, 192, 168, 168, 201);
		IP4_ADDR(&info.gw, 192, 168, 168, 138);
		IP4_ADDR(&info.netmask, 255, 255, 255, 0);
		wifi_set_ip_info(STATION_IF, &info);*/
	//wifi_station_disconnect();
wifi_station_connect();
	if(cmd->argc != 2 || cmd->callback == 0)
		return 0xFFFFFFFF;

	wifiCb = cmd->callback;
	
	char *p;
	if(!(p = strstr(stationConf.ssid, "#SSID#"))){   //get saved wifi config
		INFO("Setting new config\r\n");
		//wifi_station_get_config_default(&stationConf);
		wifi_station_set_config(&stationConf);
	}else{
		INFO("using default wifi config\r\n");
	}
	

	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 1000, 0);

	wifi_station_set_auto_connect(TRUE);
	wifi_station_connect();
	return 0;
}
uint32_t ICACHE_FLASH_ATTR
WIFI_Auto_Connect(PACKET_CMD *cmd)
{
	INFO("WIFI_INIT\r\n");
	wifi_station_set_auto_connect(FALSE);
	wifi_set_opmode_current(1);
	
	wifiCb = cmd->callback;
	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 1000, 0);

	wifi_station_set_auto_connect(TRUE);
	wifi_station_connect();
	
		
	return 0;
}
