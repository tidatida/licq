/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// written by Jon Keating <jon@licq.org>

#include "msn.h"
#include "msnpacket.h"
#include "licq_log.h"
#include "licq_message.h"

#include <openssl/md5.h>

#include <string>
#include <list>
#include <vector>

using namespace std;

//Global socket manager
CSocketManager gSocketMan;

void *MSNPing_tep(void *);

CMSN::CMSN(CICQDaemon *_pDaemon, int _nPipe) : m_vlPacketBucket(211)
{
  m_pDaemon = _pDaemon;
  m_bExit = false;
  m_nPipe = _nPipe;
  m_nSSLSocket = m_nServerSocket = -1;
  m_pPacketBuf = 0;
  m_szUserName = 0;
  m_szPassword = 0;
}

CMSN::~CMSN()
{
  if (m_pPacketBuf)
    delete m_pPacketBuf;
  if (m_szUserName)
    free(m_szUserName);
  if (m_szPassword)
    free(m_szPassword);
}

void CMSN::StorePacket(SBuffer *_pBuf, int _nSock)
{
  BufferList &b = m_vlPacketBucket[HashValue(_nSock)];
  b.push_front(_pBuf);
}

void CMSN::RemovePacket(string _strUser, int _nSock)
{
  BufferList &b = m_vlPacketBucket[HashValue(_nSock)];
  BufferList::iterator it;
  for (it = b.begin(); it != b.end(); it++)
  {
    if ((*it)->m_strUser == _strUser)
    {
      b.erase(it);
      break;
    }
  }
}

SBuffer *CMSN::RetrievePacket(string _strUser, int _nSock)
{
  BufferList &b = m_vlPacketBucket[HashValue(_nSock)];
  BufferList::iterator it;
  for (it = b.begin(); it != b.end(); it++)
  {
    if ((*it)->m_strUser == _strUser)
    {
      return *it;
    }
  }
  
  return 0;
}
  
ICQEvent *CMSN::RetrieveEvent(unsigned long _nTag)
{
  ICQEvent *e = 0;
  
  list<ICQEvent *>::iterator it;
  for (it = m_pEvents.begin(); it != m_pEvents.end(); it++)
  {
    if ((*it)->Sequence() == _nTag)
    {
      e = *it;
      m_pEvents.erase(it);
      break;
    }
  }
  
  return e;
}

void CMSN::Run()
{
  int nNumDesc;
  int nCurrent; 
  fd_set f;

  int nResult = pthread_create(&m_tMSNPing, NULL, &MSNPing_tep, this);
  if (nResult)
  {
    gLog.Error("%sUnable to start ping thread:\n%s%s.\n", L_ERRORxSTR, L_BLANKxSTR, strerror(nResult));
  }
  
  nResult = 0;
  
  while (!m_bExit)
  {
    f = gSocketMan.SocketSet();
    nNumDesc = gSocketMan.LargestSocket() + 1;
 
    if (m_nPipe != -1)
    {
      FD_SET(m_nPipe, &f);
      if (m_nPipe >= nNumDesc)
        nNumDesc = m_nPipe + 1;
    }

    nResult = select(nNumDesc, &f, NULL, NULL, NULL);
  
    nCurrent = 0;
    while (nResult > 0 && nCurrent < nNumDesc)
    {
      if (FD_ISSET(nCurrent, &f))
      {
        if (nCurrent == m_nPipe)
        {
          ProcessPipe();
        }
        
        else if (nCurrent == m_nServerSocket)
        {
          INetSocket *s = gSocketMan.FetchSocket(m_nServerSocket);
          TCPSocket *sock = static_cast<TCPSocket *>(s);
          if (sock->RecvRaw())
          {
            CMSNBuffer packet(sock->RecvBuffer());
            sock->ClearRecvBuffer();
            gSocketMan.DropSocket(sock);
            ProcessServerPacket(packet);
          }
          else
          {
            // Time to reconnect
            gLog.Info("%sDisconnected from server, reconnecting.\n", L_MSNxSTR);
            gSocketMan.CloseSocket(m_nServerSocket);
            gSocketMan.DropSocket(sock);
            m_nServerSocket = -1;
            MSNLogon("messenger.hotmail.com", 1863);
          }
        }
        
        else if (nCurrent == m_nSSLSocket)
        {
          INetSocket *s = gSocketMan.FetchSocket(m_nSSLSocket);
          TCPSocket *sock = static_cast<TCPSocket *>(s);
          if (sock->SSLRecv())
          {
            CMSNBuffer packet(sock->RecvBuffer());
            sock->ClearRecvBuffer();
            gSocketMan.DropSocket(sock);
            ProcessSSLServerPacket(packet);
          }
        }
        
        else
        {
          //SB socket
          INetSocket *s = gSocketMan.FetchSocket(nCurrent);
          TCPSocket *sock = static_cast<TCPSocket *>(s);
          if (sock->RecvRaw())
          {
            CMSNBuffer packet(sock->RecvBuffer());
            bool bProcess = false;
            sock->ClearRecvBuffer();
            char *szUser = strdup(sock->OwnerId());
            gSocketMan.DropSocket(sock);
            
            // Build the packet with last received portion
            string strUser(szUser);
            SBuffer *pBuf = RetrievePacket(strUser, nCurrent);
            if (pBuf)
            {
              *(pBuf->m_pBuf) += packet;
            }
            else
            {
              pBuf = new SBuffer;
              pBuf->m_pBuf = new CMSNBuffer(packet);
              pBuf->m_strUser = strUser;
            }
            
            // Do we have the entire packet?
            char *szNeedle;
            if ((szNeedle = strstr((char *)pBuf->m_pBuf->getDataStart(), "\r\n")))
            {
              // We have a basic packet, now check for a packet that has a payload
              if (memcmp(pBuf->m_pBuf->getDataStart(), "MSG", 3) == 0)
              {
                pBuf->m_pBuf->SkipParameter(); // command
                pBuf->m_pBuf->SkipParameter(); // user id
                pBuf->m_pBuf->SkipParameter(); // alias
                string strSize = pBuf->m_pBuf->GetParameter();
                int nSize = atoi(strSize.c_str());
                  
                if (nSize == (pBuf->m_pBuf->getDataPosWrite() - pBuf->m_pBuf->getDataPosRead()))
                {
                  bProcess = true;
                }
                else
                {
                  StorePacket(pBuf, nCurrent);
                }
                
                pBuf->m_pBuf->Reset();  
              }
              else
                bProcess = true; // no payload
            }  
            
            if (bProcess)
            {
              ProcessSBPacket(szUser, pBuf->m_pBuf);
              RemovePacket(strUser, nCurrent);
              delete pBuf;
            }
            
            free(szUser);
          }
        }
      }

      nCurrent++;
    }

  }
  
  //pthread_cancel(m_tMSNPing);
}

void CMSN::ProcessPipe()
{
  char buf[16];
  read(m_nPipe, buf, 1);
  switch (buf[0])
  {
  case 'S':  // A signal is pending
    {
      CSignal *s = m_pDaemon->PopProtoSignal();
      ProcessSignal(s);
      break;
    }

  case 'X': // Bye
    gLog.Info("%sExiting.\n", L_MSNxSTR);
    m_bExit = true;
    break;
  }
}

void CMSN::ProcessSignal(CSignal *s)
{
  switch (s->Type())
  {
    case PROTOxLOGON:
    {
      if (m_nServerSocket < 0)
        MSNLogon("messenger.hotmail.com", 1863);
      break;
    }
    
    case PROTOxCHANGE_STATUS:
    case PROTOxLOGOFF:
    case PROTOxADD_USER:
    case PROTOxREM_USER:
      break;

    case PROTOxSENDxMSG:
    {
      CSendMessageSignal *sig = static_cast<CSendMessageSignal *>(s);
      MSNSendMessage(sig->Id(), sig->Message(), sig->Thread());
      break;
    }
  }
}

void CMSN::ProcessSSLServerPacket(CMSNBuffer &packet)
{
  // Did we receive the entire packet?
  // I don't like doing this, is there a better way to see
  // if we get the entire packet at the socket level?
  // I couldn't find anything in the HTTP RFC about this packet
  // being broken up without any special HTTP headers
  static CMSNBuffer sSSLPacket = packet;
  
  char *ptr = packet.getDataStart() + packet.getDataSize() - 4;
  int x = memcmp(ptr, "\x0D\x0A\x0D\x0A", 4);
  if (sSSLPacket.getDataSize() != packet.getDataSize())
    sSSLPacket += packet;
  
  if (x)  return;
  
  // Now process the packet
  char cTmp = 0;
  string strFirstLine = "";
  
  sSSLPacket >> cTmp;
  while (cTmp != '\r')
  {
    strFirstLine += cTmp;
    sSSLPacket >> cTmp;
  }
  
  sSSLPacket >> cTmp; // skip \n as well
  
  // Authenticated
  if (strFirstLine == "HTTP/1.1 200 OK")
  {
    sSSLPacket.ParseHeaders();
    char *fromPP = strstr(sSSLPacket.GetValue("Authentication-Info").c_str(), "from-PP=");
    fromPP+= 9; // skip to the tag
    char *endTag = strchr(fromPP, '\'');
    char *tag = strndup(fromPP, endTag - fromPP); // Thanks, this is all we need
    
    CMSNPacket *pReply = new CPS_MSNSendTicket(tag);
    SendPacket(pReply);
    free(tag);
  }
  else if (strFirstLine == "HTTP/1.1 401 Unauthorized")
  {
    gLog.Error("%sInvalid password.\n", L_MSNxSTR);
  }
  else
  {
    gLog.Error("%sUnknown sign in error.\n", L_MSNxSTR);
  }
  
  gSocketMan.CloseSocket(m_nSSLSocket, false, true);
  m_nSSLSocket = -1;
}

void CMSN::ProcessSBPacket(char *szUser, CMSNBuffer *packet)
{
  char szCommand[4];
  CMSNPacket *pReply;
  
  while (!packet->End())
  {
    pReply = 0;
    packet->UnpackRaw(szCommand, 3);
    string strCmd(szCommand);
    
    if (strCmd == "IRO")
    {
      packet->SkipParameter(); // Seq
      packet->SkipParameter(); // current user to add
      packet->SkipParameter(); // total usrers in conversation
      string strUser = packet->GetParameter();
      
      gLog.Info("%s%s joined the conversation.\n", L_MSNxSTR, strUser.c_str());
    }
    else if (strCmd == "ANS")
    {
      // just OK, ready to talk
      // we can ignore this
    }
    else if (strCmd == "MSG")
    {
      string strUser = packet->GetParameter();
      packet->SkipParameter(); // Nick
      string strSize = packet->GetParameter();
      packet->SkipPacket(); // Skip \r\m
      packet->ParseHeaders();
      int nSize = atoi(strSize.c_str());
      
      string strType = packet->GetValue("Content-Type");
      if (strType == "text/x-msmsgscontrol")
      {
        //printf("%s is typing\n", strUser.c_str());
      }
      else if (strncmp(strType.c_str(), "text/plain", 10) == 0)
      {
        int nCurrent = packet->getDataPosWrite() - packet->getDataPosRead();
        char szMsg[nSize - nCurrent - 1];
        int i;
        for (i = 0; i < (nSize - nCurrent - 2); i++)
          (*packet) >> szMsg[i];
        szMsg[i] = '\0';
        
        CEventMsg *e = CEventMsg::Parse(szMsg, ICQ_CMDxRCV_SYSxMSGxOFFLINE, time(0), 0);
        ICQUser *u = gUserManager.FetchUser(strUser.c_str(), MSN_PPID, LOCK_W);
        if (m_pDaemon->AddUserEvent(u, e))
          m_pDaemon->m_xOnEventManager.Do(ON_EVENT_MSG, u);
        gUserManager.DropUser(u);
      }
    }
    else if (strCmd == "ACK")
    {
      string strId = packet->GetParameter();
      unsigned long nSeq = (unsigned long)atoi(strId.c_str());
      ICQEvent *e = RetrieveEvent(nSeq);
      if (e)
      {
        e->m_eResult = EVENT_ACKED;
        if (e->m_pUserEvent)
        {
          ICQUser *u = gUserManager.FetchUser(e->m_szId, e->m_nPPID, LOCK_R);
          if (u != NULL)
          {
            e->m_pUserEvent->AddToHistory(u, D_SENDER);
            u->SetLastSentEvent();
            m_pDaemon->m_xOnEventManager.Do(ON_EVENT_MSGSENT, u);
            gUserManager.DropUser(u);
          }
          m_pDaemon->m_sStats[STATS_EventsSent].Inc();
        }
      }

      m_pDaemon->PushPluginEvent(e);
    }
    else if (strCmd == "USR")
    {
      SStartMessage *pStart = m_lStart.front();
      pReply = new CPS_MSNCall(pStart->m_szUser);
    }
    else if (strCmd == "JOI")
    {
      string strUser = packet->GetParameter();
      gLog.Info("%s%s joined the conversation.\n", L_MSNxSTR, strUser.c_str());
      
      SStartMessage *pStart = 0;
      StartList::iterator it;
      for (it = m_lStart.begin(); it != m_lStart.end(); it++)
      {
        if (strcmp((*it)->m_szUser, strUser.c_str()) == 0) // case insensitive perhaps?
        {
          pStart = *it;
          m_lStart.erase(it);
          break;
        }
      }
      
      if (pStart)
      {
        MSNSendMessage(pStart->m_szUser, pStart->m_szMsg, pStart->m_tPlugin);
        delete pStart;
      }
    }
    else if (strCmd == "BYE")
    {
      // closed the window
    }
  
    // Get the next packet
    packet->SkipPacket();
    
    if (pReply)
    {
      string strTo(szUser);
      Send_SB_Packet(strTo, pReply);
    }
  }
  
  delete packet;
}

void CMSN::ProcessServerPacket(CMSNBuffer &packet)
{
  char szCommand[4];
  CMSNPacket *pReply;
  
  // Build the entire packet
  if (!m_pPacketBuf)
    m_pPacketBuf = new CMSNBuffer(packet);
  else
    *m_pPacketBuf += packet;
    
  if (memcmp((void *)(&(m_pPacketBuf->getDataStart())[m_pPacketBuf->getDataSize() - 2]), "\x0D\x0A", 2))
    return;
  
  while (!m_pPacketBuf->End())
  {
    pReply = 0;
    m_pPacketBuf->UnpackRaw(szCommand, 3);
    string strCmd(szCommand);
    
    if (strCmd == "VER")
    {
      // Don't really care about this packet's data.
      pReply = new CPS_MSNClientVersion(m_szUserName);
    }
    else if (strCmd == "CVR")
    {
      // Don't really care about this packet's data.
      pReply = new CPS_MSNUser(m_szUserName);
    }
    else if (strCmd == "XFR")
    {
      //Time to transfer to a new server
      m_pPacketBuf->SkipParameter(); // Seq
      string strServType = m_pPacketBuf->GetParameter();
      string strServer = m_pPacketBuf->GetParameter();
    
      if (strServType == "SB")
      {
        m_pPacketBuf->SkipParameter(); // 'CKI'
        string strCookie = m_pPacketBuf->GetParameter();
        
        MSNSBConnectStart(strServer, strCookie);
      }
      else
      {
        const char *szParam = strServer.c_str();
        char szNewServer[16];
        char *szPort;
        if ((szPort = strchr(szParam, ':')))
        {
          strncpy(szNewServer, szParam, szPort - szParam);
          szNewServer[szPort - szParam] = '\0';
          *szPort++ = '\0';
        }
        
        gSocketMan.CloseSocket(m_nServerSocket, false, true);
  
        // Make the new connection
        MSNLogon(szNewServer, atoi(szPort));
      }
    }
    else if (strCmd == "USR")
    {
      m_pPacketBuf->SkipParameter(); // Seq
      string strType = m_pPacketBuf->GetParameter();
      
      if (strType == "OK")
      {
        m_pPacketBuf->SkipParameter(); // email account
        string strNick = m_pPacketBuf->GetParameter();
        gLog.Info("%s%s logged in.\n", L_MSNxSTR, strNick.c_str());
        
        pReply = new CPS_MSNSync();
      }
      else
      {
        m_pPacketBuf->SkipParameter(); // "S"
        string strParam = m_pPacketBuf->GetParameter();
      
        // Make an SSL connection to authenticate
        MSNAuthenticate(strdup(strParam.c_str()));
      }
    }
    else if (strCmd == "CHL")
    {
      m_pPacketBuf->SkipParameter(); // Seq
      string strHash = m_pPacketBuf->GetParameter();
      
      pReply = new CPS_MSNChallenge(strHash.c_str());
    }
    else if (strCmd == "SYN")
    {
      m_pPacketBuf->SkipParameter();
      string strVersion = m_pPacketBuf->GetParameter();
      
      pReply = new CPS_MSNChangeStatus();
    }
    else if (strCmd == "LST")
    {
      // Add user
      string strUser = m_pPacketBuf->GetParameter();
      m_pDaemon->AddUserToList(strUser.c_str(), MSN_PPID);
    }
    else if (strCmd == "LSG")
    {
      // Add group
    }
    else if (strCmd == "CHG")
    {
      // Send our local list now
      FOR_EACH_PROTO_USER_START(MSN_PPID, LOCK_R)
      {
        pReply = new CPS_MSNAddUser(pUser->IdString());
        SendPacket(pReply);
      }
      FOR_EACH_PROTO_USER_END
      
      pReply = 0;
    }
    else if (strCmd == "ILN" || strCmd == "NLN")
    {
      if (strCmd == "ILN")
        m_pPacketBuf->SkipParameter(); // seq
      string strStatus = m_pPacketBuf->GetParameter();
      string strUser = m_pPacketBuf->GetParameter();
      unsigned short nStatus = ICQ_STATUS_AWAY;
      
      if (strStatus == "NLN")
        nStatus = ICQ_STATUS_ONLINE;
        
      ICQUser *u = gUserManager.FetchUser(strUser.c_str(), MSN_PPID, LOCK_W);
      if (u)
      {
        gLog.Info("%s%s changed status (%s).\n", L_SRVxSTR, u->GetAlias(), strStatus.c_str());
        m_pDaemon->ChangeUserStatus(u, nStatus);
      }
      gUserManager.DropUser(u);
    }
    else if (strCmd == "FLN")
    {
      string strUser = m_pPacketBuf->GetParameter();
      
      ICQUser *u = gUserManager.FetchUser(strUser.c_str(), MSN_PPID, LOCK_W);
      if (u)
      {
        gLog.Info("%s%s logged off.\n", L_MSNxSTR, u->GetAlias());
        m_pDaemon->ChangeUserStatus(u, ICQ_STATUS_OFFLINE);
      }
      gUserManager.DropUser(u);
    }
    else if (strCmd == "RNG")
    {
      string strSessionID = m_pPacketBuf->GetParameter();
      string strServer = m_pPacketBuf->GetParameter();
      m_pPacketBuf->SkipParameter(); // 'CKI'
      string strCookie = m_pPacketBuf->GetParameter();
      string strUser = m_pPacketBuf->GetParameter();
      
      MSNSBConnectAnswer(strServer, strSessionID, strCookie, strUser);
    }
    
    // Get the next packet
    m_pPacketBuf->SkipPacket();
    
    if (pReply)
      SendPacket(pReply);
  }
  
  // Clear it out
  delete m_pPacketBuf;
  m_pPacketBuf = 0;
}

void CMSN::SendPacket(CMSNPacket *p)
{
  INetSocket *s = gSocketMan.FetchSocket(m_nServerSocket);
  SrvSocket *sock = static_cast<SrvSocket *>(s);
  sock->SendRaw(p->getBuffer());
  gSocketMan.DropSocket(sock);
  
  delete p;
}

void CMSN::Send_SB_Packet(string &strUser, CMSNPacket *p, bool bDelete)
{
  ICQUser *u = gUserManager.FetchUser(const_cast<char *>(strUser.c_str()), MSN_PPID, LOCK_R);
  if (!u) return;

  int nSock = u->SocketDesc(ICQ_CHNxNONE);
  gUserManager.DropUser(u);  
  INetSocket *s = gSocketMan.FetchSocket(nSock);
  TCPSocket *sock = static_cast<TCPSocket *>(s);
  sock->SendRaw(p->getBuffer());
  gSocketMan.DropSocket(sock);
  
  if (bDelete)
    delete p;
}

void CMSN::MSNLogon(const char *_szServer, int _nPort)
{
  ICQOwner *o = gUserManager.FetchOwner(MSN_PPID, LOCK_R);
  if (!o)
  {
    gLog.Error("%sNo owner set.\n", L_MSNxSTR);
    return;
  }
  m_szUserName = strdup(o->IdString());
  m_szPassword = strdup(o->Password());
  gUserManager.DropOwner(MSN_PPID);
  
  SrvSocket *sock = new SrvSocket(m_szUserName, MSN_PPID);
  sock->SetRemoteAddr(_szServer, _nPort);
  char ipbuf[32];
  gLog.Info("%sServer found at %s:%d.\n", L_MSNxSTR, sock->RemoteIpStr(ipbuf), sock->RemotePort());
  
  if (!sock->OpenConnection())
  {
    gLog.Info("%sConnect failed.\n", L_MSNxSTR);
    delete sock;
    return;
  }
  
  gSocketMan.AddSocket(sock);
  m_nServerSocket = sock->Descriptor();
  gSocketMan.DropSocket(sock);
  
  CMSNPacket *pHello = new CPS_MSNVersion();
  SendPacket(pHello);
}

void CMSN::MSNAuthenticate(char *szCookie)
{
  TCPSocket *sock = new TCPSocket(m_szUserName, MSN_PPID);
  sock->SetRemoteAddr("loginnet.passport.com", 443);
  char ipbuf[32];
  gLog.Info("%sAuthenticating to %s:%d\n", L_MSNxSTR, sock->RemoteIpStr(ipbuf), sock->RemotePort());
  
  if (!sock->OpenConnection())
  {
    gLog.Error("%sConnection to %s failed.\n", L_MSNxSTR, sock->RemoteIpStr(ipbuf));
    delete sock;
    free(szCookie);
    return;
  }
  
  if (!sock->SecureConnect())
  {
    gLog.Error("%sSSL connection failed.\n", L_MSNxSTR);   
    delete sock;
    return;
  }
  
  gSocketMan.AddSocket(sock);
  m_nSSLSocket = sock->Descriptor();
  CMSNPacket *pHello = new CPS_MSNAuthenticate(m_szUserName, m_szPassword, szCookie);
  sock->SSLSend(pHello->getBuffer());
  gSocketMan.DropSocket(sock);

  free(szCookie);
}

bool CMSN::MSNSBConnectStart(string &strServer, string &strCookie)
{
  const char *szParam = strServer.c_str();
  char szServer[16];
  char *szPort;
  if ((szPort = strchr(szParam, ':')))
  {
    strncpy(szServer, szParam, szPort - szParam);
    szServer[szPort - szParam] = '\0';
    *szPort++ = '\0';
  }
  
  SStartMessage *pStart = m_lStart.front();
  
  TCPSocket *sock = new TCPSocket(pStart->m_szUser, MSN_PPID);
  sock->SetRemoteAddr(szServer, atoi(szPort));
  char ipbuf[32];
  gLog.Info("%sConnecting to SB at %s:%d.\n", L_MSNxSTR, sock->RemoteIpStr(ipbuf),
    sock->RemotePort());
  
  if (!sock->OpenConnection())
  {
    gLog.Error("%sConnection to SB at %s failed.\n", L_MSNxSTR, sock->RemoteIpStr(ipbuf));
    delete sock;
    return false;
  }
  
  gSocketMan.AddSocket(sock);
  ICQUser *u = gUserManager.FetchUser(pStart->m_szUser, MSN_PPID, LOCK_W);
  if (u)
  {
    u->SetSocketDesc(sock);
    gUserManager.DropUser(u);
  }
  gSocketMan.DropSocket(sock);
  
  CMSNPacket *pReply = new CPS_MSN_SBStart(strCookie.c_str(), m_szUserName);
  string strUser(pStart->m_szUser);
  Send_SB_Packet(strUser, pReply);  

  return true;
}

bool CMSN::MSNSBConnectAnswer(string &strServer, string &strSessionId, string &strCookie,
                              string &strUser)
{
  const char *szParam = strServer.c_str();
  char szServer[16];
  char *szPort;
  if ((szPort = strchr(szParam, ':')))
  {
    strncpy(szServer, szParam, szPort - szParam);
    szServer[szPort - szParam] = '\0';
    *szPort++ = '\0';
  }
  
  TCPSocket *sock = new TCPSocket(strUser.c_str(), MSN_PPID);
  sock->SetRemoteAddr(szServer, atoi(szPort));
  char ipbuf[32];
  gLog.Info("%sConnecting to SB at %s:%d.\n", L_MSNxSTR, sock->RemoteIpStr(ipbuf),
    sock->RemotePort());
  
  if (!sock->OpenConnection())
  {
    gLog.Error("%sConnection to SB at %s failed.\n", L_MSNxSTR, sock->RemoteIpStr(ipbuf));
    delete sock;
    return false;
  }
  
  gSocketMan.AddSocket(sock);
  CMSNPacket *pReply = new CPS_MSN_SBAnswer(strSessionId.c_str(),
    strCookie.c_str(), m_szUserName);
  ICQUser *u = gUserManager.FetchUser(strUser.c_str(), MSN_PPID, LOCK_W);
  if (u)
  {
    u->SetSocketDesc(sock);
    gUserManager.DropUser(u);
  }
  
  gSocketMan.DropSocket(sock);
  
  Send_SB_Packet(strUser, pReply);
  
  return true;
}

void CMSN::MSNSendMessage(char *_szUser, char *_szMsg, pthread_t _tPlugin)
{
  string strUser(_szUser);
  
  ICQUser *u = gUserManager.FetchUser(_szUser, MSN_PPID, LOCK_R);
  if (!u) return;
  int nSockDesc = u->SocketDesc(ICQ_CHNxNONE);
  gUserManager.DropUser(u);
  
  if (nSockDesc > 0)
  {
    CMSNPacket *pSend = new CPS_MSNMessage(_szMsg);
    CEventMsg *m = new CEventMsg(_szMsg, 0, TIME_NOW, 0);
    m->m_eDir = D_SENDER;
    ICQEvent *e = new ICQEvent(m_pDaemon, 0, pSend, CONNECT_SERVER, strdup(_szUser), MSN_PPID, m);
    m_pEvents.push_back(e);
    
    CICQSignal *s = new CICQSignal(SIGNAL_EVENTxID, 0, strdup(_szUser), MSN_PPID, e->EventId());
    e->thread_plugin = _tPlugin;
    m_pDaemon->PushPluginSignal(s);
      
    Send_SB_Packet(strUser, pSend, false);
  }
  else
  {
    // Must connect to the SB and call the user
    CMSNPacket *pSend = new CPS_MSNXfr();
      
    SStartMessage *p = new SStartMessage;
    p->m_szUser = strdup(_szUser);
    p->m_szMsg = strdup(_szMsg);
    p->m_tPlugin = _tPlugin;
    m_lStart.push_back(p);
   
    SendPacket(pSend);
  }  
}


void CMSN::MSNPing()
{
  CMSNPacket *pSend = new CPS_MSNPing();
  SendPacket(pSend);
}

void *MSNPing_tep(void *p)
{
  pthread_detach(pthread_self());
  
  CMSN *pMSN = (CMSN *)p;
  
  struct timeval tv;

  while (true)
  {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    
    if (pMSN->Connected())
      pMSN->MSNPing();

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    select(0, 0, 0, 0, &tv);

    pthread_testcancel();
  }  
  
  return 0;
}
