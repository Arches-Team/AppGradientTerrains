// Cpu  

#pragma once

#include <QtCore/QString>
#include <QtCore/QElapsedTimer>


class System
{
protected:
  static bool avx; //!< Advanced Vector Extensions flag.
public:
#ifdef _MSC_VER
  #endif
  static QString GetEnv(const QString&);
        static QString GetResource(const QString&, const QString & = QString(""));
      
        static QString LongInteger(int);
    static QString DateTime();

        
    
      
  
  
  
    static bool Avx();
protected:
    };
