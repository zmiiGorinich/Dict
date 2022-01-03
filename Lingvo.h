#ifndef Lingvo_h
#define Lingvo_h
#include"OnlineDict.h"
#include<QAbstractSocket>

class LingvoCardParser: public dictElementParser{
   Q_OBJECT

   void fetchVerbForms();
public:
   LingvoCardParser(OnlineDict * dict): dictElementParser(dict){}
   QString fDictName;
   ~LingvoCardParser(){}
   void parseElement(QString );
public slots:
   void replyFinished();
};

class Lingvo: public OnlineDict{
   Q_OBJECT

   QString fHTML;
   void parseHTML();
   bool fGotReply = false;
public:
   void fetch(QString queue);
public slots:
   void parserDone();
   
   void socketError(QAbstractSocket::SocketError socketError);
   void connected();
   void disconnected();
   void bytesWritten(qint64 bytes);
   void readyRead();

};

#endif
