#ifndef __AT_IPCMD_H
#define __AT_IPCMD_H

#define at_linkMax 5

typedef enum
{
  teClient,
  teServer
}teType;

typedef struct
{
	BOOL linkEn;
	BOOL teToff;
	uint8_t linkId;
	teType teType;
	uint8_t repeaTime;
	struct espconn *pCon;
}at_linkConType;


void at_testCmdCifsr(uint8_t id);
void at_setupCmdCifsr(uint8_t id, char *pPara);
void at_exeCmdCifsr(uint8_t id);

void at_testCmdCipstatus(uint8_t id);
void at_exeCmdCipstatus(uint8_t id);

void at_testCmdCipstart(uint8_t id);
void at_setupCmdCipstart(uint8_t id, char *pPara);

void at_testCmdCipclose(uint8_t id);
void at_setupCmdCipclose(uint8_t id, char *pPara);
void at_exeCmdCipclose(uint8_t id);

void at_testCmdCipsend(uint8_t id);
void at_setupCmdCipsend(uint8_t id, char *pPara);
void at_exeCmdCipsend(uint8_t id);

void at_queryCmdCipmux(uint8_t id);
void at_setupCmdCipmux(uint8_t id, char *pPara);

void at_setupCmdCipserver(uint8_t id, char *pPara);

void at_queryCmdCipmode(uint8_t id);
void at_setupCmdCipmode(uint8_t id, char *pPara);

void at_queryCmdCipsto(uint8_t id);
void at_setupCmdCipsto(uint8_t id, char *pPara);

void at_exeCmdUpdate(uint8_t id);

void send_data_directly(uint8_t conid, char *payload);
uint8_t check_connection(uint8_t conid);
BOOL isIPMUX(void);

#endif
