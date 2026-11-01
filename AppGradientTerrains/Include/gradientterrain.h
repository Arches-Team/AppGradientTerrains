#pragma once 

#include "heightfield.h"
#include "vectorfield.h"

#define HFMAGN 400.0

class GradientTerrain: public VectorField2
{
protected:
	HeightField heightField;
	/* grid dimensions */
	unsigned int Nx=0, Ny=0;

	ScalarField2 lap;
	ScalarField2 border;
	ScalarField2 tmp;
	
	int record=0; // true if we are in a recording mode (export every new terrain)
	int recordid=0; // counter for the recording session
	QString recordPath;
	// undo history
	QVector<QVector<Vector2>> gradient_history;
	QVector<ScalarField2> alpha_history;
	QVector<ScalarField2> altitude_history;
	
	//real *laptab;

public: 
	ScalarField2 alpha;
	ScalarField2 alt;


	GradientTerrain();
	GradientTerrain(const VectorField2&);
	GradientTerrain(const QImage& image, const bool& isHeightfield=false);
	GradientTerrain(const QString&, const bool& isHeightfield = false);

	~GradientTerrain();

	HeightField GetHeightField() { return heightField; }

	void ExportStroke(const QVector<Vector2>& line, const double& radius, const QString & suffix = QString("stroke"));
	void ExportPositions(const QVector<Vector2>& line, const double& radius, const QString& suffix = QString("positions"));
	void ExportZone(const QPolygon & poly, const QString& suffix = QString("zone"));
	void ExportMap(const ScalarField2& map, const QString& suffix = QString("map"));


	Vector2 GradientAt(const int&, const int&);
	double HeightAt(const int&, const int&);

	void EnhanceLocalGradient(const QVector<Vector2>& positions, const double&);
	void SoftenLocalGradient(const QVector<Vector2>& positions, const double&);

	void SlopeOrientationEffect(const QPolygon&, const Vector2&, const double &);


	void HistorySave();
	void HistoryUndo();
	bool HasHistory() { return !gradient_history.isEmpty(); }

	void CopyGradientLinearZone(const QVector<Vector2>& positions, const Vector2&, const Vector2&, const double&);
	void CopyGradientFreeZone(const Vector2&, const QPolygon&, const int&);
	void CopyGradientBrokenLine(const QVector<Vector2>&, const QVector<Vector2>&, const int&);
	void SetAltitude(const QVector<Vector2>&, const double&, const double&, const double &noise = 0., const double& slope = 0.);
	void SetAltitudeAndGradient(const QVector<Vector2>&, const double&, const double&, const double& slopeleft = 0., const double& sloperight = 0.);
	void AddGradient(const QVector<Vector2>&, const double&, const double& slopeleft = 0., const double& sloperight = 0.);
	void ForgetConstraints(void);
	void SyncGradient(void);
	void Mountain(const QVector<Vector2>&, const double&, const double & slope = 0.7);
	void TensorBrush(const QVector<Vector2>& line, const double& radius, const double& strength = 0.5);
	void TensorNoise(const QVector<Vector2>& line, const double& radius, const double& amplitude = 20.);
	void AddSomeNoize(const QVector<Vector2>& line, const double& radius, const double& amplitude = 20.);


	void FreezeRivers(const double& riverWidth = 1.);
	void FreezeRivers(const QPolygon & zone, const double& riverWidth = 1.);
	void FreezeRidges();
	void FreezeRidges(const QPolygon& zone);
	void FreezeTool(const QVector<Vector2>&, const double& );
	void AddConstraint(const QVector<Vector2>& positions, const double& radius, const double& altitude);
	void MoveConstraint(const QVector<Vector2>& positions, const double& radius, const double& shift);
	void RemoveConstraint(const QVector<Vector2>& positions, const double& radius);

	void GPUErosion();


	void ToggleRecord(int val);
	

	//void UpdateHeightFieldOld();
	void UpdateHeightField();

};
