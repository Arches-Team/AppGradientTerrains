#include "qte.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include "noise.h"
#include "turbulence.h"
#include "gputerrainsimu.h"

/*!
\brief Updates the gradient
*/
void QteWindow::UpdateGradient()
{
  if (ui.checkBox_record->isChecked())
    mayawidget->gradientTerrain.ToggleRecord(1);
  mayawidget->gradientTerrain.UpdateHeightField();
  mayawidget->displayedHF = mayawidget->gradientTerrain.GetHeightField();
  mayawidget->editable = true;
}

/*!
\brief Build the mesh and the texture of the terrain and display it
\param camera true to move the camera so that the whole terrain is visible
*/
void QteWindow::Render(bool camera)
{
  MayaWidgetTerrainEditor* widget = mayawidget;

  widget->ClearAll();

  // Geometry
  Box2 box2 = Box2(widget->displayedHF.GetBox());

  MayaGeometry mg("HF", widget->displayedHF.CreateMesh(), RenderTexture());
  mg.generatePlannarZ_Mapping(box2);
  world = MayaGeometryAll(mg);

  widget->SetWorld(world);

  // Light
  widget->SetLight(box2.Center().ToVector(1000.0));

  // Camera
  if (camera == true)
  {
    Camera cc = Camera::View(box2.ToBox(0.0, 200.0));
    cc.SetPlanes(1.0, 200000.0);
    widget->SetCamera(cc);
  }
}

/*!
\brief Load a terrain from an image of gradients
*/
void QteWindow::LoadGradientCall()
{
  QString filename = QFileDialog::getOpenFileName(nullptr, "Choose a gradient to open", "./", "*.png");
  if (filename == QString("")) {
    return;
  }
  GradientTerrain gt(filename);

  mayawidget->gradientTerrain = gt;

  UpdateUndoButton();
  UpdateGradient();
  Render(true);
}

/*!
\brief Load a terrain from an image of heights
*/
void QteWindow::LoadHeightfieldCall()
{
  QString filename = QFileDialog::getOpenFileName(nullptr, "Choose a heightfield to open", "./", "*.png");
  if (filename == QString("")) {
    return;
  }
  GradientTerrain gt(filename, true);

  mayawidget->gradientTerrain = gt;

  UpdateUndoButton();
  UpdateGradient();
  Render(true);
}

/*!
\brief Generates a 513x513 terrain by using fBm
*/
void QteWindow::FBMCall()
{
  NoiseTurbulence2 noise(0.0, 0.2 * 2.1 / HFMAGN, 2.0, 0.5, 0.5, 6);
  HeightField hf = noise.Sample(Array2(Box2(513., 513.), 513, 513));

  VectorField2 g = hf.Gradient();

  mayawidget->gradientTerrain = GradientTerrain(g);
  UpdateGradient();
  Render(true);

  mayawidget->gradientTerrain.HistorySave();
  UpdateUndoButton();
}

/*!
\brief Generates a 1025x1025 terrain by using fBm
*/
void QteWindow::FBMCall_1025()
{
  Noise2 noise;
  GradientTerrain gt("1025.png");
  if (gt.GetSizeX() == 0 || gt.GetSizeY() == 0) {
    QMessageBox::warning(nullptr, "Loading error", "Could not load 1025.png.");
    return;
  }

  // same size as gt, since the gradient of hf is copied into gt
  HeightField hf(Box2(40.0, 40.0), gt.GetSizeX(), gt.GetSizeY(), 0.0);

  for (int i = 0; i < hf.GetSizeX(); i++) {
    for (int j = 0; j < hf.GetSizeY(); j++) {
      double x = i / double(hf.GetSizeX() - 1.0) * 40.0 - 20.0;
      double y = j / double(hf.GetSizeY() - 1.0) * 40.0 - 20.0;
      Vector2 p(x, y);

      hf[hf.VertexIndex(i, j)] = 0.2 * (2.1 * noise.Value(0.5 * p) + noise.Value(1. * p) + 0.53 * noise.Value(2. * p) + 0.27 * noise.Value(4. * p) + 0.13 * noise.Value(8. * p) + 0.7 * noise.Value(16. * p)) / HFMAGN;
    }
  }
  VectorField2 g = hf.Gradient();

  for (int i = 0; i < hf.GetSizeX(); i++) {
    for (int j = 0; j < hf.GetSizeY(); j++) {
      gt[gt.VertexIndex(i, j)] = g[gt.VertexIndex(i, j)];
    }
  }
  mayawidget->gradientTerrain = gt;
  UpdateGradient();
  Render(true);

  UpdateUndoButton();
}

/*!
\brief Freezes the altitude of the river network
*/
void QteWindow::FreezeRiversCall()
{
  mayawidget->gradientTerrain.HistorySave();
  if (ui.checkBox_Zone->isChecked()) {
    mayawidget->gradientTerrain.FreezeRivers(mayawidget->GetLassoPolygon(), 0.5);
  }
  else {
    mayawidget->gradientTerrain.FreezeRivers(0.5);
  }
  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief Freezes the altitude of the ridge network
*/
void QteWindow::FreezeRidgesCall()
{
  mayawidget->gradientTerrain.HistorySave();
  if (ui.checkBox_Zone->isChecked()) {
    mayawidget->gradientTerrain.FreezeRidges(mayawidget->GetLassoPolygon());
  }
  else {
    mayawidget->gradientTerrain.FreezeRidges();
  }
  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief Lowers all the gradients by a factor of 1.2
*/
void QteWindow::LowerAllCall()
{
  mayawidget->gradientTerrain.HistorySave();
  mayawidget->gradientTerrain.Mul(1. / 1.2);
  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief Increases all the gradients by a factor of 1.2
*/
void QteWindow::IncreaseAllCall()
{
  mayawidget->gradientTerrain.HistorySave();
  mayawidget->gradientTerrain.Mul(1.2);
  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief Remove the altitude constraints
*/
void QteWindow::ForgetConstraintsCall()
{
  mayawidget->gradientTerrain.HistorySave();
  mayawidget->gradientTerrain.ForgetConstraints();
  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief Update the gradient wrt the current HF
*/
void QteWindow::SyncGradientCall()
{
  mayawidget->gradientTerrain.HistorySave();
  mayawidget->gradientTerrain.SyncGradient();
  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief Hydraulic erosion on the GPU
*/
void QteWindow::GPUErosionCall()
{
  mayawidget->makeCurrent();
  mayawidget->gradientTerrain.HistorySave();
  mayawidget->gradientTerrain.GPUErosion();

  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief exports the terrain currently displayed in the edit Widget to a png heightfield greyscale image
*/
void QteWindow::ExportCurrentHeightFieldToPng() {
  QString filename = QFileDialog::getSaveFileName(nullptr, "Choose a filename to export this heightfield", "./", "*.png");
  if (filename.isEmpty())
    return;

  QImage image = mayawidget->gradientTerrain.GetHeightField().CreateImage(-0.5, 0.5);
  image.save(filename);
}
