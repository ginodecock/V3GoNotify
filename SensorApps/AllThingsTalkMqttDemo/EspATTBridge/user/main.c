#include <esp8266.h>
#include "httpd.h"

#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "cgiflash.h"
#include "auth.h"
#include "espfs.h"
#include "captdns.h"
#include "webpages-espfs.h"

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "cmd.h"
#include "driver/uart.h"
#define FPM_SLEEP_MAX_TIME 0xFFFFFFF
#ifdef ESPFS_POS
CgiUploadEspfsParams espfsParams={
	.espFsPos=ESPFS_POS,
	.espFsSize=ESPFS_SIZE
};
#endif
uint32 user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "192.168.4.1"},
	{"/", cgiRedirect, "/wifiGB/wifi.tpl"},
//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},
	{"/wifiGB", cgiRedirect, "/wifiGB/wifi.tpl"},
	{"/wifiGB/", cgiRedirect, "/wifiGB/wifi.tpl"},
	{"/wifiGB/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifiGB/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifiGB/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifiGB/connstatus.cgi", cgiWiFiConnStatus, NULL},
	{"/wifiGB/setmode.cgi", cgiWiFiSetMode, NULL},
	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

void ICACHE_FLASH_ATTR
bridge_init(void)
{
	CMD_Init();
}

void ICACHE_FLASH_ATTR setup_wifi_softap_mode(void)
{
   wifi_set_opmode_current(3);
   wifi_softap_dhcps_start();
   struct softap_config apconfig;
   if(wifi_softap_get_config(&apconfig))
   {
      os_memset(apconfig.ssid, 0, sizeof(apconfig.ssid));
      os_sprintf(apconfig.ssid, "%s_%x", "GoNotify",system_get_chip_id());
      apconfig.authmode = AUTH_OPEN;
      apconfig.ssid_len = 0;
      apconfig.max_connection = 1;
	  if(!wifi_softap_set_config(&apconfig))
      {
         #ifdef PLATFORM_DEBUG
        // ets_uart_printf("ESP8266 not set softap config!\n");
         #endif
      }
   }
   if(wifi_get_phy_mode() != PHY_MODE_11N)
      wifi_set_phy_mode(PHY_MODE_11N);
}

//extern UartDevice UartDev;	
void ICACHE_FLASH_ATTR
user_init(void)
{
	uart_init(BIT_RATE_38400, BIT_RATE_38400);
	wifi_station_set_auto_connect(FALSE);
	system_init_done_cb(bridge_init);
	setup_wifi_softap_mode();
	
	wifi_fpm_do_wakeup();
	wifi_fpm_close();

	#ifdef ESPFS_POS
		espFsInit((void*)(0x40200000 + ESPFS_POS));
	#else
		espFsInit((void*)(webpages_espfs_start));	
	#endif
	httpdInit(builtInUrls, 80);
	
	//sntp_stop();
	//sntp_setservername(0, "pool.ntp.org");
	//sntp_setservername(1, "time.nist.gov");
	//sntp_setservername(2, "2.be.pool.ntp.org");
	//sntp_set_timezone(2);
	//sntp_init();
			
}
