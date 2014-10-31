#ifndef __AT_BASECMD_H
#define __AT_BASECMD_H

#define MAX_HOST_LENGTH 	64
#define MAX_PATH_LENGTH 	128
#define TTL_INIT 			20
#define	CONNECTION_DELAY_MS 200

void at_exeCmdNull(uint8_t id);
void at_setupCmdE(uint8_t id, char *pPara);
void at_exeCmdRst(uint8_t id);
void at_exeCmdGmr(uint8_t id);
void at_setupCmdCipStartLog(uint8_t id, char *pPara);
void at_exeCmdStopLog(uint8_t id);
void at_exeCmdHelp(uint8_t id);
void connection_timer_elapsed(void *arg);
void polling_timer_elapsed(void *arg);
#endif
