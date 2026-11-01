#include "mayageometry.h"


#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
/*!
\brief Save the geometry as a Maya .obj file (MayaGeometryAll->MayaGeometry).
\param url Filename.
\param eraseTexture Boolean to confirm or not to erase the existing texture.
\return A boolean indicating whether the file has been successfully created or not.
*/
bool MayaGeometryAll::Save_OBJ(const QString& url, bool eraseTexture) const
{

  QFile data(url);

  if (!data.open(QFile::WriteOnly))
  {
    qDebug() << "OBJ File Not Found, url : " << url << "\n";
    return false;
  }
  QString basename;

  QTextStream out(&data);
  if (instances.size() > 0 && instances.begin()->GetMaterial().materialNode == MaterialNode::UVMapping) {
    QFileInfo fi(url);
    basename = fi.baseName();
    qDebug() << fi.baseName() << "\n";
    out << "mtllib " << basename << ".mtl" << QString('\n');
    out << "usemtl " << basename << QString('\n');
    // export the mtl file
    QFile matdata(fi.path() + "/" + fi.baseName() + ".mtl");
    if (!matdata.open(QFile::WriteOnly))
    {
      qDebug() << "Could not write " << url << "\n";
      return false;
    }
    QTextStream matout(&matdata);
    for (QMap<QString, MayaGeometrySet>::const_iterator i = instances.begin(); i != instances.end(); i++)
    {
      if (i->GetMaterial().materialNode == MaterialNode::UVMapping) {
        matout << "newmtl " << i->GetName() << QString('\n');
        matout << "Ns 96.078431" << QString('\n');
        matout << "Ka 1.000000 1.000000 1.000000" << QString('\n');
        matout << "Kd 0.640000 0.640000 0.640000" << QString('\n');
        matout << "Ks 0.500000 0.500000 0.500000" << QString('\n');
        matout << "Ke 0.000000 0.000000 0.000000" << QString('\n');
        matout << "Ni 1.000000" << QString('\n');
        matout << "d 1.000000" << QString('\n');
        matout << "illum 2" << QString('\n');

        // Texture Albedo
        QImage textureAlbedo = i->GetMaterial().texture.albedo;
        QString texfilename = fi.baseName() + i->GetName() + ".png";
        if (eraseTexture || !QFile::exists(fi.path() + "/" + texfilename))
          textureAlbedo.save(fi.path() + "/" + texfilename);
        matout << "map_Kd " << texfilename << QString('\n');

        // Texture Normal
        QImage textureNormal = i->GetMaterial().texture.normal;
        if (!textureNormal.isNull()) {
          QString normalfilename = fi.baseName() + i->GetName() + "_n.png";
          if (eraseTexture || !QFile::exists(fi.path() + "/" + normalfilename))
            textureNormal.save(fi.path() + "/" + normalfilename);
          matout << "map_norm " << normalfilename << QString('\n');
        }

        // Texture Opacity
        QImage textureOpacity = i->GetMaterial().texture.opacity;
        if (!textureOpacity.isNull()) {
          QString opacityfilename = fi.baseName() + i->GetName() + "_a.png";
          if (eraseTexture || !QFile::exists(fi.path() + "/" + opacityfilename))
            textureOpacity.save(fi.path() + "/" + opacityfilename);
          matout << "map_d " << opacityfilename << QString('\n');
        }
      }
    }
  }

  int decV = 1, decN = 1, decUV = 1;
  for (QMap<QString, MayaGeometrySet>::const_iterator i = instances.begin(); i != instances.end(); i++)
  {
    out << "g " << i->GetName() << "\n";
    if (i->GetMaterial().materialNode == MaterialNode::UVMapping) {
      out << "usemtl " << i->GetName() << QString('\n');
    }
    // Add the Geometry
    MayaGeometry mg = i->Collapse();
    mg.SetName(i->GetName());
    mg.Save_OBJ(out, decV, decN, decUV);

    // Needed for OBJ file
    decV += mg.size_vertex();
    decN += mg.size_normal();
    decUV += mg.size_UV();
  }

  out.flush();
  data.close();
  return true;
}
#include <QtCore/QProcess>
/*!
\brief Save the geometry as a Maya .obj file (QTextStream).
\param out QTextStream.
\param decV, decN, decUV Offsets for referencing indexes.
\return A boolean indicating whether the file has been successfully created or not.
*/
bool MayaGeometry::Save_OBJ(QTextStream& out, int decV, int decN, int decUV) const
{
  out << "name " << name << "\n";

  for (int i = 0; i < vertex.size(); i++)
  {
    out << "v " << vertex.at(i)[0] << " " << vertex.at(i)[1] << " " << vertex.at(i)[2] << QString('\n');
  }
  out.flush();

  for (int i = 0; i < normal.size(); i++)
  {
    out << "vn " << normal.at(i)[0] << " " << normal.at(i)[1] << " " << normal.at(i)[2] << QString('\n');
  }
  out.flush();

  for (int i = 0; i < UVmap.size(); i++)
  {
    out << "vt " << UVmap.at(i)[0] << " " << UVmap.at(i)[1] << QString('\n');
  }
  out.flush();

  for (int i = 0; i < indexes.size(); i += 3)
  {
    // Warning : indexes in .obj files start at 1, so increment
    if (UVmap.size() == 0)
      out << "f " << indexes.at(i).getIV() + decV << "//" << indexes.at(i).getIN() + decN << " " << indexes.at(i + 1).getIV() + decV << "//" << indexes.at(i + 1).getIN() + decN << " " << indexes.at(i + 2).getIV() + decV << "//" << indexes.at(i + 2).getIN() + decN << QString('\n');
    else
      out << "f " << indexes.at(i).getIV() + decV << "/" << indexes.at(i).getIUV() + decUV << "/" << indexes.at(i).getIN() + decN << " " << indexes.at(i + 1).getIV() + decV << "/" << indexes.at(i + 1).getIUV() + decUV << "/" << indexes.at(i + 1).getIN() + decN << " " << indexes.at(i + 2).getIV() + decV << "/" << indexes.at(i + 2).getIUV() + decUV << "/" << indexes.at(i + 2).getIN() + decN << QString('\n');
  }
  out.flush();

  return true;
}