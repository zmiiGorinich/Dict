#ifndef _Dictor_H
#define _Dictor_H
#include"TDict.h"
#include<QObject>
#include<string>
class GDrive;

class Dictor: public QObject{
   Q_OBJECT

   TDict fDict;
   TDict fDictRead;
   GDrive * fDrive;
   static Dictor * gInstance;
   Dictor();
public:
   static Dictor * get();
   static TDict & dict(){ return get()->fDict;}
   bool readGDrive();
   bool saveToGDrive();
   bool sshRead(TDict *dict=NULL);
   bool sshSave();
   void showWebPageForLabel(std::string tag);
signals:
   void updateStatusLine(const char*);
};
#endif
