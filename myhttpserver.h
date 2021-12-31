/*------------------header file myhttpserver.h --------------*/
#ifndef MYHTTPSERVER
#define MYHTTPSERVER
#include <QObject>
#include<QTcpSocket>
#include<utility>
#include<vector>
#include<iostream>

class HTTPmessage{
public:
   const QString& method() const { return fMethod;}
   const QString& header() const { return fHeader;}
   const QString& URI() const { return fURI;}
   const std::vector<std::pair<QString, QString> > & URITokens() const { return fURITokens;}
   const std::vector<std::pair<QString, QString> > & headerTokens() const { return fHeaderTokens;}
   const QString& data() const { return fData;}
   const QString & URIToken(const QString & key) const;
   const QString & headerToken(const QString & key) const;
   void clear();
   void parseMessage(const QString &);
   void Print(std::ostream & ost=std::cout) const;
   bool isNull() const { return  fHeader.isNull();}
private:
   static const QString gNullStr;
   void parseFirstLine(const QString &str);
   void parseHeaderLine(const QString &str);
   QString fMethod;
   QString fHeader; 
   QString fURI;
   std::vector<std::pair<QString, QString> > fURITokens;
   std::vector<std::pair<QString, QString> > fHeaderTokens;
   QString fData;
};
Q_DECLARE_METATYPE(HTTPmessage)
class QTcpServer;


class myHTTPserver : public QObject
{
   Q_OBJECT
public:
   explicit myHTTPserver(QObject *parent = 0);
   int serverPort() const;
   ~myHTTPserver();
   const HTTPmessage & lastMessage() const;
   const std::list<HTTPmessage> & messages() const{ return fMessages;}
public slots:
   void myConnection();
   void myError(QAbstractSocket::SocketError );
private:
   qint64 bytesAvailable() const;
   QTcpServer *server;
   QTcpSocket *socket ;
   const static HTTPmessage fNullMessage;
   std::list<HTTPmessage> fMessages;
signals:
   void messageReceived(const HTTPmessage&);
};
#endif
