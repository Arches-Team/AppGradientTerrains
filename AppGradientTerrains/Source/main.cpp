#include "qte.h"

#include <QtCore/QDir>

// The libraries locate their shaders and textures through the ARCHESLIBDIR environment variable.
// Point it to the copy of the libraries shipped with this project (the "Libs" folder found above the
// executable) so that the application does not depend on a global installation.
static void UseLocalLibs()
{
  QDir dir(QCoreApplication::applicationDirPath());
  do
  {
    if (QFileInfo::exists(dir.filePath("Libs/LibMaya/Shaders")))
    {
      qputenv("ARCHESLIBDIR", QDir::toNativeSeparators(dir.filePath("Libs")).toLocal8Bit());
      return;
    }
  } while (dir.cdUp());

#ifdef ARCHES_LIBS_DIR
  // Built with CMake outside the source tree: use the folder recorded at configuration time
  if (QFileInfo::exists(QString(ARCHES_LIBS_DIR) + "/LibMaya/Shaders"))
  {
    qputenv("ARCHESLIBDIR", QDir::toNativeSeparators(QString(ARCHES_LIBS_DIR)).toLocal8Bit());
  }
#endif
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  app.setStyle("Fusion");

  UseLocalLibs();

  QteWindow mainWin;

  mainWin.showMaximized();
  return app.exec();
}
