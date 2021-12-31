#ifndef Lingvo_h
#define Lingvo_h
#include"OnlineDict.h"


class gcardParser: public dictElementParser{
   Q_OBJECT

   QNetworkReply *fReply;
   void fetchVerbForms();
public:
   gcardParser(OnlineDict * dict): dictElementParser(dict){}
   QString fDictName;
   ~gcardParser(){}
   void parseElement(const QWebElement& );
public slots:
   void replyFinished();
};

class Lingvo: public OnlineDict{
   Q_OBJECT

   QString fHTML;
   void parseHTML();
public:
   void fetch(QString queue);
public slots:
   void replyFinished();
   void parserDone();

};

#endif
