#ifndef OnlineDict_h
#define OnlineDict_h
#include<QObject>
#include<QString>

#include"TDict.h"

class QNetworkAccessManager;
class QNetworkReply;
class TDictEntry;
class QWebElement;

class OnlineDict;
class dictElementParser: public QObject{
   Q_OBJECT
protected:
   bool fDone;
   bool fOK;
public:
   dictElementParser(OnlineDict *);
   TDictEntry   fEnt;
   virtual ~dictElementParser();
   virtual void parseElement(const QWebElement& ) = 0;
   bool isDone() const { return fDone;}
   bool ok() const { return fOK;}
   void setOK(bool ok){ fOK=ok;}
signals:
   void ready();
};

class OnlineDict: public QObject{
   Q_OBJECT
protected:
   static QNetworkAccessManager *manager ;
   QNetworkReply *fReply;
   std::list<dictElementParser*> fCF;
   QString fQuery;
   std::list<dictElementParser *>::iterator getMatchIterator();
   
public:
   OnlineDict();
   virtual ~OnlineDict();
   virtual void fetch(QString queue) = 0;
   TDictEntry *getMatch() ;
   std::list<TDictEntry *> getAll();
   static QNetworkAccessManager *networkManager();
public slots:
   virtual void replyFinished() = 0;
   virtual void parserDone();
signals:
   void ready(OnlineDict *);
   void error(OnlineDict *);
};

#endif
