
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

/*
This is example code for the esphttpd library. It's a small-ish demo showing off 
the server, including WiFi connection management capabilities, some IO and
some pictures of cats.
*/

#include <esp8266.h>
#include "httpd.h"
#include "io.h"
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
#include "driver/uart_register.h"
//The example can print out the heap use every 3 seconds. You can use this to catch memory leaks.
#define SHOW_HEAP_USE
#define FPM_SLEEP_MAX_TIME 0xFFFFFFF

#ifdef ESPFS_POS
CgiUploadEspfsParams espfsParams={
	.espFsPos=ESPFS_POS,
	.espFsSize=ESPFS_SIZE
};
#endif
user_rf_cal_sector_set(void)
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

// UartDev is defined and initialized in rom code.
extern UartDevice    UartDev;
//extern os_event_t    at_recvTaskQueue[at_recvTaskQueueLen];

LOCAL void uart0_rx_intr_handler(void *para);

/******************************************************************************
 * FunctionName : uart_config
 * Description  : Internal used function
 *                UART0 used for data TX/RX, RX buffer size is 0x100, interrupt enabled
 *                UART1 just used for debug output
 * Parameters   : uart_no, use UART0 or UART1 defined ahead
 * Returns      : NONE
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
uart_config(uint8 uart_no)
{
  if (uart_no == UART1)
  {
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
  }
  else
  {
    /* rcv_buff size if 0x100 */
    ETS_UART_INTR_ATTACH(uart0_rx_intr_handler,  &(UartDev.rcv_buff));
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
  }

  uart_div_modify(uart_no, UART_CLK_FREQ / (UartDev.baut_rate));

  WRITE_PERI_REG(UART_CONF0(uart_no), UartDev.exist_parity
                 | UartDev.parity
                 | (UartDev.stop_bits << UART_STOP_BIT_NUM_S)
                 | (UartDev.data_bits << UART_BIT_NUM_S));

  //clear rx and tx fifo,not ready
  SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
  CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);

  //set rx fifo trigger
//  WRITE_PERI_REG(UART_CONF1(uart_no),
//                 ((UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) |
//                 ((96 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) |
//                 UART_RX_FLOW_EN);
  if (uart_no == UART0)
  {
    //set rx fifo trigger
    WRITE_PERI_REG(UART_CONF1(uart_no),
                   ((0x10 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) |
                   ((0x10 & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S) |
                   UART_RX_FLOW_EN |
                   (0x02 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S |
                   UART_RX_TOUT_EN);
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_TOUT_INT_ENA |
                      UART_FRM_ERR_INT_ENA);
  }
  else
  {
    WRITE_PERI_REG(UART_CONF1(uart_no),
                   ((UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));
  }

  //clear all interrupt
  WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
  //enable rx_interrupt
  SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA);
}

/******************************************************************************
 * FunctionName : uart1_tx_one_char
 * Description  : Internal used function
 *                Use uart1 interface to transfer one char
 * Parameters   : uint8 TxChar - character to tx
 * Returns      : OK
*******************************************************************************/
LOCAL STATUS
uart_tx_one_char(uint8 uart, uint8 TxChar)
{
    while (true)
    {
      uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S);
      if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
        break;
      }
    }

    WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
    return OK;
}
/******************************************************************************
 * FunctionName : uart1_write_char
 * Description  : Internal used function
 *                Do some special deal while tx char is '\r' or '\n'
 * Parameters   : char c - character to tx
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart1_write_char(char c)
{
  if (c == '\n')
  {
    uart_tx_one_char(UART1, '\r');
    uart_tx_one_char(UART1, '\n');
  }
  else if (c == '\r')
  {
  }
  else
  {
    uart_tx_one_char(UART1, c);
  }
}
void ICACHE_FLASH_ATTR
uart0_write_char(char c)
{
  if (c == '\n')
  {
    uart_tx_one_char(UART0, '\r');
    uart_tx_one_char(UART0, '\n');
  }
  else if (c == '\r')
  {
  }
  else
  {
    uart_tx_one_char(UART0, c);
  }
}
/******************************************************************************
 * FunctionName : uart0_tx_buffer
 * Description  : use uart0 to transfer buffer
 * Parameters   : uint8 *buf - point to send buffer
 *                uint16 len - buffer len
 * Returns      :
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart0_tx_buffer(uint8 *buf, uint16 len)
{
  uint16 i;

  for (i = 0; i < len; i++)
  {
    uart_tx_one_char(UART0, buf[i]);
  }
}

/******************************************************************************
 * FunctionName : uart0_sendStr
 * Description  : use uart0 to transfer buffer
 * Parameters   : uint8 *buf - point to send buffer
 *                uint16 len - buffer len
 * Returns      :
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart0_sendStr(const char *str)
{
	while(*str)
	{
		uart_tx_one_char(UART0, *str++);
	}
}

/******************************************************************************
 * FunctionName : uart0_rx_intr_handler
 * Description  : Internal used function
 *                UART0 interrupt handler, add self handle code inside
 * Parameters   : void *para - point to ETS_UART_INTR_ATTACH's arg
 * Returns      : NONE
*******************************************************************************/

void ICACHE_FLASH_ATTR
uart1_sendStr(const char *str)
{
	while(*str)
	{
		uart_tx_one_char(UART1, *str++);
	}
}

/******************************************************************************
 * FunctionName : uart0_rx_intr_handler
 * Description  : Internal used function
 *                UART0 interrupt handler, add self handle code inside
 * Parameters   : void *para - point to ETS_UART_INTR_ATTACH's arg
 * Returns      : NONE
*******************************************************************************/
//extern void at_recvTask(void);

LOCAL void
uart0_rx_intr_handler(void *para)
{

  uint8 RcvChar;
  uint8 uart_no = UART0;//UartDev.buff_uart_no;


	if(UART_FRM_ERR_INT_ST == (READ_PERI_REG(UART_INT_ST(uart_no)) & UART_FRM_ERR_INT_ST))
	{
		//os_printf("FRM_ERR\r\n");
		WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
	}

	if(UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(uart_no)) & UART_RXFIFO_FULL_INT_ST))
	{
		ETS_UART_INTR_DISABLE();/////////

		while(READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S))
		{
			WRITE_PERI_REG(0X60000914, 0x73); //WTD
			RcvChar = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
			CMD_Input(RcvChar);
		}

	}
	else if(UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(uart_no)) & UART_RXFIFO_TOUT_INT_ST))
	{
		ETS_UART_INTR_DISABLE();/////////
		while(READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S))
		{
			WRITE_PERI_REG(0X60000914, 0x73); //WTD
			RcvChar = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
			CMD_Input(RcvChar);
		}

	}

	if(UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST))
	{
	  WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	}
	else if(UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST))
	{
	  WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}
	ETS_UART_INTR_ENABLE();


}

/******************************************************************************
 * FunctionName : uart_init
 * Description  : user interface for init uart
 * Parameters   : UartBautRate uart0_br - uart0 bautrate
 *                UartBautRate uart1_br - uart1 bautrate
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart_init(UartBautRate uart0_br, UartBautRate uart1_br)
{
  // rom use 74880 baut_rate, here reinitialize
  UartDev.baut_rate = uart0_br;
  uart_config(UART0);
  UartDev.baut_rate = uart1_br;
  uart_config(UART1);
  ETS_UART_INTR_ENABLE();
  os_install_putc1((void *)uart0_write_char);
}

void ICACHE_FLASH_ATTR
uart_reattach()
{
	uart_init(BIT_RATE_38400, BIT_RATE_38400);
//  ETS_UART_INTR_ATTACH(uart_rx_intr_handler_ssc,  &(UartDev.rcv_buff));
//  ETS_UART_INTR_ENABLE();
}
ICACHE_FLASH_ATTR int uart0_rx_one_char() {
  if(UartDev.rcv_buff.pReadPos == UartDev.rcv_buff.pWritePos) return -1;
  int ret = *UartDev.rcv_buff.pReadPos;
  UartDev.rcv_buff.pReadPos++;
  if(UartDev.rcv_buff.pReadPos == (UartDev.rcv_buff.pRcvMsgBuff + RX_BUFF_SIZE)) {
    UartDev.rcv_buff.pReadPos = UartDev.rcv_buff.pRcvMsgBuff;
  }
  return ret;
}
void ICACHE_FLASH_ATTR setup_wifi_softap_mode(void)
{
   struct softap_config apconfig;
   if(wifi_softap_get_config(&apconfig))
   {
//      wifi_softap_dhcps_stop();
      os_memset(apconfig.ssid, 0, sizeof(apconfig.ssid));
      //os_memset(apconfig.password, 0, sizeof(apconfig.password));
      os_sprintf(apconfig.ssid, "%s_%x", "GoNotify",system_get_chip_id());
      //os_sprintf(apconfig.password, "%s", "12345678");
      apconfig.authmode = AUTH_OPEN;
      apconfig.ssid_len = 0;
      apconfig.max_connection = 1;
      if(!wifi_softap_set_config(&apconfig))
      {
         #ifdef PLATFORM_DEBUG
         ets_uart_printf("ESP8266 not set softap config!\n");
         #endif
      }
      //wifi_set_opmode(SOFTAP_MODE);
	  //wifi_set_opmode_current(3);
//      wifi_softap_dhcps_start();
   }
   if(wifi_get_phy_mode() != PHY_MODE_11N)
      wifi_set_phy_mode(PHY_MODE_11N);
   #ifdef PLATFORM_DEBUG
   ets_uart_printf("ESP8266 in SoftAP mode configured.\n");
   #endif
}
#ifdef SHOW_HEAP_USE
static ETSTimer prHeapTimer;

static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg) {
	setup_wifi_softap_mode();
	
	wifi_fpm_do_wakeup();
	wifi_fpm_close();
	wifi_set_opmode_current(3);
	
	wifi_softap_dhcps_start();
	
	os_printf("Heap: %ld\n", (unsigned long)system_get_free_heap_size());
	#ifdef ESPFS_POS
		espFsInit((void*)(0x40200000 + ESPFS_POS));
	#else
		espFsInit((void*)(webpages_espfs_start));	
	#endif
	httpdInit(builtInUrls, 80);
	os_printf("Heap: %ld\n", (unsigned long)system_get_free_heap_size());
}
#endif
//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) {
		wifi_set_opmode(NULL_MODE);
		wifi_set_sleep_type(MODEM_SLEEP_T);
		wifi_fpm_open();
		wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
		/*int a;
		for( a = 0; a < 20; a++ ){
			os_delay_us(65000);
			os_printf(".");
		}*/
		//os_printf("\n");
		//system_phy_set_max_tpw(0);
		//system_phy_set_powerup_option(2);
	    //system_phy_set_tpw_via_vdd33(3000);
		wifi_station_set_auto_connect(FALSE);
		uart_init(BIT_RATE_38400, BIT_RATE_38400);
		system_init_done_cb(bridge_init);
	
#ifdef SHOW_HEAP_USE
		os_timer_disarm(&prHeapTimer);
		os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
		os_timer_arm(&prHeapTimer, 15000, 1);
#endif
}
void user_rf_pre_init() {
	//Not needed, but some SDK versions want this defined.
	
}
