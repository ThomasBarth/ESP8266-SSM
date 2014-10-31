#include "osapi.h"
#include "c_types.h"
#include "at.h"
#include "at_baseCmd.h"
#include "user_interface.h"
#include "at_version.h"
#include "version.h"
#include "gpio.h"
#include "osapi.h"
#include "os_type.h"

//uncomment to enable debug infos
//#define DEBUG

/** @defgroup AT_BASECMD_Functions
  * @{
  */

extern BOOL echoFlag;
extern at_mdStateType mdState;

static volatile os_timer_t polling_timer;
static volatile os_timer_t connection_timer;

char logger_script_path[MAX_PATH_LENGTH];
char logger_CISTART_args[32];
char logger_target_host[MAX_HOST_LENGTH];
uint8_t logger_con_id;

uint16_t GPIO_time_active=0;
uint16_t captured_time;
uint8_t  connection_TTL;

/**
  * @brief  handles the ticks of the polling timer to check if there was a edge of activity. If yes it sets up the connection and starts the get data system.
  * @param  arg: arguments passed TODO: still a bit spooky, find out how to pass args
  * @retval None
  */
void polling_timer_elapsed(void *arg)
{
   	char temp[128];
	
	//check if input is active (0), increment time
	if(!GPIO_INPUT_GET(2))	
		GPIO_time_active++;
	//the input is inactive, do we have a measurement to send?
	else
		//yep there is something to send
		if(GPIO_time_active!=0)
		{
			//Connect
			strcpy(temp, logger_CISTART_args);
			at_setupCmdCipstart(12,temp);	
			
			//set time to life (max connect tries)
			connection_TTL=TTL_INIT;
			
			//check if connection was successful with the help of a timer
			//Disarm timer
			os_timer_disarm(&connection_timer);

			//Setup timer
			os_timer_setfn(&connection_timer, (os_timer_func_t *)connection_timer_elapsed, NULL);

			//Set up the timer, (timer, milliseconds, 1=cycle 0=once)
			os_timer_arm(&connection_timer, CONNECTION_DELAY_MS, 1);	
			
			//safe data
			captured_time=GPIO_time_active;
			GPIO_time_active=0;
		} 
}

/**
  * @brief  called by the timer and checks if the connection is open, if it is open it calls the page defined at logger_script_path
  * @param  arg: arguments passed TODO: still a bit spooky, find out how to pass args
  * @retval None
  */
void connection_timer_elapsed(void *arg)
{
	char temp[128];	
	uint8_t i;
	
	connection_TTL--;

#ifdef DEBUG
	os_sprintf(temp,"checking for connections, state: %d , TTL: %d\r\n\r\n",mdState,connection_TTL);
	uart0_sendStr(temp);
#endif
	
	//some connections are linked
	if(mdState==3)
	{
#ifdef DEBUG
		os_sprintf(temp,"Checking connection#: %d \r\n\r\n",logger_con_id);
		uart0_sendStr(temp);
#endif	
		//check if our connection if open, too
		if(check_connection(logger_con_id)==1)
		{
			//reset timer
			os_timer_disarm(&connection_timer);
		
			//prepare GET command
			os_sprintf(temp,"GET %s&d=%d HTTP/1.0\r\nHost: %s \r\n\r\n",logger_script_path,captured_time,logger_target_host);			
			
#ifdef DEBUG
			uart0_sendStr("Sending data: \r\n");
			uart0_sendStr(temp);
#endif	
			//send data
			send_data_directly(logger_con_id,temp);
		}	
	}
	
	//TTL is over, not able to connect...
	if(connection_TTL==0)
	{
		os_timer_disarm(&connection_timer);
		os_sprintf(temp,"No connection after %d tries, with %d ms delay \r\n\r\n",TTL_INIT,CONNECTION_DELAY_MS);	
		uart0_sendStr(temp);
	}
}

/**
  * @brief  Setup command to start GPIO logger.
  * @param  id: command id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdCipStartLog(uint8_t id, char *pPara)
{
	char temp[128];
	int8_t len,i;
	char *pParatemp=pPara;
	char temp_protocol[3];
	

	//get station mode
	if(wifi_get_opmode()!=2)
	{
		pParatemp++;
	
		//check if MUX is 1 to set up the connection ID
		if(isIPMUX())
		{
			//check if conID seem to be ok
			if(isdigit(pParatemp[0]))
			{
				//char to int			
				logger_con_id=pParatemp[0]- '0';
				pParatemp+=2;
			}
			else
			{
				uart0_sendStr("Connection ID missing, add one or set CIPMUX to 0");
				return;
			}
		}
		else
		{
			//check if there is no conID is ok
			if(pParatemp[0]=='"')
				logger_con_id=0;
			else
			{
				uart0_sendStr("there seems to be something wrong with the parameters");

				return;
			}
		}		

		//parse protocol
		len = at_dataStrCpy(temp_protocol, pParatemp, 4);		
		if(len == -1)
		{
			uart0_sendStr("Protocol ERROR\r\n");
			return;
		}
		pParatemp+=len+3;
		
		//parse URL
		len = at_dataStrCpy(logger_script_path, pParatemp, 127);
		if(len == -1)
		{
			uart0_sendStr("URL ERROR\r\n");
			return;
		}
		pParatemp+=len+3;
		
		//separate Host and Path
		for(i=0;i<MAX_PATH_LENGTH-1;i++)
		{
			if(logger_script_path[i]!='/')
				logger_target_host[i]=logger_script_path[i];
			else
			{

				//end of host, now we go for the path, needs do be made better
				logger_target_host[i] = '\0';
				for(len=i;i<MAX_PATH_LENGTH-1;i++)
					if(logger_script_path[i]!='"')
						logger_script_path[i-len]=logger_script_path[i];
					else				
						logger_script_path[i-len]= '\0';									

				break;
			}

		}
			
		//ok now we need to get the other parameters for at_setupCmdCipstart aka AT+CIPSTART
		if(isIPMUX())
			os_sprintf(logger_CISTART_args,"=%d,\"%s\",\"%s\",%s",logger_con_id,temp_protocol,logger_target_host,pParatemp);
		else
			os_sprintf(logger_CISTART_args,"=\"%s\",\"%s\",%s",temp_protocol,logger_target_host,pParatemp);

		//TODO: get the GPIO pin number as parameter	
		
		//set gpio2 as gpio pin
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	  
		//disable pulldown
		PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO2_U);
	  
		//enable pull up R
		PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);  
	  
		//TODO: make this interrupt based

		//Disarm timer
		os_timer_disarm(&polling_timer);

		//Setup timer
		os_timer_setfn(&polling_timer, (os_timer_func_t *)polling_timer_elapsed, NULL);

		//Set up the timer, (timer, milliseconds, 1=cycle 0=once)
		os_timer_arm(&polling_timer, 1000, 1);

		uart0_sendStr("Done setting up the Logger at GPIO2\r\n");
		
#ifdef DEBUG
		uart0_sendStr("CIPStart args: \r\n");
		uart0_sendStr(logger_CISTART_args);
		
		uart0_sendStr("Path to the script \r\n");
		uart0_sendStr(logger_script_path);
		
		os_sprintf(temp,"\r\nConnection ID %d \r\n",logger_con_id);
		uart0_sendStr(temp);
#endif
		at_backOk;
	}
	else
	{
		uart0_sendStr("Cant set up logger in AP mode\r\n");
		at_backError;
	}
}


/**
  * @brief  Stop command for GPIO logger.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdStopLog(uint8_t id)
{
	os_timer_disarm(&polling_timer);	
	uart0_sendStr("Done disabling the Logger at GPIO2\r\n");
	at_backOk;
}

/**
  * @brief  Print all AT+Commands.
  * @param  id: command id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdHelp(uint8_t id)
{
	uart0_sendStr("AT+ Commands: \r\n");	
	at_print_allCmd();	
	at_backOk;
}
  
/**
  * @brief  Execution commad of AT.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdNull(uint8_t id)
{
  at_backOk;
}

/**
  * @brief  Enable or disable Echo.
  * @param  id: command id number
  * @param  pPara:
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_setupCmdE(uint8_t id, char *pPara)
{
//  os_printf("%c\n",*pPara);
  if(*pPara == '0')
  {
    echoFlag = FALSE;
  }
  else if(*pPara == '1')
  {
    echoFlag = TRUE;
  }
  else
  {
    at_backError;
    return;
  }
  at_backOk;
}

/**
  * @brief  Execution commad of restart.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdRst(uint8_t id)
{
  at_backOk;
  system_restart();
}

/**
  * @brief  Execution commad of version.
  * @param  id: commad id number
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_exeCmdGmr(uint8_t id)
{
	char temp[128];
	//http://bbs.espressif.com/viewtopic.php?f=5&t=14&sid=779d529e457e8ede73b9b7b3e664dbb3
	uart0_sendStr("DataLogger by Thomas Barth v0.1.3   barth-dev.de\r\n");
	at_backOk;
}


/**
  * @}
  */
