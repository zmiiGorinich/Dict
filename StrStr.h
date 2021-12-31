#ifndef strstr_h
#define strstr_h
#include<string>
#include<iostream>
#include<QString>
inline std::string str(const QString & s){
   return s.toUtf8().constData();
}
inline std::ostream & operator<<(std::ostream &ost,const QString & s){
   return ost<<str(s);
}
inline QString qstr(const std::string &str){
   return QString::fromUtf8(str.c_str());
}
#endif
