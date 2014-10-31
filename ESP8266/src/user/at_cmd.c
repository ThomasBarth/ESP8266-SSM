#include "at_cmd.h"
#include "user_interface.h"
#include "osapi.h"
//#include<stdlib.h>

/** @defgroup AT_BASECMD_Functions
  * @{
  */ 

  
/**
  * @brief  Print all commands
  * @retval none
  */
void ICACHE_FLASH_ATTR
at_print_allCmd(void)
{
	uint8_t i;
	char temp[128];
	for(i=1;i<at_cmdNum;i++)
	{
		os_sprintf(temp,"%s\r\n", at_fun[i].at_cmdName);
		uart0_sendStr(temp);
	}
		
}

/**
  * @brief  Query and localization one commad.
  * @param  cmdLen: received length of command
  * @param  pCmd: point to received command 
  * @retval the id of command
  *   @arg -1: failure
  */
static int16_t ICACHE_FLASH_ATTR
at_cmdSearch(int8_t cmdLen, uint8_t *pCmd)
{
  int16_t i;

  if(cmdLen == 0)
  {
    return 0;
  }
  else if(cmdLen > 0)
  {
    for(i=1; i<at_cmdNum; i++)
    {
//      os_printf("%d len %d\r\n", cmdLen, at_fun[i].at_cmdLen);
      if(cmdLen == at_fun[i].at_cmdLen)
      {
//        os_printf("%s cmp %s\r\n", pCmd, at_fun[i].at_cmdName);
        if(os_memcmp(pCmd, at_fun[i].at_cmdName, cmdLen) == 0) //think add cmp len first
        {
          return i;
        }
      }
    }
  }
  return -1;
}

/**
  * @brief  Get the length of commad.
  * @param  pCmd: point to received command 
  * @retval the length of command
  *   @arg -1: failure
  */
static int8_t ICACHE_FLASH_ATTR
at_getCmdLen(uint8_t *pCmd)
{
  uint8_t n,i;

  n = 0;
  i = 128;

  while(i--)
  {
    if((*pCmd == '\r') || (*pCmd == '=') || (*pCmd == '?') || ((*pCmd >= '0')&&(*pCmd <= '9')))
    {
      return n;
    }
    else
    {
      pCmd++;
      n++;
    }
  }
  return -1;
}

/**
  * @brief  Distinguish commad and to execution.
  * @param  pAtRcvData: point to received (command) 
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_cmdProcess(uint8_t *pAtRcvData)
{
  char tempStr[32];

  int16_t cmdId;
  int8_t cmdLen;
  uint16_t i;

  cmdLen = at_getCmdLen(pAtRcvData);
  if(cmdLen != -1)
  {
    cmdId = at_cmdSearch(cmdLen, pAtRcvData);
  }
  else 
  {
  	cmdId = -1;
  }
  if(cmdId != -1)
  {
//    os_printf("cmd id: %d\r\n", cmdId);
    pAtRcvData += cmdLen;
    if(*pAtRcvData == '\r')
    {
      if(at_fun[cmdId].at_exeCmd)
      {
        at_fun[cmdId].at_exeCmd(cmdId);
      }
      else
      {
        uart0_sendStr("could not execute this function\r\n");
      }
    }
    else if(*pAtRcvData == '?' && (pAtRcvData[1] == '\r'))
    {
      if(at_fun[cmdId].at_queryCmd)
      {
        at_fun[cmdId].at_queryCmd(cmdId);
      }
      else
      {
        uart0_sendStr("could not query this function\r\n");
      }
    }
    else if((*pAtRcvData == '=') && (pAtRcvData[1] == '?') && (pAtRcvData[2] == '\r'))
    {
      if(at_fun[cmdId].at_testCmd)
      {
        at_fun[cmdId].at_testCmd(cmdId);
      }
      else
      {
        uart0_sendStr("could not get data from this function\r\n");
      }
    }
    else if((*pAtRcvData >= '0') && (*pAtRcvData <= '9') || (*pAtRcvData == '='))
    {
      if(at_fun[cmdId].at_setupCmd)
      {
        at_fun[cmdId].at_setupCmd(cmdId, pAtRcvData);
      }
      else
      {
        uart0_sendStr("could not setup this function\r\n");
      }
    }
    else
    {
      uart0_sendStr("enter error\r\n");
    }
  }
  else 
  {
  	uart0_sendStr("the command you entered is unknown, AT+HELP for a list of commands\r\n");
	at_backError;
  }
}

/**
  * @}
  */
