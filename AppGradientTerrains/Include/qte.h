#pragma once

#include "maya.h"


#include "uic_mainWindowProjetQT.h" 
#include "gradientterrain.h"

#include "tools-render.h"


class MayaWidgetTerrainEditor : public MayaWidget
{
  Q_OBJECT
protected:
  double _radius;
  double _height;

  Vector clonesource;	//sourcePoint for the area to clone
  Vector strokebegin;	//StartPoint of a click and drag Stroke
  bool cloneok;			//if a sourcePoint has been selected or not

  bool beginstroke;		//if we are click and dragging
  int tool;				//the number of the tool that is currently selected in the ui

  bool modified;

  Vector2 releasePoint;
  MayaGpuSet direction;
  bool directionok;

  double tool_rayon = 50.0;	// Radius of the tool (modified with MAJ+Wheel)
  void UpdateAnchor(QMouseEvent*);

  bool lassoOk;				//if an area has been selected with the lasso
  bool newlassoDraw;		//if we are drawing a newZone with the lasso (CTR+leftclick) to replace the old one and redraw it
  QVector<Vector> lassoPoints; // To store the points when drawing the perimeter of the selected zone
  QPolygon lassoPolygon;	//the zone that is selected with the lasso to potentially clone
  int blendThreshold;

  //storage and display of the sourceLine to clone
  bool lineOk;
  bool newLineDraw;
  QVector<Vector2> linePoints2;
  QVector<Vector> linePoints3;

  //storage of the destLine to clone to
  bool newDestLineDraw;
  QVector<Vector2> lineDestPoints2;

  //storage of clic and drag point to apply the tool to
  QVector<Vector2> clicDragPoints;

  Vector arrowDir;

  QVector<Vector> circlePoints;
  ToolsRender tools;
  double tool_height;
  unsigned int Nx, Ny;

public:
  MayaWidgetTerrainEditor();
  ~MayaWidgetTerrainEditor();
  void IncreaseToolSize(void) { tool_rayon *= 1.3; }
  void DecreaseToolSize(void) { tool_rayon /= 1.3; }
  void InitCamera();
  void SetTool(int number) {
    tool = number; lineDestPoints2.clear(); clicDragPoints.clear(); lineOk = false; newLineDraw = false; cloneok = false;  newDestLineDraw = false; lassoPoints.clear(); updateToolAnchor(); update();
  }
  void SetBlendTreshold(int number) { blendThreshold = number; }
  int GetBlendThreshold() { return blendThreshold; }
  Vector2 GetReleasePoint() { return releasePoint; }
  Vector GetArrowDir() { return arrowDir; }
  int GetTool(void) { return tool; }
  Vector GetCloneSource(void) { return clonesource; }
  Vector GetStrokeBegin(void) { return strokebegin; }
  QPolygon GetLassoPolygon(void) { return lassoPolygon; }
  QVector<Vector2> GetSourceLinePoints(void) { return linePoints2; }
  QVector<Vector2> GetDestLinePoints(void) { return lineDestPoints2; }
  QVector<Vector2> GetPointsToApplyTool(void) { return clicDragPoints; }
  double GetRadius(void) { return tool_rayon; }

  Camera* GetCameraPtr(void) { return &camera; }

  void updateToolAnchor();

  GradientTerrain gradientTerrain;

  HeightField displayedHF;
  bool editable;


  ScalarField2 lap;
  ScalarField2 border;
  ScalarField2 tmp;

public slots:
  virtual void mousePressEvent(QMouseEvent*);       // Action souris Clic Press
  virtual void mouseDoubleClickEvent(QMouseEvent*); // Action souris Double Clic
  virtual void mouseMoveEvent(QMouseEvent*);        // Action souris Clic + Deplacement
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent*);   // Action souris Clic Milieu
  virtual void keyPressEvent(QKeyEvent*);


  void paintGL(void);
  void SetToolHeight(double h) { tool_height = h / HFMAGN; }

signals:
  void _signalEditSceneLeft(const Ray&);         // Signal Edition Left Clic 
  void _signalEditSceneRight(const Ray&);        // Signal Edition Right Clic 
  void _UpdateGradient(void);                    // reconstruire le terrain à partir du gradient
  void _Render(bool);                            // reconstruire le terrain à partir du gradient
  void _Tool(Vector, double);                     // apply tool
  void _PickAltitude(double, int);                // pick the altitude from the terrain using a given tool
  void _AutoReconstruct();
  void _Undo();
  void _ToggleConstraintsVisibility();
};

class QteWindow : public QMainWindow
{
  Q_OBJECT
protected:

private:
  Ui::Assets ui; //!< Interface.
  MayaWidgetTerrainEditor* mayawidget; //!< Terrain editor widget.
  QImage texture; //!< Texture of the terrain.

  MayaGeometryAll world; //! A single geometry representing the whole scene.
public:
  QteWindow();
  ~QteWindow();

  MayaMaterial RenderTexture();

  // Slots
public slots:
  void ExportObj();
  void Save8();
  void Save32();
  void ExportCurrentHeightFieldToPng();

  void LoadGradientCall();
  void LoadHeightfieldCall();
  void InitialTerrain(void);   //!< Generate the default terrain once the GL widget is ready.
  void FBMCall(void);
  void FBMCall_1025(void);

  // Global tools
  void FreezeRiversCall(void);
  void FreezeRidgesCall(void);
  void ForgetConstraintsCall(void);
  void SyncGradientCall(void);
  void LowerAllCall(void);
  void IncreaseAllCall(void);
  void GPUErosionCall(void);

  void UpdateGradient();
  void Render(bool camera = true);

  void RenderCameraUnchanged();

  // Interactive tools
  void Tool(Vector, double);
  void ToolChanged(int);
  void BlendThresholdChange();
  void ChangeAltitude(double, int);

  void AutoReconstruct();
  void Reconstruct();

  void ToggleRecord(int);
  void UndoHistoryAction();
  void UpdateUndoButton();

private:
  void createActions();
};
