#ifndef LOG_H
#define LOG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <vector.h>
#include <stdio.h>
#include <list.h>

#include "pthread_rdwr.h"
#include "constants.h"

// Info: basic information about what's going on
const unsigned short L_INFO     = 0x0001;
// Unknown: unknown packets or bytes
const unsigned short L_UNKNOWN  = 0x0002;
// Error: critical errors which should be brought to the attention of the user
const unsigned short L_ERROR    = 0x0004;
// Warn: warnings which are not critical but could be important
const unsigned short L_WARN     = 0x0008;
// Packet: packet dumps
const unsigned short L_PACKET   = 0x0010;

const unsigned short L_ALL      = L_INFO | L_UNKNOWN | L_ERROR | L_WARN | L_PACKET;
const unsigned short L_NONE     = 0;

const char L_WARNxSTR[]    = "[WRN] ";
const char L_UDPxSTR[]     = "[UDP] ";
const char L_TCPxSTR[]     = "[TCP] ";
const char L_ERRORxSTR[]   = "[ERR] ";
const char L_UNKNOWNxSTR[] = "[???] ";
const char L_BLANKxSTR[]   = "                ";
const char L_SBLANKxSTR[]  = "      ";
const char L_PACKETxSTR[]  = "[PKT] ";
const char L_INITxSTR[]    = "[INI] ";
const char L_ENDxSTR[]     = "[END] ";
const char L_FIFOxSTR[]    = "[FIF] ";

const unsigned short S_STDOUT   = 1;
const unsigned short S_FILE     = 2;
const unsigned short S_PLUGIN   = 4;

const unsigned short LOG_PREFIX_OFFSET = 10;

const unsigned short MAX_MSG_SIZE = 2048;


//-----CLogService---------------------------------------------------------------
class CLogService
{
public:
  CLogService(unsigned short _nLogTypes);
  virtual void lprintf(unsigned short _nLogType, const char *_szPrefix,
                       const char *_szFormat, va_list argp) = 0;
  void SetLogTypes(unsigned short _nLogTypes);
  unsigned short ServiceType(void);
  unsigned short LogType(unsigned short _nLogType);
  void AddLogType(unsigned short _nLogType);
  void RemoveLogType(unsigned short _nLogType);
  void SetData(void *);

protected:
  unsigned short m_nLogTypes;
  unsigned short m_nServiceType;
  void *m_pData;
};


//-----StdOut-------------------------------------------------------------------
class CLogService_StdOut : public CLogService
{
public:
  CLogService_StdOut(unsigned short _nLogTypes, bool _bUseColor);
  virtual void lprintf(unsigned short _nLogType, const char *_szPrefix,
                       const char *_szFormat, va_list argp);
protected:
  bool m_bUseColor;
};


//-----File---------------------------------------------------------------------
class CLogService_File : public CLogService
{
public:
   CLogService_File(unsigned short _nLogTypes);
   bool SetLogFile(const char *_szFile, const char *_szFlags);
   virtual void lprintf(unsigned short _nLogType, const char *_szPrefix,
                        const char *_szFormat, va_list argp);
protected:
   FILE *m_fLog;
};


//-----Plugin------------------------------------------------------------------
class CPluginLog
{
public:
  CPluginLog(void);
  char *NextLogMsg(void);
  unsigned short NextLogType(void);
  void ClearLog(void);
  void AddLog(char *_szLog, unsigned short _nLogType);
  int Pipe(void)  { return pipe_log[PIPE_READ]; }
protected:
  pthread_mutex_t mutex;
  list <char *> m_vszLogs;
  list <unsigned short> m_vnLogTypes;
  int pipe_log[2];
};

class CLogService_Plugin : public CLogService
{
public:
  CLogService_Plugin(CPluginLog *_xWindow, unsigned short _nLogTypes);
  bool SetLogWindow(CPluginLog *_xWindow);
  virtual void lprintf(unsigned short _nLogType, const char *_szPrefix,
                       const char *_szFormat, va_list argp);
protected:
  CPluginLog *m_xLogWindow;
};


//-----CLogServer---------------------------------------------------------------
class CLogServer
{
public:
  CLogServer(void);
  void AddService(CLogService *_xService);
  void AddLogTypeToService(unsigned short _nServiceType, unsigned short _nLogType);
  void RemoveLogTypeFromService(unsigned short _nServiceType, unsigned short _nLogType);
  void ModifyService(unsigned short _nServiceType, unsigned short _nLogTypes);
  unsigned short ServiceLogTypes(unsigned short _nServiceType);
  void SetServiceData(unsigned short _nServiceType, void *_pData);

  void Info(const char *_szFormat, ...);
  void Info(unsigned short _nServiceTypes, const char *_szFormat, ...);
  void Unknown(const char *_szFormat, ...);
  void Unknown(unsigned short _nServiceTypes, const char *_szFormat, ...);
  void Error(const char *_szFormat, ...);
  void Error(unsigned short _nServiceTypes, const char *_szFormat, ...);
  void Warn(const char *_szFormat, ...);
  void Warn(unsigned short _nServiceTypes, const char *_szFormat, ...);
  void Packet(const char *_szFormat, ...);
  void Packet(unsigned short _nServiceTypes, const char *_szFormat, ...);

protected:
  vector <CLogService *> m_vxLogServices;
  pthread_mutex_t mutex;
  void Log(const unsigned short _nLogType, const char *_szFormat, va_list argp);
  void Log(const unsigned short _nServiceTypes, const unsigned short _nLogType, const char *_szFormat, va_list argp);
};


// Define an external log server to be started in log.cpp
extern CLogServer gLog;


#endif
