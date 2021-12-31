#include "myhttpserver.h"
#include<iostream>
#include <QTcpServer>
#include<QStringList>
#include <stdlib.h>
using namespace std;

void HTTPmessage::clear(){
   fMethod.clear();
   fHeader.clear();
   fURI.clear();
   fData.clear();
   fURITokens.clear();
   fHeaderTokens.clear();
}

const QString & HTTPmessage::URIToken(const QString & key) const{
   for(auto &p: fURITokens) if(p.first==key) return p.second;
   return gNullStr;
}

const QString & HTTPmessage::headerToken(const QString & key) const{
   for(auto &p: fHeaderTokens) if(p.first==key) return p.second;
   return gNullStr;
}

void HTTPmessage::parseFirstLine(const QString &str){
   QStringList lstsp=str.split(' ');
   fMethod=lstsp.at(0);
   fURI=lstsp.at(1);
   if(fURI.startsWith("/?")){
      QStringList lstamp=fURI.mid(2).split('&');
      for(int i=0; i<lstamp.size(); i++){
	 QStringList lsteq=lstamp.at(i).split('=');
	 if(lsteq.size()>0) fURITokens.push_back(pair<QString,QString> (lsteq.at(0),""));
	 if(lsteq.size()>1) fURITokens.back().second=lsteq.at(1);
      }
   }
}
void HTTPmessage::parseHeaderLine(const QString &str){
   int ind=str.indexOf(':');
   if(ind>=0){
      fHeaderTokens.push_back(pair<QString,QString> (str.mid(0,ind).trimmed(),""));
      fHeaderTokens.back().second=str.mid(ind+1).trimmed();
   }
}
void HTTPmessage::parseMessage(const QString & instr) {
   clear();
   QTextStream stream(const_cast<QString*>(&instr),QIODevice::ReadOnly);
   while(!stream.atEnd()){
      QString line=stream.readLine();
      if(line.isEmpty()){
	 while(!stream.atEnd()){
	    QString line=stream.readLine();
	    fData+=line+"\n";
	 }
      } else {
	 if(fHeader.isEmpty()) parseFirstLine(line);
	 else parseHeaderLine(line);
	 fHeader+=line+"\n";
      }
   }
}

void HTTPmessage::Print(std::ostream & ost) const{
   ost<<"-------Method: "<<fMethod.toStdString()<<endl;
   ost<<"-------URI: "<<fURI.toStdString()<<endl;
   ost<<"------ Header -----------"<<endl
      <<fHeader.toStdString()
      <<"------ Data -----------"<<endl
      <<fData.toStdString()
      <<"----------------------"<<endl;
      if(!fURITokens.empty()){
	 for(auto &p :fURITokens) ost<<"URIToken: "<<p.first.toStdString()<<"==="<<p.second.toStdString()<<endl;
      }
      if(!fHeaderTokens.empty()){
	 for(auto &p :fHeaderTokens) ost<<"HeaderToken: "<<p.first.toStdString()<<"==="<<p.second.toStdString()<<endl;
      }
      ost<<endl;
}

myHTTPserver::myHTTPserver(QObject *parent) : QObject(parent)
{
   server = new QTcpServer(this);
   // waiting for the web brower to make contact,this will emit signal
   connect(server, SIGNAL(acceptError(QAbstractSocket::SocketError)), 
	   this, SLOT(myError(QAbstractSocket::SocketError)));
   connect(server, SIGNAL(newConnection()), this, SLOT(myConnection()));
   if(server->listen(QHostAddress::Any, 0)){
      cout <<endl
	   <<"Web server is waiting for a connection on port "
	   <<QString::number(server->serverPort()).toStdString()
	   <<endl;
   } else {
      cout <<endl
	   << "Web server could not start"
	   <<endl;
   }
}
void myHTTPserver::myError(QAbstractSocket::SocketError e){
   cout<<"SERVERERR "<<e<<endl;
}
int myHTTPserver::serverPort() const{
   return server->serverPort();
}
void myHTTPserver::myConnection()
{
   socket = server->nextPendingConnection();
   int contlen=0;
   bool is_cont=false;
   QString message;
   while( !(is_cont&&contlen==0) ){
      bool ok=socket->waitForReadyRead(10000);  //waiting for data to be read from web browser
      if(!ok){
	 cout<<"Incomplete data on socket "<<endl;
	 break;
      }
      while(socket->canReadLine()){
	 QByteArray l=socket->readLine();
	 message+=l;
	 for(int i = 0; i < l.size(); i++) cout << l[i];
	 if(!is_cont){
	    QString str(l);
	    if(str.contains("Content-Length:")) {
	       QStringList lst=str.split(":");
	       if(lst.size()==2) {
		  contlen=lst.at(1).toInt();
		  //		  cout<<"Found Len= "<<contlen<<endl;
	       }
	    }
	    is_cont = (str=="\r\n");
	 } else contlen-=l.size();
      }
   }
   //   cout<<"AllDone"<<endl;
   if(!message.isEmpty()){
      fMessages.push_front(HTTPmessage());
      fMessages.front().parseMessage(message);
      emit messageReceived(lastMessage());
   }

   socket->write("HTTP/1.1 200 OK\r\n");       // \r needs to be before \n
   socket->write("Connection: close\r\n\r\n");
   socket->flush();
   connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
        socket->disconnectFromHost();
}
const HTTPmessage & myHTTPserver::lastMessage() const{
   if(!fMessages.empty()) return fMessages.front();
   return fNullMessage;
}

myHTTPserver::~myHTTPserver()
{
   socket->close();
}
const QString HTTPmessage::gNullStr=QString();
const HTTPmessage myHTTPserver::fNullMessage=HTTPmessage();
