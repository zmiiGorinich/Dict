#ifndef Promt_h
#define Promt_h
#include"OnlineDict.h"

class cformParser: public dictElementParser{
    Q_OBJECT
 
public:
   cformParser(OnlineDict * dict): dictElementParser(dict){}
   ~cformParser(){}
   void parseElement(const QWebElement& );
};

class Promt: public OnlineDict{
   Q_OBJECT

   QString fHTML;
   void getNext(QString resCode);
   void parseHTML();
public:
   void fetch(QString queue);
public slots:
   void replyFinished();
};

#endif
