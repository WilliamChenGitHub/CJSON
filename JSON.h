#ifdef __cplusplus
extern "C"{
#endif

#pragma once


#define TAB_CHAR "\t"
//#define TAB_CHAR "    "

//#define ENTER_CHAR "\n"
#define ENTER_CHAR "\r\n"
#include "stdbool.h"


typedef enum 
{
    JSON_VAR_TP_INT32,
    JSON_VAR_TP_BOOL,
    JSON_VAR_TP_FLOAT32,
    JSON_VAR_TP_STRING,
    JSON_VAR_TP_ARR,
    JSON_VAR_TP_OBJ,
    JSON_VAR_TP_BUTT,
}JsonVarTp_E;

#pragma pack(push)
#pragma pack(1)

typedef struct __JSON_OBJ
{
    struct __JSON_OBJ *pNext, *pChild;
    char * name;
    unsigned char type;
    char data[0];
}JsonObj_S;

typedef struct __JSON
{
    JsonObj_S *pRoot;
    char * prtBuf;
    int prtBufSz;
    bool isPrtStrFormat;
}Json_S;
#pragma pack(pop)




JsonObj_S *JSON_CreateObj(JsonVarTp_E tp, int sz, void *data, char * name);
Json_S *JSON_Creat(void);
bool JSON_Destory(Json_S *pJson);
void JSON_AddObj(JsonObj_S *dst, JsonObj_S *src);
bool JSON_DeletObj(JsonObj_S *obj);
JsonObj_S *JSON_Seek(JsonObj_S *obj, char * objName);
int JSON_Printf(Json_S *pJson, bool isEnFormat);
Json_S *JSON_ParseStr(char * txt);
Json_S *JSON_ParseFile(char * file);
int JSON_Print2File(Json_S *pJson, char * fileName);
int JSON_GetArrSz(JsonObj_S *pArr);
JsonObj_S *JSON_GetArrItem(JsonObj_S *pArr, int idx);
bool JSON_GetObjValue(JsonObj_S *pObj, void *pBuf);

#ifdef __cplusplus
}
#endif


