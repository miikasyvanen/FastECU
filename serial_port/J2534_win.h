//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2004- Tactrix Inc.
//
// You are free to use this file for any purpose, but please keep
// notice of where it came from!
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
#include <windows.h>
#endif

#include "J2534_tactrix_win.h"

#define PTfn(name) PF_##name* pf##name
#define PText(name) PT_API PF_##name name

#define DBGPRINT(args_in_parens)                                \
{                                               \
        if (debugMode)	\
        dbgprint args_in_parens; \
}

#define DBGDUMP(args_in_parens)                                \
{                                               \
        if (debugMode)	\
        dbgdump args_in_parens; \
}

#define DBGPRINTPT(args_in_parens)                                \
{                                               \
        if (debugMode)	\
        dbgprintptmsg args_in_parens; \
}

class J2534
{
public:
    J2534();
    ~J2534();

    bool serial_port_protocol_iso14230 = false;
    bool J2534_init_ok = false;

    bool is_serial_port_open();
    bool init() { return checkDLL(); };
    void disable();
    void setDllName(const char* name);
    void getDllName(char* name);
    bool valid();
    void debug(bool enable) { debugMode = enable; };
    char* getLastError();

    long PassThruOpen(const void *pName, unsigned long *pDeviceID);
    long PassThruClose(unsigned long DeviceID);
    long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID);
    long PassThruDisconnect(unsigned long ChannelID);
    long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
    long PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
    long PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval);
    long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID);
    long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID);
    long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID);
    long PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage);
    long PassThruReadVersion(char *pApiVersion,char *pDllVersion,char *pFirmwareVersion,unsigned long DeviceID);
    long PassThruGetLastError(char *pErrorDescription);
    long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput);


private:
    bool getPTfns();
    long LoadJ2534DLL(const char* szDLL);
    bool checkDLL();
    void dbgprint(const char* Format, ...);
    void dbgdump(const unsigned char *data,unsigned int datalen,int kind);
    void dbgprintptmsg(const PASSTHRU_MSG *pMsg,int kind);
    int is_valid_sconfig_param(SCONFIG s);
    void dump_sbyte_array(const SBYTE_ARRAY* s);
    void dump_sconfig_param(SCONFIG s);

    char lastError[256];
    char dllName[256];
    bool debugMode;
    bool isLibraryInitialized;


#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
    HINSTANCE hDLL;               // Handle to DLL
#else
    void* hDLL;
#endif

    /* J2534 Interface API function pointers */

    PTfn(PassThruOpen);
    PTfn(PassThruClose);
    PTfn(PassThruConnect);
    PTfn(PassThruDisconnect);
    PTfn(PassThruReadMsgs);
    PTfn(PassThruWriteMsgs);
    PTfn(PassThruStartPeriodicMsg);
    PTfn(PassThruStopPeriodicMsg);
    PTfn(PassThruStartMsgFilter);
    PTfn(PassThruStopMsgFilter);
    PTfn(PassThruSetProgrammingVoltage);
    PTfn(PassThruReadVersion);
    PTfn(PassThruGetLastError);
    PTfn(PassThruIoctl);
};

#if defined(OP20PT32_USE_LIB)
//	PT_API void OP20PT32_Start();
//	PT_API void OP20PT32_Stop();
PText(PassThruOpen);
PText(PassThruClose);
PText(PassThruConnect);
PText(PassThruDisconnect);
PText(PassThruReadMsgs);
PText(PassThruWriteMsgs);
PText(PassThruStartPeriodicMsg);
PText(PassThruStopPeriodicMsg);
PText(PassThruStartMsgFilter);
PText(PassThruStopMsgFilter);
PText(PassThruSetProgrammingVoltage);
PText(PassThruReadVersion);
PText(PassThruGetLastError);
PText(PassThruIoctl);
#endif
