#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif

#include "history.h"
#include "log.h"
#include "constants.h"

CUserHistory::CUserHistory(void)
{
  m_szFileName = m_szDescription = NULL;
}

CUserHistory::~CUserHistory(void)
{
  if (m_szFileName != NULL) free(m_szFileName);
  if (m_szDescription != NULL) free(m_szDescription);
}

void CUserHistory::SetFile(const char *_sz, unsigned long _nUin)
{
  if (m_szFileName != NULL) free(m_szFileName);
  if (m_szDescription != NULL) free(m_szDescription);

  // default history filename, compare only first 7 chars in case of spaces
  if (strncmp(_sz, "default", 7) == 0)
  {
    char temp[MAX_FILENAME_LEN];
    sprintf(temp, "%s%s%ld.history", BASE_DIR, HISTORY_DIR, _nUin);
    m_szFileName = strdup(temp);
    m_szDescription = strdup("default");
  }
  // no history file
  else if (strncmp(_sz, "none", 4) == 0)  // no history file
  {
    m_szFileName = NULL;
    m_szDescription = strdup("none");
  }
  // use given name
  else
  {
    m_szFileName = strdup(_sz);
    m_szDescription = strdup(_sz);
  }
}


void CUserHistory::Load(char *&hbuf)
{
  if (m_szFileName == NULL)
  {
    hbuf = new char[64];
    strcpy(hbuf, "No history file set.");
    return;
  }

  int fd = open(m_szFileName, O_RDONLY);
  if (fd == -1)
  {
    hbuf = new char[256];
    if (errno == ENOENT)
    {
      strcpy(hbuf, "Empty.");
      return;
    }
    else
    {
      sprintf(hbuf, "%sUnable to open history file (%s):\n%s%s.\n", L_WARNxSTR,
              m_szFileName, L_BLANKxSTR, strerror(errno));
      gLog.Warn("%s", hbuf);
      return;
    }
  }
  struct stat buf;
  fstat(fd, &buf);
  hbuf = new char[buf.st_size + 1];
  read(fd, hbuf, buf.st_size);
  hbuf[buf.st_size] = '\0';
  close(fd);
}


void CUserHistory::Save(const char *buf)
{
  if (m_szFileName == NULL || buf == NULL) return;
  int fd = open(m_szFileName, O_WRONLY | O_CREAT | O_TRUNC, 00664);
  if (fd == -1)
  {
    gLog.Error("%sUnable to open history file (%s):\n%s%s.\n", L_ERRORxSTR,
               m_szFileName, L_BLANKxSTR, strerror(errno));
    return;
  }
  write(fd, buf, strlen(buf));
  close(fd);
}


void CUserHistory::Append(const char *_sz)
{
  if (m_szFileName == NULL || _sz == NULL) return;
  int fd = open(m_szFileName, O_WRONLY | O_CREAT | O_APPEND, 00664);
  if (fd == -1)
  {
    gLog.Error("%sUnable to open history file (%s):\n%s%s.\n", L_ERRORxSTR,
               m_szFileName, L_BLANKxSTR, strerror(errno));
    return;
  }
  write(fd, _sz, strlen(_sz));
  close(fd);
}


