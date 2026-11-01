// System

#include <QtCore/QProcessEnvironment>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

#include "cpu.h"

#ifdef _MSC_VER

// Architecture
#include <windows.h>
#endif

bool System::avx = true;/*!
Get the Advanced Vector Extensions flag.
\param a Boolean, set to true to run AVX accelerated algorithms.
*/
bool System::Avx()
{
  return avx;
}

/*!
\brief Get the environment variable with Qt.
\param name Name.
*/
QString System::GetEnv(const QString& name)
{
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  return env.value(name);
}/*!
\brief Get the resource file name given an ENV variable and the subfolder/name of the file.

Searches also in the current folder, by path and then by file name only, if the file is not found in the folder given by the env variable.
\param env Name of the environment variable.
\param name Name of the file.
\throw std::runtime_error if the resource has not been found.
*/

QString System::GetResource(const QString& env, const QString& name)
{
  QString envvalue = System::GetEnv(env);

  QFileInfo check_file_env(envvalue + name);

  if (!check_file_env.exists()) {
    // The env variable is not defined, we will try to find the file in the current directory
    //envvalue = ".";
    if (name.isEmpty())
      return envvalue; // a precise filename has not been asked, we just fall back to the current directory

    //QString fullPath = envvalue + name;
    QString fullPath = name;
    QFileInfo check_file(fullPath);
    if (check_file.exists()) {
      return fullPath;
    }
    else {  // if file is still not found, remove the path and search only the filename in the current directory
      QString justName = QFileInfo(name).fileName();
      QFileInfo check_file2(justName);
      if (check_file2.exists()) {
        return justName;
      }
      else {
		  throw std::runtime_error("Resource not found: " + justName.toStdString());
      }

    }
  }
  else {
    return envvalue + name;
  }
  return QString();
}/*
\brief Format an integer with spaces.
\param n Integer.
*/
QString System::LongInteger(int n)
{
  if (n < 1000)
  {
    return QString("%1").arg(n);
  }
  else if (n < 1000000)
  {
    return QString("%1.%2").arg(n / 1000).arg(n % 1000, 3, 10, QChar('0'));
  }
  else if (n < 1000000000)
  {
    return QString("%1.%2.%3").arg(n / 1000000).arg((n / 1000) % 1000, 3, 10, QChar('0')).arg(n % 1000, 3, 10, QChar('0'));
  }
  else
  {
    return QString("%1.%2.%3.%4").arg(n / 1000000000).arg((n / 1000000) % 1000, 3, 10, QChar('0')).arg((n / 1000) % 1000, 3, 10, QChar('0')).arg(n % 1000, 3, 10, QChar('0'));
  }
}/*
\brief Set s string according to date and time.

This function is useful for labeling screenshots.
\param n Integer.
*/
QString System::DateTime()
{
  // Date and time
  QDate date = QDate::currentDate();
  QTime time = QTime::currentTime();

  QString s = QString("%1%2%3-%4%5%6")
    .arg(date.year(), 4)
    .arg(date.month(), 2, 10, QChar('0'))
    .arg(date.day(), 2, 10, QChar('0'))
    .arg(time.hour(), 2, 10, QChar('0'))
    .arg(time.minute(), 2, 10, QChar('0'))
    .arg(time.second(), 2, 10, QChar('0'));
  return s;
}