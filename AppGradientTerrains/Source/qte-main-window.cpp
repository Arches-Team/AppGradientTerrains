#include "qte.h"
#include "heightfieldshader.h"
#include <QtWidgets/QButtonGroup>
#include <QtCore/QTimer>

/*!
\class QteWindow qte.h
\brief This class implements the main window: the terrain editor and its two tool tabs
(interactive tools and global tools).
*/

/*!
\brief Create widget.
*/
QteWindow::QteWindow()
{
  // Load the interface
  ui.setupUi(this);

  // The tool panel keeps its width when the window is resized, the 3D view takes the extra space
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 1);
  ui.splitter->setSizes({ 430, 1200 });

  // The tools are in scroll areas: they never get squeezed below their natural height, a scroll bar appears instead
  ui.scrollContents_verticalLayout_tab->setMinimumHeight(ui.scrollContents_verticalLayout_tab->sizeHint().height());
  ui.scrollContents_verticalLayout_tab2->setMinimumHeight(ui.scrollContents_verticalLayout_tab2->sizeHint().height());

  // GL widget of the edited terrain
  mayawidget = new MayaWidgetTerrainEditor;
  mayawidget->gradientTerrain = GradientTerrain();

  QGridLayout* GLlayout = new QGridLayout;
  GLlayout->addWidget(mayawidget, 0, 0);
  GLlayout->setContentsMargins(0, 0, 0, 0);
  ui.EditingTab->setLayout(GLlayout);
  mayawidget->SetCamera(Camera(Vector(-10.0, -10.0, 10.0)));

  // Create the connections
  createActions();

  // Default state: the Mountain Range tool is active ...
  ui.Objects_Mountain_2->setChecked(true);
  ToolChanged(20);

  // ... and the 513x513 FBM terrain is loaded as soon as the GL widget is initialized
  QTimer::singleShot(0, this, SLOT(InitialTerrain()));
}

/*!
rief Generate the default terrain (FBM 513). The GL context of the editor must exist, so we retry until it does.
*/
void QteWindow::InitialTerrain()
{
  if (!mayawidget->isValid())
  {
    QTimer::singleShot(50, this, SLOT(InitialTerrain()));
    return;
  }
  mayawidget->makeCurrent();
  FBMCall();
}

/*!
\brief Destroy widget.
*/
QteWindow::~QteWindow()
{
}

/*!
\brief Create callbacks between member slots and user interface.
*/
void QteWindow::createActions()
{
  // File
  connect(ui.actionNoise_terrain_513, SIGNAL(triggered()), this, SLOT(FBMCall()));
  connect(ui.actionNoise_Terrain_1025, SIGNAL(triggered()), this, SLOT(FBMCall_1025()));

  connect(ui.actionLoad_Heightfield, SIGNAL(triggered()), this, SLOT(LoadHeightfieldCall()));
  connect(ui.actionLoad_Gradient, SIGNAL(triggered()), this, SLOT(LoadGradientCall()));

  connect(ui.actionExport_obj, SIGNAL(triggered()), this, SLOT(ExportObj()));
  connect(ui.action_Export_HeightField, SIGNAL(triggered()), this, SLOT(ExportCurrentHeightFieldToPng()));

  connect(ui.actionSave_8_bits, SIGNAL(triggered()), this, SLOT(Save8()));
  connect(ui.actionSave_32_bits, SIGNAL(triggered()), this, SLOT(Save32()));

  connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

  // View
  connect(ui.actionConstraintsOverlay, SIGNAL(toggled(bool)), this, SLOT(RenderCameraUnchanged()));
  connect(ui.actionReconstruct, SIGNAL(triggered()), this, SLOT(Reconstruct()));

  // MayaWidget
  connect(mayawidget, SIGNAL(_UpdateGradient(void)), this, SLOT(UpdateGradient()));
  connect(mayawidget, SIGNAL(_Render(bool)), this, SLOT(Render(bool)));
  connect(mayawidget, SIGNAL(_Tool(Vector, double)), this, SLOT(Tool(Vector, double)));
  connect(mayawidget, SIGNAL(_AutoReconstruct()), this, SLOT(AutoReconstruct()));
  connect(mayawidget, SIGNAL(_PickAltitude(double, int)), this, SLOT(ChangeAltitude(double, int)));

  // Record
  connect(ui.checkBox_record, SIGNAL(stateChanged(int)), this, SLOT(ToggleRecord(int)));

  // Undo (linked to CTRL+Z)
  connect(ui.undoAction, SIGNAL(clicked()), this, SLOT(UndoHistoryAction()));
  connect(mayawidget, SIGNAL(_Undo()), this, SLOT(UndoHistoryAction()));
  connect(mayawidget, SIGNAL(_ToggleConstraintsVisibility()), ui.actionConstraintsOverlay, SLOT(toggle()));

  // Interactive tools (the id of a button is the tool number used by the editor widget)
  QButtonGroup* buttongroup = new QButtonGroup(this);
  buttongroup->addButton(ui.Objects_Soften, 1);
  buttongroup->addButton(ui.Objects_Enhance, 2);
  buttongroup->addButton(ui.Objects_Clone, 5);
  buttongroup->addButton(ui.Objects_CloneZone, 6);
  buttongroup->addButton(ui.Objects_CloneLine, 7);
  buttongroup->addButton(ui.Objects_SlopeOrientation, 9);
  buttongroup->addButton(ui.Objects_FreezeTool, 13);
  buttongroup->addButton(ui.Objects_RiseConstraint, 14);
  buttongroup->addButton(ui.Objects_LowerConstraint, 15);
  buttongroup->addButton(ui.Objects_RemoveConstraintTool, 16);
  buttongroup->addButton(ui.Objects_AddConstraintTool, 17);
  buttongroup->addButton(ui.Objects_AddConstraintAndSlopeTool, 18);
  buttongroup->addButton(ui.Objects_AddSlopeTool, 19);
  buttongroup->addButton(ui.Objects_Mountain_2, 20);
  buttongroup->addButton(ui.Objects_SlopeTensorBrush, 21);
  buttongroup->addButton(ui.Objects_TensorNoise, 22);
  buttongroup->addButton(ui.Objects_PaintNoise, 23);
  connect(buttongroup, SIGNAL(idClicked(int)), this, SLOT(ToolChanged(int)));

  connect(ui.borderBlend_SpinBox, SIGNAL(valueChanged(int)), this, SLOT(BlendThresholdChange()));

  connect(ui.doubleSpinBox_altitude_constraint, SIGNAL(valueChanged(double)), mayawidget, SLOT(SetToolHeight(double)));
  connect(ui.doubleSpinBox_altitude_constraint_2, SIGNAL(valueChanged(double)), mayawidget, SLOT(SetToolHeight(double)));

  // Global tools
  connect(ui.Objects_FreezeRivers, SIGNAL(clicked()), this, SLOT(FreezeRiversCall()));
  connect(ui.Objects_FreezeRidges, SIGNAL(clicked()), this, SLOT(FreezeRidgesCall()));
  connect(ui.Objects_SyncGradient, SIGNAL(clicked()), this, SLOT(SyncGradientCall()));
  connect(ui.Objects_EraseAll, SIGNAL(clicked()), this, SLOT(ForgetConstraintsCall()));
  connect(ui.Objects_GPUErosion, SIGNAL(clicked()), this, SLOT(GPUErosionCall()));
  connect(ui.Objects_Soften_Global, SIGNAL(clicked()), this, SLOT(LowerAllCall()));
  connect(ui.Objects_Enhance_Global, SIGNAL(clicked()), this, SLOT(IncreaseAllCall()));
}

/*!
\brief Build the texture of the displayed terrain (relief shading, with the constraints darkened if the overlay is on)
*/
MayaMaterial QteWindow::RenderTexture()
{
  HeightField toRender = mayawidget->displayedHF;

  HeightFieldShader shader(toRender);
  texture = shader.Relief();

  if (ui.actionConstraintsOverlay->isChecked()) {
    for (int i = 0; i < toRender.GetSizeX(); i++) {
      for (int j = 0; j < toRender.GetSizeY(); j++) {
        if (mayawidget->gradientTerrain.alpha(i, j) < 0.5)
        {
          QColor color = texture.pixelColor(i, j);
          color = color.darker();
          texture.setPixelColor(i, j, color);
        }
      }
    }
  }

  MayaMaterial mm(ShaderTextureUV, Color(0.1, 0.1, 0.1, 1.0), Color(0.1, 0.1, 0.1, 1.0), Color(0.1, 0.1, 0.1, 1.0), 20.0, texture);
  mm.materialNode = MaterialNode::UVMapping;
  return mm;
}

/*!
\brief updates the tool value of the mayaWidget depending on the selected tool in the UI
*/
void QteWindow::ToolChanged(int currentTool)
{
  mayawidget->SetTool(currentTool);

  // Change Statusbar
  switch (currentTool)
  {
  case 1: ui.statusBar->showMessage(QString("Soften the terrain: Click on the terrain and move the mouse - Shift + Wheel to reduce or enlarge the tool")); break;
  case 2: ui.statusBar->showMessage(QString("Enhance: Click on the terrain and move the mouse - Shift + Wheel to reduce or enlarge the tool")); break;
  case 5: ui.statusBar->showMessage(QString("Clone: Ctrl-Click on the terrain to select the source point - Click and move the mouse to copy/paste - Shift + Wheel to reduce or enlarge the tool")); break;
  case 6: ui.statusBar->showMessage(QString("CloneZone: Ctrl-Click on the terrain to select the source zone - Click to paste the zone")); break;
  case 7: ui.statusBar->showMessage(QString("CloneLine: Clrl-Click on the terrain to select the source line - Click to paste the line on another line - Shift + Wheel to reduce or enlarge the tool")); break;
  case 9: ui.statusBar->showMessage(QString("SlopeOrientation: Ctrl-Click on the terrain to select the zone to apply - Click and move to draw the direction of the tensor to apply")); break;
  case 13: ui.statusBar->showMessage(QString("Freeze Tool: Click and move on the terrain to freeze the zone - Shift + Wheel to reduce or enlarge the tool")); break;
  case 14: ui.statusBar->showMessage(QString("Rise Constraint: Select the height offset in the altitude shift spinbox - Click and move on the terrain to rise the constraint - Shift + Wheel to reduce or enlarge the tool")); break;
  case 15: ui.statusBar->showMessage(QString("Lower Constraint: Select the height offset in the altitude shift spinbox - Click and move on the terrain to lower the constraint - Shift + Wheel to reduce or enlarge the tool")); break;
  case 16: ui.statusBar->showMessage(QString("Remove Constraint Tool: Click and move on the terrain to remove the constraint - Shift + Wheel to reduce or enlarge the tool")); break;
  case 17: ui.statusBar->showMessage(QString("Add Constraint Tool: Ctrl-Click to pick altitude from terrain - Click and move on the terrain to put an altitude constraint - Shift + Wheel to reduce or enlarge the tool")); break;
  case 18: ui.statusBar->showMessage(QString("Add Constraint And Slope Tool: Ctrl-Click to pick altitude from terrain - Click and move on the terrain to put altitude and slope constraint - Shift + Wheel to reduce or enlarge the tool")); break;
  case 19: ui.statusBar->showMessage(QString("Add Slope Tool: Click and move on the terrain to put a slope constraint - Shift + Wheel to reduce or enlarge the tool")); break;
  case 20: ui.statusBar->showMessage(QString("Mountain: Click and move on the terrain to draw a mountain - Shift + Wheel to reduce or enlarge the tool")); break;
  case 21: ui.statusBar->showMessage(QString("Slope Tensor Brush: Click and move on the terrain to apply the tensor in the direction of the stroke - Shift + Wheel to reduce or enlarge the tool")); break;
  case 22: ui.statusBar->showMessage(QString("Tensor Noise: Click and move on the terrain to add a noise in the direction of the stroke - Shift + Wheel to reduce or enlarge the tool")); break;
  case 23: ui.statusBar->showMessage(QString("Add Noise: Click and move on the terrain to add a noise - Shift + Wheel to reduce or enlarge the tool")); break;
  default: ui.statusBar->showMessage(QString(""));
  }
}

/*!
\brief updates the value of the blendTreshold in the mayawidget
*/
void QteWindow::BlendThresholdChange() {
  mayawidget->SetBlendTreshold(ui.borderBlend_SpinBox->value());
}

/*!
\brief Render again the terrain, the camera is unchanged
*/
void QteWindow::RenderCameraUnchanged()
{
  Render(false);
}

/*!
\brief calls the right method depending on the selected tool
\param position the position to apply the selected tool
\param rayon the radius of the area where the tool is applied to
*/
void QteWindow::Tool(Vector position, double rayon) {
  mayawidget->gradientTerrain.HistorySave();
  if (ui.Objects_Soften->isChecked())
  {
    mayawidget->gradientTerrain.SoftenLocalGradient(mayawidget->GetPointsToApplyTool(), rayon);
  }
  else if (ui.Objects_Enhance->isChecked())
  {
    mayawidget->gradientTerrain.EnhanceLocalGradient(mayawidget->GetPointsToApplyTool(), rayon);
  }
  else if (ui.Objects_Clone->isChecked())
  {
    mayawidget->gradientTerrain.CopyGradientLinearZone(mayawidget->GetPointsToApplyTool(), mayawidget->GetCloneSource(), mayawidget->GetStrokeBegin(), rayon);
  }
  else if (ui.Objects_CloneZone->isChecked())
  {
    if (mayawidget->GetLassoPolygon().size() >= 3) {
      mayawidget->gradientTerrain.CopyGradientFreeZone(position, mayawidget->GetLassoPolygon(), mayawidget->GetBlendThreshold());
    }
    else {
      return;
    }
  }
  else if (ui.Objects_CloneLine->isChecked())
  {
    if (mayawidget->GetSourceLinePoints().size() > 1 && mayawidget->GetDestLinePoints().size() > 1) {
      mayawidget->gradientTerrain.CopyGradientBrokenLine(mayawidget->GetSourceLinePoints(), mayawidget->GetDestLinePoints(), rayon);
    }
  }
  else if (ui.Objects_SlopeOrientation->isChecked())
  {
    if (mayawidget->GetLassoPolygon().size() >= 3) {
      mayawidget->gradientTerrain.SlopeOrientationEffect(mayawidget->GetLassoPolygon(), mayawidget->GetArrowDir(), ui.doubleSpinBox_Strength->value());
    }
  }
  else if (ui.Objects_FreezeTool->isChecked()) {
    mayawidget->gradientTerrain.FreezeTool(mayawidget->GetPointsToApplyTool(), rayon);
  }
  else if (ui.Objects_AddConstraintTool->isChecked()) {
    double slope = ui.doubleSpinBox_altitude_constraint_slope->value() / HFMAGN;
    if (!ui.checkBox_Slope->isChecked())
      slope = 0.0;
    double noise = ui.doubleSpinBox_altitude_constraint_noise->value() / HFMAGN;
    if (!ui.checkBox_Noise->isChecked())
      noise = 0.0;
    mayawidget->gradientTerrain.SetAltitude(mayawidget->GetPointsToApplyTool(), ui.doubleSpinBox_altitude_constraint->value() / HFMAGN, rayon, noise, slope);
  }
  else if (ui.Objects_AddConstraintAndSlopeTool->isChecked()) {
    double slope_right = tan(Math::DegreeToRadian(ui.doubleSpinBox_slope_right->value())) / HFMAGN;
    double slope_left = tan(Math::DegreeToRadian(ui.doubleSpinBox_slope_left->value())) / HFMAGN;
    mayawidget->gradientTerrain.SetAltitudeAndGradient(mayawidget->GetPointsToApplyTool(), ui.doubleSpinBox_altitude_constraint_2->value() / HFMAGN, rayon, slope_left, slope_right);
  }
  else if (ui.Objects_AddSlopeTool->isChecked()) {
    double slope_right = tan(Math::DegreeToRadian(ui.doubleSpinBox_slope_right->value())) / HFMAGN;
    double slope_left = tan(Math::DegreeToRadian(ui.doubleSpinBox_slope_left->value())) / HFMAGN;
    mayawidget->gradientTerrain.AddGradient(mayawidget->GetPointsToApplyTool(), rayon, slope_left, slope_right);
  }
  else if (ui.Objects_RiseConstraint->isChecked()) {
    mayawidget->gradientTerrain.MoveConstraint(mayawidget->GetPointsToApplyTool(), rayon, ui.doubleSpinBox_altitude_shift->value() / HFMAGN);
  }
  else if (ui.Objects_LowerConstraint->isChecked()) {
    mayawidget->gradientTerrain.MoveConstraint(mayawidget->GetPointsToApplyTool(), rayon, -ui.doubleSpinBox_altitude_shift->value() / HFMAGN);
  }
  else if (ui.Objects_RemoveConstraintTool->isChecked()) {
    mayawidget->gradientTerrain.RemoveConstraint(mayawidget->GetPointsToApplyTool(), rayon);
  }
  else if (ui.Objects_Mountain_2->isChecked()) {
    mayawidget->gradientTerrain.Mountain(mayawidget->GetPointsToApplyTool(), rayon, tan(Math::DegreeToRadian(ui.mountainSlope_doubleSpinBox->value())));
  }
  else if (ui.Objects_SlopeTensorBrush->isChecked()) {
    mayawidget->gradientTerrain.TensorBrush(mayawidget->GetPointsToApplyTool(), rayon, ui.doubleSpinBox_Strength->value());
  }
  else if (ui.Objects_TensorNoise->isChecked()) {
    mayawidget->gradientTerrain.TensorNoise(mayawidget->GetPointsToApplyTool(), rayon, ui.doubleSpinBox_NoiseAmplitude->value());
  }
  else if (ui.Objects_PaintNoise->isChecked()) {
    mayawidget->gradientTerrain.AddSomeNoize(mayawidget->GetPointsToApplyTool(), rayon, ui.doubleSpinBox_NoiseAmplitude_2->value());
  }

  UpdateUndoButton();
}

/*!
\brief Reconstruct the terrain after a tool has been applied, if the automatic reconstruction is on
*/
void QteWindow::AutoReconstruct() {
  if (ui.actionAutoReconstruct->isChecked())
    Reconstruct();
}

/*!
\brief Reconstruct the heightfield from the gradient and display it
*/
void QteWindow::Reconstruct(void) {
  UpdateGradient();
  Render(false);
}

void QteWindow::UpdateUndoButton() {
  ui.undoAction->setEnabled(mayawidget->gradientTerrain.HasHistory());
}

void QteWindow::UndoHistoryAction() {
  mayawidget->gradientTerrain.HistoryUndo();
  UpdateGradient();
  Render(false);
  UpdateUndoButton();
}

/*!
\brief Pick the altitude of the terrain into the altitude spinbox of the selected tool
*/
void QteWindow::ChangeAltitude(double altitude, int t) {
  if (t == 17) { // add altitude constraint
    ui.doubleSpinBox_altitude_constraint->setValue(altitude);
  }
  else if (t == 18) {
    ui.doubleSpinBox_altitude_constraint_2->setValue(altitude);
  }
}

/*!
\brief toggle record mode
*/
void QteWindow::ToggleRecord(int val) {
  mayawidget->gradientTerrain.ToggleRecord(val);
}
