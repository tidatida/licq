#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "licqgui.h"
#include "sigman.h"
#include "outputwin.h"
#include "log.h"
#include "plugin.h"
#ifdef USE_KDE
#include <kthemestyle.h>
#endif
#include <qwindowsstyle.h>
#include <qmotifstyle.h>
#include <qplatinumstyle.h>
#include <qcdestyle.h>

#include "icqd.h"

CLicqGui *licqQtGui;

void LP_Usage(void)
{
  fprintf(stderr, "Licq Plugin: %s %s\n", LP_Name(), LP_Version());
  fprintf(stderr, "Usage:  Licq [options] -p qt-gui -- [-h] [-s skinname] [-i iconpack] [-g gui style]\n");
  fprintf(stderr, " -h : this help screen\n");
  fprintf(stderr, " -s : set the skin to use (must be in {base dir}/qt-gui/skin.skinname)\n");
  fprintf(stderr, " -i : set the icons to use (must be in {base dir}/qt-gui/icons.iconpack)\n");
  fprintf(stderr, " -g : set the gui style (MOTIF / WINDOWS / MAC / CDE)\n\n");
}

const char *LP_Name(void)
{
#ifdef USE_KDE
  static const char name[] = "KDE GUI";
#else
  static const char name[] = "Qt GUI";
#endif
  return name;
}


const char *LP_Version(void)
{
  static const char version[] = VERSION;
  return version;
}

const char *LP_Status(void)
{
  static const char status[] = "running";
  return status;
}

bool LP_Init(int argc, char **argv)
{
  char skinName[32] = "";
  char iconsName[32] = "";
  char styleName[32] = "";

  // parse command line for arguments
  int i = 0;
  while( (i = getopt(argc, argv, "hs:i:g:")) > 0)
  {
    switch (i)
    {
    case 'h':  // help
      LP_Usage();
      return false;
    case 's':  // skin name
      sprintf(skinName, "%s", optarg);
      break;
    case 'i':  // icons name
      sprintf(iconsName, "%s", optarg);
      break;
    case 'g': // gui style
      strcpy(styleName, optarg);
      break;
    }
  }
  if (qApp != NULL)
  {
    gLog.Error("%sA Qt application is already loaded.\n%sRemove the plugin from the command line.\n", L_ERRORxSTR, L_BLANKxSTR);
    return false;
  }
  licqQtGui = new CLicqGui(argc, argv, skinName, iconsName, styleName);
  return (licqQtGui != NULL);
}


int LP_Main(CICQDaemon *_licqDaemon)
{
  int nResult = licqQtGui->Run(_licqDaemon);
  licqQtGui->Shutdown();
  return nResult;
}

QStyle *CLicqGui::SetStyle(const char *_szStyle)
{
  QStyle *s = NULL;
  if (strncmp(_szStyle, "MOTIF", 3) == 0)
    s = new QMotifStyle;
  else if (strncmp(_szStyle, "WINDOWS", 3) == 0)
    s = new QWindowsStyle;
  else if (strncmp(_szStyle, "MAC", 3) == 0)
    s = new QPlatinumStyle;
  else if (strncmp(_szStyle, "CDE", 3) == 0)
    s = new QCDEStyle;
#ifdef USE_KDE
  else if (strncmp(_szStyle, "KDE", 3) == 0)
    s = new KThemeStyle;
#endif
  return s;
}


CLicqGui::CLicqGui(int argc, char **argv, const char *_szSkin, const char *_szIcons, const char *_szStyle)
#ifdef USE_KDE
: KApplication(argc, argv, "KDE GUI")
#else
: QApplication(argc, argv)
#endif
{
  char buf[64];
  sprintf(buf, "%s/licq_qt-gui.style", BASE_DIR);

  QStyle *style = SetStyle(_szStyle);

  // Write out the style if not NULL
  if (style != NULL)
  {
    FILE *f = fopen(buf, "w");
    if (f != NULL)
    {
      fprintf(f, "%s\n", _szStyle);
      fclose(f);
    }
  }
  // Otherwise try and load it from the file
  else
  {
#ifdef USE_KDE
    style = new KThemeStyle;
#else
    FILE *f = fopen(buf, "r");
    if (f != NULL)
    {
      if (fgets(buf, 64, f) != NULL)
        style = SetStyle(buf);
      fclose(f);
    }
    if (style == NULL) style = new STYLE;
#endif
  }

  setStyle(style);
  m_szSkin = strdup(_szSkin);
  m_szIcons = strdup(_szIcons);

  // Try and load a translation
  char *p;
  if ( (p = getenv("LANGUAGE")) || (p = getenv("LANG")) );
  {
    QString str;
    str.sprintf("%s/qt-gui/locale/%s.qm", SHARE_DIR, p);
    QTranslator *trans = new QTranslator(this);
    trans->load(str);
    installTranslator(trans);
  }
}


CLicqGui::~CLicqGui(void)
{
  delete licqSignalManager;
  delete licqLogWindow;
}

void CLicqGui::Shutdown(void)
{
  gLog.Info("%sShutting down gui.\n", L_ENDxSTR);
  gLog.ModifyService(S_PLUGIN, 0);
  delete licqMainWindow;
}


int CLicqGui::Run(CICQDaemon *_licqDaemon)
{
  // Register with the daemon, we want to receive all signals
  int nPipe = _licqDaemon->RegisterPlugin(SIGNAL_ALL);

  // Create the main widgets
  licqSignalManager = new CSignalManager(_licqDaemon, nPipe);
  licqLogWindow = new CQtLogWindow;
  gLog.AddService(new CLogService_Plugin(licqLogWindow, L_INFO | L_WARN | L_ERROR));
  licqMainWindow = new CMainWindow(_licqDaemon, licqSignalManager, licqLogWindow, m_szSkin, m_szIcons);

  setMainWidget(licqMainWindow);
  licqMainWindow->show();
  int r = exec();
  _licqDaemon->UnregisterPlugin();

  return r;
}

