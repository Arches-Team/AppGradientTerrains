#include "maya.h"
#include "qte.h"

#include <QtWidgets/QFileDialog>

/*!
\brief Export model as an .obj file.
*/
void QteWindow::ExportObj()
{
  QString filename = QFileDialog::getSaveFileName(nullptr,
    "Choose a filename to save",
    "",
    "OBJ file (*.obj)");

  if (!filename.isNull())
  {
    world.Save_OBJ(filename);
  }
}

/*!
\brief Export model as an .obj file.
*/
void QteWindow::Save8()
{
  QString filename = QFileDialog::getSaveFileName(nullptr,
    "Choose a filename to save",
    "",
    "png file (*.png)");

  if (!filename.isNull())
  {
    QImage image = mayawidget->displayedHF.CreateImage(true);
    image.save(filename);
  }
}

/*!
\brief Export model as an .obj file.
*/
void QteWindow::Save32()
{
  QString filename = QFileDialog::getSaveFileName(nullptr,
    "Choose a filename to save",
    "",
    "png file (*.png)");

  if (!filename.isNull())
  {
    QImage image = mayawidget->displayedHF.CreateImage(0.0, 256.0 * 256.0, false);
    image.save(filename);
  }
}


