/*
Function: This Module can be use to create JSON and parse JSON files
Author: William Chen
Date: 2022/10/12
*/
#include "JSON.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "hal.h"


#define JSON_MemMalloc malloc
#define JSON_MemFree free

JsonObj_S *JSON_CreateObj(JsonVarTp_E tp, int sz, void *data, char * name)
{
    JsonObj_S *pObj = JSON_MemMalloc(sizeof(*pObj) + sz);
    memset(pObj, 0, sizeof(*pObj) + sz);

    pObj->type = tp;
    if(name)
    {
        pObj->name = JSON_MemMalloc(strlen(name) + 1);
        memset(pObj->name, 0, strlen(name) + 1);
        sprintf(pObj->name, "%s", name);
    }
    
    switch(tp)
    {
        case JSON_VAR_TP_BOOL:
        case JSON_VAR_TP_FLOAT32:
        case JSON_VAR_TP_INT32:
        case JSON_VAR_TP_STRING:
        {
            memcpy(pObj->data, data, sz);
        }break;
        
        default : break;
    }
    return pObj;
}


Json_S *JSON_Creat(void)
{
    Json_S *pJson = JSON_MemMalloc(sizeof(*pJson));
    memset(pJson, 0, sizeof(*pJson));

    if(pJson)
    {
        pJson->pRoot = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, NULL);
        if(!pJson->pRoot)
        {
            JSON_MemFree(pJson);
            pJson = NULL;
        }
    }

    return pJson;
}



static void DestoryObj(JsonObj_S *pObj)
{
    if(!pObj)
    {
        return ;
    }
    DestoryObj(pObj->pChild); // destory child obj
    DestoryObj(pObj->pNext);
    if(pObj->name)
    {
        JSON_MemFree(pObj->name);
        pObj->name = NULL;
    }
    JSON_MemFree(pObj); //destory
}

bool JSON_Destory(Json_S *pJson)
{
    if(pJson)
    {
        DestoryObj(pJson->pRoot);
        if(pJson->prtBuf)
        {
            JSON_MemFree(pJson->prtBuf);
        }
        JSON_MemFree(pJson);
        return true;
    }
    return false;
}

void JSON_AddObj(JsonObj_S *dst, JsonObj_S *src)
{
    if(dst && src)
    {
        if(dst->pChild)
        {
            JsonObj_S *pTail = dst->pChild;
            while (pTail->pNext)
            {
                pTail = pTail->pNext;
            }

            pTail->pNext = src;
        }
        else
        {
            dst->pChild = src;
        }
    }
}


JsonObj_S *JSON_Seek(JsonObj_S *obj, char * objName)
{
    JsonObj_S *p = NULL;
    if(!obj)
    {
        return NULL;
    }
    
    for(p = obj->pChild; p; p = p->pNext)
    {
        if(p->name)
        {
            if(0 == strcmp(objName, p->name))
            {
                return p;
            }
        }
    }
    return NULL;
}

static int PrintString(char **pBuf, int *pSz, char * str)
{
    if(pBuf)
    {
        int len = snprintf(*pBuf, *pSz, "%s", str);
        if(0 < len)
        {
            *pBuf += len;
            *pSz -= len;;
        }
    }
    else
    {
        return strlen(str);
    }
}


static int PrintLineHead(char **buf, int *bufSz, int childObjDepth, bool isFormat)
{
    int len = 0;
    for(int i =0; isFormat && i < childObjDepth; i++)
    {
        len += PrintString(buf, bufSz, TAB_CHAR);
    }
    return len;
}

static int PrintLineTail(char **buf, int *bufSz, bool isFormat)
{
    int len = 0;
    if(isFormat)
    {
        len += PrintString(buf, bufSz, ENTER_CHAR);
    }
    return len;
}

static int ObjHead2Txt(JsonObj_S *pObj, char **buf, int *bufSz, int childObjDepth, bool isFormat)
{
    int len = 0;
    if(pObj)
    {
        if(pObj->name)
        {
            len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
            len += PrintString(buf, bufSz, "\"");
            len += PrintString(buf, bufSz, pObj->name);
            len += PrintString(buf, bufSz, "\":");
        }
        else if(JSON_VAR_TP_INT32 <= pObj->type && JSON_VAR_TP_STRING >= pObj->type)
        {
            if(!pObj->name)
            {
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
            }
        }

        switch(pObj->type)
        {
            case JSON_VAR_TP_OBJ:
            {
                if(pObj->name)
                {
                    len += PrintLineTail(buf, bufSz, isFormat);
                }
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "{");
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            case JSON_VAR_TP_ARR:
            {
                if(pObj->name)
                {
                    len += PrintLineTail(buf, bufSz, isFormat);
                }
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "[");
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            case JSON_VAR_TP_BOOL:
            {
                bool val = false;
                memcpy(&val, pObj->data, sizeof val);
                len += PrintString(buf, bufSz, val? "true" : "false");
            }break;

            case JSON_VAR_TP_FLOAT32:
            {
                float val = 0;
                memcpy(&val, pObj->data, sizeof val);
                char valStr[32] = {0};
                snprintf(valStr, sizeof(valStr), "%f", val);
                len += PrintString(buf, bufSz, valStr);
            }break;

            case JSON_VAR_TP_INT32:
            {
                int val = 0;
                memcpy(&val, pObj->data, sizeof val);
                char valStr[32] = {0};
                snprintf(valStr, sizeof(valStr), "%d", val);
                len += PrintString(buf, bufSz, valStr);
            }break;

            case JSON_VAR_TP_STRING:
            {
                char * val = (char *)pObj->data;
                len += PrintString(buf, bufSz, "\"");
                len += PrintString(buf, bufSz, val);
                len += PrintString(buf, bufSz, "\"");
            }break;

            default : break;
        }

        if(JSON_VAR_TP_INT32 <= pObj->type && JSON_VAR_TP_STRING >= pObj->type)
        {
            if(pObj->pNext)
            {
                len += PrintString(buf, bufSz, ",");
            }
            len += PrintLineTail(buf, bufSz, isFormat);
        }
    }
    return len;
}

static int ObjTail2Txt(JsonObj_S *pObj, char **buf, int *bufSz, int childObjDepth, bool isFormat)
{
    int len = 0;
    if(pObj)
    {
        switch(pObj->type)
        {
            case JSON_VAR_TP_OBJ:
            {
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "}");
                if(pObj->pNext)
                {
                    len += PrintString(buf, bufSz, ",");
                }
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            case JSON_VAR_TP_ARR:
            {
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "]");
                if(pObj->pNext)
                {
                    len += PrintString(buf, bufSz, ",");
                }
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            default : break;
        }
    }
    return len;
}


static int PrintObj(JsonObj_S *pObj, char **buf, int *bufSz, int childObjDepth, bool isFormat)
{
    int len = 0;
    if(pObj)
    {
        len += ObjHead2Txt(pObj, buf, bufSz, childObjDepth, isFormat);
        len += PrintObj(pObj->pChild, buf, bufSz, childObjDepth + 1, isFormat);
        len += ObjTail2Txt(pObj, buf, bufSz, childObjDepth, isFormat);

        len += PrintObj(pObj->pNext, buf, bufSz, childObjDepth, isFormat);
    }
    return len;
}


static int GetJsonPrintfBufSz(Json_S *pJson, bool isFormat)
{
    int sz = 0, childObjDepth = 0;
    sz = PrintObj(pJson->pRoot, NULL, NULL, childObjDepth, isFormat);
    return sz;
}

int JSON_Printf(Json_S *pJson, bool isEnFormat)
{
    int childObjDepth = 0, sz = 0;
    char *buf = NULL;

    do
    {
        if(!pJson)
        {
            break;
        }

        if(pJson->prtBuf)
        {
            JSON_MemFree(pJson->prtBuf);
        }
        
        sz = GetJsonPrintfBufSz(pJson, isEnFormat) + 1;
        if(!sz)
        {
            break;
        }

        buf = JSON_MemMalloc(sz);
        if(!buf)
        {
            break;
        }
        memset(buf, 0, sz);
        pJson->prtBuf = buf;
        pJson->prtBufSz = sz;
        pJson->isPrtStrFormat = isEnFormat;

        PrintObj(pJson->pRoot, &buf, &sz, childObjDepth, isEnFormat);
    }
    while (0);
    
    return sz;
}


static int CreatFile(char * fileName, char *dat, int len)
{
    int wLen = 0, cnt = 0;
    if(!fileName && !dat && !len)
    {
        return -1;
    }
/*
    FILE *fP = fopen(fileName, "w+");

    if(fP)
    {
        do
        {
            wLen += fwrite(&dat[wLen], 1, len - wLen, fP);
            cnt++;
        }
        while ((wLen != len) && (cnt < 3));

        fclose(fP);
        if(wLen == len)
        {
            return 0;
        }
    }
*/
    return -1;
}

int JSON_Print2File(Json_S *pJson, char * fileName)
{
    int ret = -1;
    JSON_Printf(pJson, true);
    ret = CreatFile(fileName, pJson->prtBuf, pJson->prtBufSz);

    return ret;
}

static char GetChFromString(char * *pStr)
{
    char ch = '\0';
    if(*pStr && **pStr)
    {
        ch = **pStr;
        *pStr += 1;
    }
    return ch;
}

static int GetStringLen(char * *pTxt)
{
    int strLen = 0;
    char ch = 0;

    ch = GetChFromString(pTxt);

    while(ch)
    {
        switch(ch)
        {
            case '\"':
            {
                return strLen;
            }break;

            case '[':
            case ']':
            case '{':
            case '}':
            case ',':
            {
                return -1;
            }break;
        }
        strLen++;
        ch = GetChFromString(pTxt);
    }
    
    return -1;
}

static char * StringParse(char * *pTxt)
{
    int strLen = 0;
    char * strBuf = NULL;
    char * str = *pTxt;
    strLen = GetStringLen(pTxt);
    if(strLen > 0)
    {
        strBuf = JSON_MemMalloc(strLen + 1);
        memset(strBuf, 0, strLen + 1);
        if(strBuf)
        {
            memcpy(strBuf, str, strLen);
        }
    }
    return strBuf;
}


static int GetObjStrLen(char * *pTxt)
{
    char ch = 0;
    int len = 0;

    ch = GetChFromString(pTxt);
    while(ch)
    {
        len++;
        switch(ch)
        {
            case '{':
            {
                len += GetObjStrLen(pTxt);
            }break;
            
            case '}':
            {
                return len;
            }break;

            default : break;
        }
        ch = GetChFromString(pTxt);
    }
    return len;;
}

static int GetArrStrLen(char * *pTxt)
{
    char ch = 0;
    int len = 0;

    ch = GetChFromString(pTxt);
    while(ch)
    {
        len++;
        switch(ch)
        {
            case '[':
            {
                len += GetArrStrLen(pTxt);
            }break;

            case ']':
            {
                return len;
            }break;

            default : break;
        }
        ch = GetChFromString(pTxt);
    }
    return len;;
}

static int GetItemStrLen(char * *pTxt)
{
    char ch = 0;
    int len = 0;

    ch = GetChFromString(pTxt);
    while(ch)
    {
        len++;
        switch(ch)
        {
            case '{':
            {
                len += GetObjStrLen(pTxt);
                return len;
            }break;
            case '[':
            {
                len += GetArrStrLen(pTxt);
                return len;
            }break;

            case ',':
            {
                return len;
            }break;

            default : break;
        }
        ch = GetChFromString(pTxt);
    }

    return len;
}


static char * GetItemStr(char * *pTxt)
{
    char * str = NULL;
    char ch = 0, i = 0;
    char * txt = *pTxt;
    int len = GetItemStrLen(pTxt);

    if(len > 0)
    {
        str = JSON_MemMalloc(len + 1);
        memset(str, 0, len + 1);
        if(str)
        {
            memcpy(str, txt, len);
        }
    }
    return str;
}


static void StringDestory(char * *str)
{
    if(*str)
    {
        JSON_MemFree(*str);
        *str = NULL;
    }
}

static void SkipWhiteSpace(char * *str)
{
    char ch = 0;

    do
    {
        ch = GetChFromString(str);
    }
    while (ch && ch <= 32);
}


static long long IntParse(char * str)
{
    return atol(str);
}


static double FloatParse(char * str)
{
    return atof(str);
}

static void ObjParse(char * *pTxt, JsonObj_S *pObj);

static void ArrParse(char * *pTxt, JsonObj_S *pObj)
{
    char * itemStr = NULL;

    itemStr = GetItemStr(pTxt);

    while(itemStr)
    {
        char * str = itemStr;
        SkipWhiteSpace(&str);
        str--;

        if('\"' == str[0])
        {
            str++;
            char * str1 = StringParse(&str);
            JsonObj_S *pStrObj = JSON_CreateObj(JSON_VAR_TP_STRING, strlen(str1) + 1, str1, NULL);
            JSON_AddObj(pObj, pStrObj);

            StringDestory(&str1);
        }
        else if('{' == str[0])
        {
            str++;
            JsonObj_S *pObjItem = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, NULL);
            ObjParse(&str, pObjItem);
            JSON_AddObj(pObj, pObjItem);
        }
        else if('[' == str[0])
        {
            str++;
            JsonObj_S *pObjArr = JSON_CreateObj(JSON_VAR_TP_ARR, 0, NULL, NULL);
            ArrParse(&str, pObjArr);
            JSON_AddObj(pObj, pObjArr);
        }
        else if('-' == str[0] || ('0' <= str[0] && '9' >= str[0]))
        {
            bool isFloatNb = false;
            for(int i = 0; str[i]; i++)
            {
                if('.' == str[i])
                {
                    isFloatNb = true;
                    break;
                }
            }
            if(isFloatNb)
            {
                float fNb = FloatParse(str);
                JsonObj_S *pfNbObj = JSON_CreateObj(JSON_VAR_TP_FLOAT32, sizeof(fNb), &fNb, NULL);
                JSON_AddObj(pObj, pfNbObj);
            }
            else
            {
                int iNb = IntParse(str);
                JsonObj_S *pNbObj = JSON_CreateObj(JSON_VAR_TP_INT32, sizeof(iNb), &iNb, NULL);
                JSON_AddObj(pObj, pNbObj);
            }
        }

        StringDestory(&itemStr);
        
        itemStr = GetItemStr(pTxt);
    }
}

static void ObjParse(char * *pTxt, JsonObj_S *pObj)
{
    char * objName = NULL;
    char ch = 0;
    do
    {
        ch = GetChFromString(pTxt);
        switch(ch)
        {
            case '\"':
            {
                objName = StringParse(pTxt);
            }break;

            case ':':
            {
                char * itemStr = GetItemStr(pTxt);
                char * str = itemStr;
                SkipWhiteSpace(&str);
                str--;
                
                if(0 == strncmp(str, "false", 5))
                {
                    bool val = false;
                    JsonObj_S *pValObj = JSON_CreateObj(JSON_VAR_TP_BOOL, sizeof(val), &val, objName);
                    JSON_AddObj(pObj, pValObj);
                }
                else if(0 == strncmp(str, "true", 4))
                {
                    bool val = true;
                    JsonObj_S *pValObj = JSON_CreateObj(JSON_VAR_TP_BOOL, sizeof(val), &val, objName);
                    JSON_AddObj(pObj, pValObj);
                }
                else if('\"' == str[0])
                {
                    str++;
                    char * str1 = StringParse(&str);
                    if(str1)
                    {
                        JsonObj_S *pStrObj = JSON_CreateObj(JSON_VAR_TP_STRING, strlen(str1) + 1, str1, objName);
                        JSON_AddObj(pObj, pStrObj);
                        StringDestory(&str1);
                    }
                }
                else if('{' == str[0])
                {
                    str++;
                    JsonObj_S *pObjItem = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, objName);
                    ObjParse(&str, pObjItem);
                    JSON_AddObj(pObj, pObjItem);
                }
                else if('[' == str[0])
                {
                    str++;
                    JsonObj_S *pObjArr = JSON_CreateObj(JSON_VAR_TP_ARR, 0, NULL, objName);
                    ArrParse(&str, pObjArr);
                    JSON_AddObj(pObj, pObjArr);
                }
                else if('-' == str[0] || ('0' <= str[0] && '9' >= str[0]))
                {
                    bool isFloatNb = false;
                    for(int i = 0; str[i]; i++)
                    {
                        if('.' == str[i])
                        {
                            isFloatNb = true;
                            break;
                        }
                    }
                    if(isFloatNb)
                    {
                        float fNb = FloatParse(str);
                        JsonObj_S *pfNbObj = JSON_CreateObj(JSON_VAR_TP_FLOAT32, sizeof(fNb), &fNb, objName);
                        JSON_AddObj(pObj, pfNbObj);
                    }
                    else
                    {
                        int iNb = IntParse(str);
                        JsonObj_S *pNbObj = JSON_CreateObj(JSON_VAR_TP_INT32, sizeof(iNb), &iNb, objName);
                        JSON_AddObj(pObj, pNbObj);
                    }
                }
                
                StringDestory(&objName);
                StringDestory(&itemStr);
            }break;
        }
    }
    while(ch);
}

static char BracketCheck(char * *pTxt, bool *result)
{
    char ch = 0, ch2 = 0;
    do
    {
        ch = GetChFromString(pTxt);
        switch(ch)
        {
            case '{':
            {
                ch2 = BracketCheck(pTxt, result);
                if('}' != ch2)
                {
                    *result = false;
                }
            }break;
            case '[':
            {
                ch2 = BracketCheck(pTxt, result);
                if(']' != ch2)
                {
                    *result = false;
                }
            }break;

            case '}':
            case ']':
            {
                return ch;
            }break;

            case '\"':
            {
                if(0 > GetStringLen(pTxt))
                {
                    *result = false;
                }
            }break;

            default : break;
        }

    }
    while ('\0' != ch && *result);

    return ch;
}


bool IsJsonTxtLegal(char * txt)
{
    char * str = txt;
    bool result = true;
    char errBuf[256] = {0};

    BracketCheck(&str, &result);
    if(false == result)
    {
        int diff = str - txt;
        if(diff < sizeof(errBuf) / 2)
        {
            snprintf(errBuf, sizeof(errBuf) - 1, "%s", (str - diff));
        }
        else
        {
            snprintf(errBuf, sizeof(errBuf) - 1, "%s", (str - sizeof(errBuf) / 2));
            diff = sizeof(errBuf) / 2;
        }

        //printf("\033[0;32;31m""error : \n""\033[m");
        for(int i = 0; i < sizeof(errBuf); i++)
        {
            if(diff == i + 1)
            {
                //printf("\033[0;32;31m""%c""\033[m", errBuf[i]);
            }
            else
            {
                //printf("%c", errBuf[i]);
            }
        }
        
        printf("\n");
    }
    return result;
}


Json_S *JSON_ParseStr(char * txt)
{
    Json_S *pJson = NULL;
    char ch = 0;
    do
    {
        if(!txt)
        {
            break;
        }

        if(false == IsJsonTxtLegal(txt))
        {
            //printf("Json txt is illeagal\r\n");
            break;
        }

        pJson = JSON_Creat();
        if(!pJson)
        {
            break;
        }

        do
        {
            ch = GetChFromString(&txt);
            if('{' == ch)
            {
                ObjParse(&txt, pJson->pRoot);
                break;
            }
        }
        while (ch);
    }while(0);
    return pJson;
}


Json_S *JSON_ParseFile(char * file)
{
    Json_S *pJson = NULL;
/*
    FILE *fpParse = fopen(file, "r+");

    if(fpParse)
    {
        char * str = NULL;
        
        fseek(fpParse, 0, SEEK_END);
        int fileSz = ftell(fpParse);
        fseek(fpParse, 0, SEEK_SET);

        str = JSON_MemMalloc(fileSz);

        int has_been_read = 0;

        for(has_been_read = 0; has_been_read < fileSz;)
        {
            int read_len = fread(&str[has_been_read], 1, fileSz - has_been_read,fpParse);

            if(read_len < 0)
            {
                break;
            }
            else if (read_len == 0)
            {
                break;
            }

            has_been_read +=read_len;
        }

        pJson = JSON_ParseStr(str);
        JSON_MemFree(str);
        fclose(fpParse);
    }
*/
    return pJson;
}

int JSON_GetArrSz(JsonObj_S *pArr)
{
    int size = 0;

    do
    {
        if(!pArr)
        {
            break;
        }

        JsonObj_S *pArrItem = pArr->pChild;

        while(pArrItem)
        {
            size++;
            pArrItem = pArrItem->pNext;
        }
    }
    while (0);
    return size;
}

JsonObj_S *JSON_GetArrItem(JsonObj_S *pArr, int idx)
{
    JsonObj_S *pArrItem = NULL;

    do
    {
        if(!pArr)
        {
            break;
        }

        pArrItem = pArr->pChild;
        while(pArrItem && idx)
        {
            idx--;
            pArrItem = pArrItem->pNext;
        }
    }
    while (0);

    return pArrItem;
}

bool JSON_GetObjValue(JsonObj_S *pObj, void *pBuf)
{
    bool ret = true;
    do
    {
        if(!pObj || !pBuf)
        {
            ret = false;
            break;
        }

        switch(pObj->type)
        {
            case JSON_VAR_TP_BOOL :
            {
                memcpy(pBuf, pObj->data, sizeof(bool));
            }break;

            case JSON_VAR_TP_FLOAT32 :
            {
                memcpy(pBuf, pObj->data, sizeof(float));
            }break;

            case JSON_VAR_TP_INT32 :
            {
                memcpy(pBuf, pObj->data, sizeof(int));
            }break;

            case JSON_VAR_TP_STRING :
            {
                sprintf(pBuf, "%s", pObj->data);
            }break;

            default :
            {
                ret = false; 
            }break;
        }
    }while(0);
    return ret;
}


#if 0

static JsonObj_S *CreatAPerson(char * name, int age, int height)
{
    JsonObj_S *pPerson = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, NULL);

    JsonObj_S *pObjName = JSON_CreateObj(JSON_VAR_TP_STRING, strlen(name) + 1, name, "Name");
    JsonObj_S *pObjAge = JSON_CreateObj(JSON_VAR_TP_INT32, sizeof(age), &age, "Age");
    JsonObj_S *pObjHeigh = JSON_CreateObj(JSON_VAR_TP_INT32, sizeof(height), &height, "Height");

    JSON_AddObj(pPerson, pObjHeigh);
    JSON_AddObj(pPerson, pObjName);
    JSON_AddObj(pPerson, pObjAge);
    return pPerson;
}

int main(int argc, char *argv[])
{

    Json_S *pJson = JSON_Creat();

    JsonObj_S *pObjCoporation = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, "Coporation");
    JsonObj_S *pObjCopname = JSON_CreateObj(JSON_VAR_TP_STRING, strlen("AAA") + 1, "AAA", "Name");

    int employeeCnt = 1000;
    JsonObj_S *pObjEmployeeCnt = JSON_CreateObj(JSON_VAR_TP_INT32, sizeof(employeeCnt), &employeeCnt, "employeeCnt");

    JsonObj_S *pDepartment = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, "Department");
    JsonObj_S *pDepartmentArm = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, "DepartmentArm");
    JsonObj_S *pDepartmentProduct = JSON_CreateObj(JSON_VAR_TP_OBJ, 0, NULL, "DepartmentProduct");

    JsonObj_S *pIntArr = JSON_CreateObj(JSON_VAR_TP_ARR, 0, NULL, "IntArr");

    for(int n = 0; n < 10; n++)
    {
        JsonObj_S *p = JSON_CreateObj(JSON_VAR_TP_INT32, sizeof(n), &n, NULL);
        
        JSON_AddObj(pIntArr, p);
    }


    JsonObj_S *pEmpolyeeArm = JSON_CreateObj(JSON_VAR_TP_ARR, 0, NULL, "empolyee");
    JSON_AddObj(pEmpolyeeArm, CreatAPerson("william", 17, 180));



    JSON_AddObj(pDepartmentArm, pEmpolyeeArm);
    
    JSON_AddObj(pDepartment, pDepartmentProduct);
    JSON_AddObj(pDepartment, pDepartmentArm);

    JSON_AddObj(pObjCoporation, pObjCopname);
    JSON_AddObj(pObjCoporation, pObjEmployeeCnt);
    JSON_AddObj(pObjCoporation, pDepartment);
    JSON_AddObj(pObjCoporation, pIntArr);

    JSON_AddObj(pJson->pRoot, pObjCoporation);

    JSON_Printf(pJson, true);

    printf("%s", pJson->prtBuf);
    printf("------------------------------------\n");

    Json_S *pJsonParse = JSON_ParseStr(pJson->prtBuf);
    JSON_Destory(pJson);
    if(pJsonParse)
    {
        JsonObj_S *pCoporationObj = JSON_Seek(pJsonParse->pRoot, "Coporation");
        JsonObj_S *pNameObj = JSON_Seek(pCoporationObj, "Name");
        JsonObj_S *pEEmployeeCntObj = JSON_Seek(pCoporationObj, "employeeCnt");

        JsonObj_S *pDepartmentObj = JSON_Seek(pCoporationObj, "Department");
        JsonObj_S *pDepartmentPrdObj = JSON_Seek(pDepartmentObj, "DepartmentProduct");
        JsonObj_S *pEmpolyeeArrObj = JSON_Seek(pDepartmentPrdObj, "empolyee");

        int arrSz = JSON_GetArrSz(pEmpolyeeArrObj);
        printf("arr sz = %d\r\n", arrSz);
        for(int idx = 0; idx < arrSz; idx++)
        {
            JsonObj_S *pArrItem = JSON_GetArrItem(pEmpolyeeArrObj, idx);
            JsonObj_S *pArrHeightObj = JSON_Seek(pArrItem, "Height");
            JsonObj_S *pArrNameObj = JSON_Seek(pArrItem, "Name");
            JsonObj_S *pArrAgeObj = JSON_Seek(pArrItem, "Age");
            printf("Height = %d, name = %s, age = %d, idx = %d\r\n", *(long long *)pArrHeightObj->data, pArrNameObj->data, *(long long *)pArrAgeObj->data, idx);
        }

        printf("name = %s, %d\r\n", pNameObj->data, *(long long*)pEEmployeeCntObj->data);

        JSON_Printf(pJsonParse, true);
        printf("%s", pJsonParse->prtBuf);
        JSON_Destory(pJsonParse);
    }
    else
    {
        printf("Json parse failed\r\n");
    }
    
    printf("malloc cnt = %d, free cnt = %d\r\n", mallocCnt, freeCnt);
}

#endif


