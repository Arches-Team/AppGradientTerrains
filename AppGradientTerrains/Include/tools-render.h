#pragma once

#include "maya.h"
#include "plane.h"

class ToolsRender {
protected:
  //! A tool overlay: its data, visibility and GPU renderer.
  struct Tool {
    QVector<Vector> data;
    bool visible = false;
    bool dirty = false;   //!< Data changed, renderer must be rebuilt (in a valid GL context).
    MayaSimpleRenderer* renderer = nullptr;
    Color color;
    GLenum mode;
    double lineWidth;

    Tool(const Color& c, GLenum m, double w) : color(c), mode(m), lineWidth(w) {}
    void SetData(const QVector<Vector>& d) { data = d; dirty = true; }
    void Release();
    void Render(const Camera& c, int width, int height);
  };

  Tool lasso;
  Tool lassoCpy;
  Tool line;
  Tool lineCpy;
  Tool circle;
  Tool circleCpy;
  Tool border;

public:
  ToolsRender();
  ~ToolsRender();
  ToolsRender(const ToolsRender&) = delete;
  ToolsRender& operator=(const ToolsRender&) = delete;

  void Release();
  void Render(const Camera& c, int width, int height);
  void DataLasso(const QVector<Vector>& d) { lasso.SetData(d); }
  void DataLassoCopy(const QVector<Vector>& d) { lassoCpy.SetData(d); }
  void DataLine(const QVector<Vector>& d) { line.SetData(d); }
  void DataLineCopy(const QVector<Vector>& d) { lineCpy.SetData(d); }
  void DataCircle(const QVector<Vector>& d) { circle.SetData(d); }
  void DataCircleCopy(const QVector<Vector>& d) { circleCpy.SetData(d); }
  void DataBorder(const QVector<Vector>& d) { border.SetData(d); }

  void setLasso(bool b) { lasso.visible = b; }
  void setLassoCpy(bool b) { lassoCpy.visible = b; }
  void setLine(bool b) { line.visible = b; }
  void setLineCpy(bool b) { lineCpy.visible = b; }
  void setCircle(bool b) { circle.visible = b; }
  void setCircleCpy(bool b) { circleCpy.visible = b; }
  void setBorder(bool b) { border.visible = b; }

};
