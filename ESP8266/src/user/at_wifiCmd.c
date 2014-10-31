#include "user_interface.h"
#include "at.h"
#include "at_wifiCmd.h"
#include "osapi.h"
#include "c_types.h"
#include "mem.h"

at_mdStateType mdState = m_unlink;

extern BOOL specialAtState;
extern at_stateType at_state;
extern at_funcationType at_fun[];

uint8_t at_wifiMode;
os_timer_t at_japDelayChack;

/** @defgroup AT_WSIFICMD_Functions
  * @{
  */

/**
  * @brief  Copy param from receive data to dest.
  * @param  pDest: point to dest
  * @param  pSrc: point to source
  * @param  maxLen: copy max number of byte
  * @retval the length of param
  *   @arg -1: failure
  */
int8_t ICACHE_FLASH_ATTR
at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen)
{
//  assert(pDest!=NULL && pSrc!=NULL);

  char *pTempD = pDest;
  const char *pTempS = pSrc;
  int8_t len;

  if(*pTempS != '\"')
  {
    return -1;
  }
  pTempS++;
  for(len=0; len<maxLen; len++)
  {
    if(*pTempS == '\"')
    {
      *pTempD = '\0';
      break;
    }
    else
    {
      *pTempD++ = *pTempS++;
    }
  }
  if(len == maxLen)
  {
    return -1;
  }
  return len;
}

/**
  * @brief  Test commad of set wifi mode.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdCwmode(uint8_t id)
{
  char temp[32];
  os_sprintf(temp, "%s:(1-3)\r\n", at_fun[id].at_cmdName);
  uart0_sendStr(temp);
  at_backOk;
}

/**
  * @brief  Query commad of set wifi mode.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCwmode(uint8_t id)
{
  char temp[32];

  at_wifiMode = wifi_get_opmode();
  os_sprintf(temp, "%s:%d\r\n", at_fun[id].at_cmdName, at_wifiMode);
  uart0_sendStr(temp);
  at_backOk;
}

/**
  * @brief  Setup commad of set wifi mode.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCwmode(uint8_t id, char *pPara)
{
  uint8_t mode;
  char temp[32];

  pPara++;
  mode = atoi(pPara);
  if(mode == at_wifiMode)
  {
    uart0_sendStr("no change\r\n");
    return;
  }
  if((mode >= 1) && (mode <= 3))
  {
    ETS_UART_INTR_DISABLE();
    wifi_set_opmode(mode);
    ETS_UART_INTR_ENABLE();
    at_wifiMode = mode;
    at_backOk;
//    system_restart();
  }
  else
  {
    at_backError;
  }
}

/**
  * @brief  Wifi ap scan over callback to display.
  * @param  arg: contain the aps information
  * @param  status: scan over status
  * @retval None
  */
static void ICACHE_FLASH_ATTR
scan_done(void *arg, STATUS status)
{
  uint8 ssid[33];
  char temp[128];

  if (status == OK)
  {
    struct bss_info *bss_link = (struct bss_info *)arg;
    bss_link = bss_link->next.stqe_next;//ignore first

    while (bss_link != NULL)
    {
      os_memset(ssid, 0, 33);
      if (os_strlen(bss_link->ssid) <= 32)
      {
        os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid));
      }
      else
      {
        os_memcpy(ssid, bss_link->ssid, 32);
      }
      os_sprintf(temp,"+CWLAP:(%d,\"%s\",%d,\""MACSTR"\",%d)\r\n",
                 bss_link->authmode, ssid, bss_link->rssi,
                 MAC2STR(bss_link->bssid),bss_link->channel);
      uart0_sendStr(temp);
      bss_link = bss_link->next.stqe_next;
    }
    at_backOk;
  }
  else
  {
//  	os_sprintf(temp,"err, scan status %d\r\n", status);
//  	uart0_sendStr(temp);
    at_backError;
  }
  specialAtState = TRUE;
  at_state = at_statIdle;
}

//void ICACHE_FLASH_ATTR
//at_testCmdCwjap(uint8_t id)
//{
//  wifi_station_scan(scan_done);
//  specialAtState = FALSE;
//}

void ICACHE_FLASH_ATTR
at_setupCmdCwlap(uint8_t id, char *pPara)
{
  struct scan_config config;
  int8_t len;
  char ssid[32];
  char bssidT[18];
  char bssid[6];
  uint8_t channel;
  uint8_t i;

  pPara++;
//  os_printf("%x\r\n",*pPara);////
//  os_bzero(ssid, 32);

  len = at_dataStrCpy(ssid, pPara, 32);
//  os_printf("%d\r\n",len);////

  if(len == -1)
  {
    at_backError;
//    uart0_sendStr(at_backTeError"11\r\n");
    return;
  }
  if(len == 0)
  {
    config.ssid = NULL;
  }
  else
  {
    config.ssid = ssid;
  }
  pPara += (len+3);
//  os_printf("%x\r\n",*pPara);/////
  len = at_dataStrCpy(bssidT, pPara, 18);
//  os_printf("%d\r\n",len);////
  if(len == -1)
  {
    at_backError;
  //    uart0_sendStr(at_backTeError"11\r\n");
    return;
  }
  if(len == 0)
  {
    config.bssid = NULL;
  }
  else
  {
    if(os_str2macaddr(bssid, bssidT) == 0)
    {
      at_backError;
      return;
    }
    config.bssid = bssid;
  }
//  if(*pPara == ',')
//  {
//    config.bssid = NULL;
//  }
//  else
//  {
//    os_memcpy(bssidT,pPara,17);
//    bssidT[17] = '\0';
//    os_str2macaddr(bssid, bssidT);
//    config.bssid = bssid;
//  }
  pPara += (len+3);
  config.channel = atoi(pPara);

//  os_printf("%s,%s,%d\r\n",
//            config.ssid,
//            config.bssid,
//            config.channel);
  if(wifi_station_scan(&config, scan_done))
  {
    specialAtState = FALSE;
  }
  else
  {
    at_backError;
  }
}

/**
  * @brief  Execution commad of list wifi aps.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCwlap(uint8_t id)//need add mode chack
{
  if(at_wifiMode == SOFTAP_MODE)
  {
    at_backError;
    return;
  }
  wifi_station_scan(NULL,scan_done);
  specialAtState = FALSE;
}

/**
  * @brief  Query commad of join to wifi ap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCwjap(uint8_t id)
{
  char temp[64];
  struct station_config stationConf;
  wifi_station_get_config(&stationConf);
  struct ip_info pTempIp;

  wifi_get_ip_info(0x00, &pTempIp);
  if(pTempIp.ip.addr == 0)
  {
    at_backError;
    //    os_sprintf(temp, at_backTeError, 4);
    //    uart0_sendStr(at_backTeError"4\r\n");
    return;
  }
  mdState = m_gotip;
  if(stationConf.ssid != 0)
  {
    os_sprintf(temp, "%s:\"%s\"\r\n", at_fun[id].at_cmdName, stationConf.ssid);
    uart0_sendStr(temp);
    at_backOk;
  }
  else
  {
    at_backError;
    //    os_sprintf(temp, at_backTeError, 5);
    //    uart0_sendStr(at_backTeError"5\r\n");
  }
}

/**
  * @brief  Transparent data through ip.
  * @param  arg: no used
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_japChack(void *arg)
{
  static uint8_t chackTime = 0;
  uint8_t japState;

  os_timer_disarm(&at_japDelayChack);
  chackTime++;
  japState = wifi_station_get_connect_status();
  if(japState == STATION_GOT_IP)
  {
    chackTime = 0;
    at_backOk;
    specialAtState = TRUE;
    at_state = at_statIdle;
    return;
  }
  else if(chackTime >= 7)
  {
    wifi_station_disconnect();
    chackTime = 0;
    uart0_sendStr("\r\nFAIL\r\n");
    specialAtState = TRUE;
    at_state = at_statIdle;
    return;
  }
  os_timer_arm(&at_japDelayChack, 2000, 0);

}

/**
  * @brief  Setup commad of join to wifi ap.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCwjap(uint8_t id, char *pPara)
{
  char temp[64];
  struct station_config stationConf;

  int8_t len;

  if (at_wifiMode == SOFTAP_MODE)
  {
    at_backError;
    return;
  }
  pPara++;
  len = at_dataStrCpy(&stationConf.ssid, pPara, 32);
  if(len != -1)
  {
    pPara += (len+3);
    len = at_dataStrCpy(&stationConf.password, pPara, 64);
  }
  if(len != -1)
  {
    wifi_station_disconnect();
    mdState = m_wdact;
    ETS_UART_INTR_DISABLE();
    wifi_station_set_config(&stationConf);
    ETS_UART_INTR_ENABLE();
    wifi_station_connect();
//    if(1)
//    {
//      mdState = m_wact;
//    }
//    os_sprintf(temp,"%s:%s,%s\r\n",
//               at_fun[id].at_cmdName,
//               stationConf.ssid,
//               stationConf.password);
//    uart0_sendStr(temp);
    os_timer_disarm(&at_japDelayChack);
    os_timer_setfn(&at_japDelayChack, (os_timer_func_t *)at_japChack, NULL);
    os_timer_arm(&at_japDelayChack, 3000, 0);
    specialAtState = FALSE;
  }
  else
  {
    at_backError;
  }
}

/**
  * @brief  Test commad of quit wifi ap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_testCmdCwqap(uint8_t id)
{
  at_backOk;
}

/**
  * @brief  Execution commad of quit wifi ap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdCwqap(uint8_t id)
{
  wifi_station_disconnect();
  mdState = m_wdact;
  at_backOk;
}

/**
  * @brief  Query commad of module as wifi ap.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_queryCmdCwsap(uint8_t id)
{
  struct softap_config apConfig;
  char temp[128];

  if(at_wifiMode == STATION_MODE)
  {
    at_backError;
    return;
  }
  wifi_softap_get_config(&apConfig);
  os_sprintf(temp,"%s:\"%s\",\"%s\",%d,%d\r\n",
             at_fun[id].at_cmdName,
             apConfig.ssid,
             apConfig.password,
             apConfig.channel,
             apConfig.authmode);
  uart0_sendStr(temp);
  at_backOk;
}

/**
  * @brief  Setup commad of module as wifi ap.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCwsap(uint8_t id, char *pPara)
{
  char temp[64];
  int8_t len,passLen;
  struct softap_config apConfig;

  wifi_softap_get_config(&apConfig);

  if(at_wifiMode == STATION_MODE)
  {
    at_backError;
//    uart0_sendStr(at_backTeError"6\r\n");
    return;
  }
  pPara++;
  len = at_dataStrCpy(apConfig.ssid, pPara, 32);
  if(len < 1)
  {
//    uart0_sendStr("ssid ERROR\r\n");
    at_backError;
//    uart0_sendStr(at_backTeError"7\r\n");
    return;
  }
  pPara += (len+3);
  passLen = at_dataStrCpy(apConfig.password, pPara, 64);
  if(passLen == -1 )
  {
//    uart0_sendStr("pwd ERROR\r\n");
    at_backError;
//    uart0_sendStr(at_backTeError"8\r\n");
    return;
  }
  pPara += (passLen+3);
//  os_printf("%x\r\n",*pPara);/////
  apConfig.channel = atoi(pPara);
  if(apConfig.channel<1 || apConfig.channel>13)
  {
//    uart0_sendStr("ch ERROR\r\n");
    at_backError;
//    uart0_sendStr(at_backTeError"9\r\n");
    return;
  }
  pPara++;
  pPara = strchr(pPara, ',');
  pPara++;
  apConfig.authmode = atoi(pPara);
  if(apConfig.authmode >= 5)
  {
//    uart0_sendStr("s ERROR\r\n");
    at_backError;
//    uart0_sendStr(at_backTeError"10\r\n");
    return;
  }
  if((apConfig.authmode != 0)&&(passLen < 5))
  {
    at_backError;
//    uart0_sendStr(at_backTeError"8\r\n");
    return;
  }
//  os_sprintf(temp,"%s,%s,%d,%d\r\n",
//             apConfig.ssid,
//             apConfig.password,
//             apConfig.channel,
//             apConfig.authmode);
//  uart0_sendStr(temp);
  ETS_UART_INTR_DISABLE();
  wifi_softap_set_config(&apConfig);
  ETS_UART_INTR_ENABLE();
  at_backOk;
//  system_restart();
}

void ICACHE_FLASH_ATTR
at_exeCmdCwlif(uint8_t id)
{
  struct station_info *station;
  struct station_info *next_station;
  char temp[128];

  if(at_wifiMode == STATION_MODE)
  {
    at_backError;
    return;
  }
  station = wifi_softap_get_station_info();
  while(station)
  {
    os_sprintf(temp, "%d.%d.%d.%d,"MACSTR"\r\n",
               IP2STR(&station->ip), MAC2STR(station->bssid));
    uart0_sendStr(temp);
    next_station = STAILQ_NEXT(station, next);
    os_free(station);
    station = next_station;
  }
  at_backOk;
}

/**
  * @}
  */
