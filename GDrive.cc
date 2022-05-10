#include"GDrive.h"
#include"myhttpserver.h"
#include<QWebEngineView>
#include <QWebEngineProfile>
#include<QThread>
#include<QNetworkRequest>
#include<QTimer>
#include<iostream>
#include<iomanip>
#include<QNetworkReply>
#include<QFile>
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>
#include<QIODevice>
#include<QFileInfo>
#include<QDir>
#include<QEventLoop>
#include"StrStr.h"
using namespace std;

GDrive::GDrive(QObject *parent): QObject(parent),fIntermediateOperation(false)
{
   fServer = new myHTTPserver;
   QThread * workerThread = new QThread;
   fServer->moveToThread(workerThread);
   workerThread->start();

   fDebugUrl=QString("http://localhost:%1/drive/apihuhu").arg(fServer->serverPort());

   fBrowser = new QWebEngineView;

   //   fBrowser->show();

   fNetManager = new QNetworkAccessManager(this);
   //   QTimer::singleShot(0, this, SLOT(Start()));

   string alnum;
   for(int i = '0'; i < '9'; i++) alnum += char(i);
   for(int i = 'A'; i < 'Z'; i++) alnum += char(i);
   for(int i = 'a'; i < 'z'; i++) alnum += char(i);
   cout << "Alnum:" << alnum << endl;
   string strcode;
   for(int i = 0; i < 30; i++) strcode += alnum[qrand() % alnum.size()];
   fStateCode = QString(strcode.c_str());
   strcode.clear();
   alnum += '-';
   alnum += '.';
   alnum += '_';
   alnum += '~';
   cout << "Alnum2:" << alnum << endl;
   for(int i = 0; i < 100; i++) strcode += alnum[qrand() % alnum.size()];
   fCodeVerifier = QString(strcode.c_str());
   cout << "codever " << fCodeVerifier.toStdString() << endl;

   connect(this, SIGNAL(accessTokenReadyForChecking()), this, SLOT(checkExpireTime()));
}

////////////////////////////////////////////////////////////////////////////////////////

void GDrive::Start()
{
   //   connect(this, SIGNAL(authorizationObtained()), this, SLOT(readDriveFiles()));
   authorize();
   return;
}

void GDrive::checkExpireTime()
{
   cout << "GDrive::checkExpireTime() " <<setprecision(12) << accessTokenExpireTime()
        << ' ' << QDateTime::currentDateTimeUtc().toTime_t()
	<<" Left: "<<accessTokenExpireTime() - (double)(QDateTime::currentDateTimeUtc().toTime_t()) << endl;
   if(accessTokenExpireTime() <= QDateTime::currentDateTimeUtc().toTime_t()) {
      requestNewAccessToken();
   } else emit(authorizationObtained());
}

void GDrive::authorize()
{
   if(accessToken().isEmpty()) {
      if(!readTokensFile()) {
         requestOAuthCode();
      } else emit(accessTokenReadyForChecking());
   } else emit(accessTokenReadyForChecking());
}

void  GDrive::query(const QString& query){
   fQuery=query;
   fQueriedFileId.clear();
   connect(this, SIGNAL(authorizationObtained()), this, SLOT(queryDriveFiles()));
   authorize();
   if(!fIntermediateOperation){
      QEventLoop loop;
      connect(this,SIGNAL(driveOperationFinished(const QVariant&)),&loop,SLOT(quit()));
      loop.exec();
   }
}

void  GDrive::read(const QString& filename){
   fIntermediateOperation=true;
   connect(this, SIGNAL(intermediateOperationFinished()), this, SLOT(readFileById()));
   ls(filename);//authorize
   QEventLoop loop;
   connect(this,SIGNAL(driveOperationFinished(const QVariant&)),&loop,SLOT(quit()));
   loop.exec();
}

void  GDrive::readFileById(){//authorize skipped
   disconnect(this, SIGNAL(intermediateOperationFinished()), this, SLOT(readFileById()));
   cout<<"GDrive::readFileById fQueriedFileId "<<fQueriedFileId<<endl;
   fIntermediateOperation=false;
   fOperationResult.clear();
   if(fQueriedFileId.isEmpty()) emit driveOperationFinished(fOperationResult);
   else {
      QNetworkRequest *req = new 
	 QNetworkRequest(QUrl("https://www.googleapis.com/drive/v3/files/"+fQueriedFileId+"?alt=media"+QString("&redirect_uri=http://127.0.0.1:%1").arg(fServer->serverPort()) ) );
      
      req->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
      req->setRawHeader(QByteArray("Authorization"), ("Bearer " + accessToken()).toLatin1() + "");
      
      //   req->setUrl(QUrl(fDebugUrl+fQuery));
      fNetManager->get(*req);
      connect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
	      this, SLOT(readReplyReceived(QNetworkReply  *)));
   }
}

void GDrive::readReplyReceived(QNetworkReply  *reply)
{
   cout<<"readReplyReceived"<<endl;
   disconnect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
              this, SLOT(readReplyReceived(QNetworkReply  *)));
   QByteArray arr=reply->readAll();
   //   cout<<QString(arr)<<endl;

   fOperationResult=arr;
   // if(fIntermediateOperation) emit intermediateOperationFinished(fOperationResult);
   // else
   emit driveOperationFinished(fOperationResult);
   cout << "GDrive::readReplyReceived" << endl;
}

void  GDrive::write(const QString& filename, const QByteArray &data){
   fUploadData=data;
   fUploadFileName=filename;
   fOperationResult.clear();
   connect(this, SIGNAL(authorizationObtained()), this, SLOT(writeDriveFiles()));
   authorize();
   QEventLoop loop;
   connect(this,SIGNAL(driveOperationFinished(const QVariant&)),&loop,SLOT(quit()));
   loop.exec();
}

void GDrive::writeDriveFiles(){
   disconnect(this, SIGNAL(authorizationObtained()), this, SLOT(writeDriveFiles()));
   fIntermediateOperation=true;
   connect(this, SIGNAL(intermediateOperationFinished()), this, SLOT(writeFileByIdOrNew()));
   ls(fUploadFileName);//authorize
}
void GDrive::writeFileByIdOrNew(){
   disconnect(this, SIGNAL(intermediateOperationFinished()), this, SLOT(writeFileByIdOrNew()));
   cout<<"GDrive::writeFileByIdOrNew fQueriedFileId "<<fQueriedFileId<<endl;
   fIntermediateOperation=false;

   if(fQueriedFileId.isEmpty()) uploadNewDriveFiles();//New File
   else updateDriveFileById();


}
void GDrive::updateDriveFileById(){
   fOperationResult.clear();
   QNetworkRequest *req = new 
      QNetworkRequest(QUrl("https://www.googleapis.com/upload/drive/v3/files/"+fQueriedFileId+
			   "?uploadType=media"));
   //   req->setUrl(QUrl(fDebugUrl+"/upload/drive/v3/files/"+fQueriedFileId));
   req->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
   req->setRawHeader(QByteArray("Authorization"), ("Bearer " + accessToken()).toLatin1() + "");
   req->setRawHeader(QByteArray("Content-Type"), QByteArray("text/plain"));

   fNetManager->sendCustomRequest(*req,"PATCH",fUploadData);
   
   connect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
           this, SLOT(updateReplyReceived(QNetworkReply  *)));
}
void GDrive::updateReplyReceived(QNetworkReply  *reply)
{
   cout << "Drive::updateReplyReceived" << endl;
   disconnect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
              this, SLOT(updateReplyReceived(QNetworkReply  *)));
   QByteArray arr=reply->readAll();
   cout<<QString(arr)<<endl;

   QJsonDocument doc = QJsonDocument::fromJson(arr);

   QJsonObject obj=doc.object();
   fQueriedFileId=obj.find("id").value().toString();

   fOperationResult=obj;
   //   if(fIntermediateOperation) emit intermediateOperationFinished();
   //   else
   emit driveOperationFinished(fOperationResult);
}


void GDrive::uploadNewDriveFiles(){
   cout << "GDrive::uploadNewDriveFiles()" << endl;

   QNetworkRequest *req = new 
      QNetworkRequest(QUrl("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"));
   //   req->setUrl(QUrl(fDebugUrl+"/upload/drive/v3?uploadType=media"));
   req->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
   req->setRawHeader(QByteArray("Authorization"), ("Bearer " + accessToken()).toLatin1() + "");
   req->setRawHeader(QByteArray("Content-Type"), QByteArray("multipart/related; boundary=foo_bar_baz"));

   QTextStream txt(new QString);
   txt<<"--foo_bar_baz\r\n";
   txt<<"Content-Type: application/json; charset=UTF-8\r\n";
   txt<<"\r\n";
   txt<<"{\r\n";
   txt<<"\"name\": \""<<fUploadFileName<<"\"\r\n";
   txt<<"}\r\n";
   txt<<"\r\n";
   txt<<"--foo_bar_baz\r\n";
   txt<<"Content-Type: text/plain\r\n";
   txt<<"\r\n";
   fUploadData=txt.readAll().toLatin1()+fUploadData;
   fUploadData+=QString("\r\n--foo_bar_baz--\r\n").toLatin1();
   //   txt<<"--foo_bar_baz--\r\n";

   fNetManager->post(*req,fUploadData);
   
   connect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
           this, SLOT(uploadNewReplyReceived(QNetworkReply  *)));
}
void GDrive::uploadNewReplyReceived(QNetworkReply  *reply)
{
   cout << "GDrive::uploadNewReplyReceived" << endl;
   disconnect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
              this, SLOT(uploadNewReplyReceived(QNetworkReply  *)));
   QByteArray arr=reply->readAll();
   cout<<QString(arr)<<endl;

   QJsonDocument doc = QJsonDocument::fromJson(arr);

   QJsonObject obj=doc.object();
   fQueriedFileId=obj.find("id").value().toString();

   fOperationResult=obj;
   //   if(fIntermediateOperation) emit intermediateOperationFinished();
   //   else
   emit driveOperationFinished(fOperationResult);
}


void GDrive::queryDriveFiles()
{
   disconnect(this, SIGNAL(authorizationObtained()), this, SLOT(queryDriveFiles()));
   cout << "GDrive::queryDriveFiles()" << endl;

   QNetworkRequest *req = new 
      QNetworkRequest(QUrl("https://www.googleapis.com/drive/v3/files"+fQuery));

   req->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
   req->setRawHeader(QByteArray("Authorization"), ("Bearer " + accessToken()).toLatin1() + "");

   //   req->setUrl(QUrl(fDebugUrl+fQuery));
   fNetManager->get(*req);
   connect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
           this, SLOT(queryReplyReceived(QNetworkReply  *)));
}

void GDrive::queryReplyReceived(QNetworkReply  *reply)
{
   disconnect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
              this, SLOT(queryReplyReceived(QNetworkReply  *)));
   cout<<"QRR "<<endl;
   auto cont = reply->readAll();
   QJsonDocument doc = QJsonDocument::fromJson(cont);
   cout<<"Doc: "<<QString(cont)<<endl;
   QJsonObject obj=doc.object();
   if(obj.find("files")!=obj.end()){
      QJsonValue val=obj.find("files").value();
      if(val.isArray()){
	 QJsonArray arr=val.toArray();
	 if(arr.size()>0){
	    QJsonValue val=arr.at(0);
	    if(val.isObject()){
	       QJsonObject obj=val.toObject();
	       if(obj.find("id")!=obj.end()){
		  fQueriedFileId=obj.find("id").value().toString();
	       }
	    }
	 }
      }
   }

   fOperationResult=obj;
   if(fIntermediateOperation) emit intermediateOperationFinished();
   else emit driveOperationFinished(fOperationResult);
   cout << "GDrive::queryReplyReceived" << endl;
}


///////////////// Authorization requests ////////////////////////////////////////////////

void GDrive::requestOAuthCode()
{
   QTextStream data(new QString);
   data << "https://accounts.google.com/o/oauth2/v2/auth?";
   data << "scope=https://www.googleapis.com/auth/drive&";
   data << "response_type=code&";
   data << QString("redirect_uri=http://127.0.0.1:%1&").arg(fServer->serverPort());
   data << "client_id=" << clientId() << '&';
   if(!email().isEmpty()) data << "login_hint="<<email()<<'&';
   data << "code_challenge_method=plain&";
   data << "code_challenge=" << fCodeVerifier << "&";
   data << "state=" << fStateCode;

   cout << "Url: " << data.string()->toStdString() << endl;
   cout << fStateCode.toStdString() << endl;
   connect(fServer, SIGNAL(messageReceived(const HTTPmessage&)), this,
           SLOT(OAuthCodeReceived(const HTTPmessage&)));
   fBrowser->load(QUrl(*data.string()));
   fBrowser->show();

}

void GDrive::OAuthCodeReceived(const HTTPmessage& message)
{
   disconnect(fServer, SIGNAL(messageReceived(const HTTPmessage&)),
              this, SLOT(OAuthCodeReceived(const HTTPmessage&)));
   cout << "GDrive::processGoogleOAuthResponse " << endl;
   message.Print();
   cout << "State " << message.URIToken("state").toStdString() << endl;
   if(message.URIToken("state") != fStateCode)
      cout << "Wrong State! " << fStateCode.toStdString() << "<->" << message.URIToken("state").toStdString() << endl;
   if(!message.URIToken("error").isNull()) {
      cout << "Auth Error " << message.URIToken("error").toStdString() << endl;
      return;
   }
   if(message.URIToken("code").isNull()) {
      cout << "No core returned from Auth!" << endl;
      return;
   }
   requestOAuthTokens(message.URIToken("code"));
}

void GDrive::requestOAuthTokens(const QString & code)
{
   QNetworkRequest *req = new QNetworkRequest(QUrl("https://www.googleapis.com/oauth2/v4/token"));

   req->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

   QTextStream data(new QString);
   data << "code=" << code << "&";
   data << "client_id=" << clientId() << '&';
   data << "client_secret=" << clientSecret() << '&';
   data << QString("redirect_uri=http://127.0.0.1:%1&").arg(fServer->serverPort());
   data << "grant_type=authorization_code&";
   data << "code_verifier=" << fCodeVerifier;

   cout << "Go for tokens! " << data.string()->toStdString() << endl;

   fNetManager->post(*req, data.string()->toLatin1());
   connect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
           this, SLOT(OAuthTokensReceived(QNetworkReply  *)));
}


void GDrive::OAuthTokensReceived(QNetworkReply  *reply)
{
   disconnect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
              this, SLOT(OAuthTokensReceived(QNetworkReply  *)));
   cout << "GDrive::tokensReceived " << endl;
   updateAccessRefreshTokens(*reply);
   updateAccessTokenExpireTime();
   writeTokensFile();
   emit(accessTokenReadyForChecking());
}

void GDrive::requestNewAccessToken()
{
   QNetworkRequest *req = new QNetworkRequest(QUrl("https://www.googleapis.com/oauth2/v4/token"));

   req->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

   QTextStream data(new QString);
   data << "client_id=" << clientId() << "&";
   data << "client_secret=" << clientSecret() << "&";
   data << "refresh_token=" << refreshToken() << "&";
   data << "grant_type=refresh_token";


   cout << "Go for ref tokens! " << data.string()->toStdString() << endl;

   fNetManager->post(*req, data.string()->toLatin1());
   connect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
           this, SLOT(newAccessTokenReceived(QNetworkReply  *)));
}

void GDrive::newAccessTokenReceived(QNetworkReply  *reply)
{
   disconnect(fNetManager, SIGNAL(finished(QNetworkReply  *)),
              this, SLOT(newAccessTokenReceived(QNetworkReply  *)));
   cout << "GDrive::newAccessTokenReceived " << endl;
   QByteArray ar = reply->readAll();
   QJsonDocument doc = QJsonDocument::fromJson(ar);
   cout << "ref: " << doc.object().find("access_token").value().toString().toStdString() << endl;
   setAccessToken(doc.object().find("access_token").value().toString());
   updateAccessTokenExpireTime();
   writeTokensFile();
   cout << QString(ar).toStdString() << endl;
   emit(accessTokenReadyForChecking());
}


///////////////// Manage JSON with tokens ////////////////////////////////////////////


QString GDrive::accessToken() const
{
   return fTokensJSON.find("access_token").value().toString();
}
QString GDrive::refreshToken() const
{
   return fTokensJSON.find("refresh_token").value().toString();
}
double GDrive::accessTokenExpireTime() const
{
   return fTokensJSON.find("expire_time_UTC").value().toDouble();
}

bool GDrive::updateAccessRefreshTokens(QIODevice & io)
{
   QByteArray ar = io.readAll();
   QJsonDocument doc = QJsonDocument::fromJson(ar);

   fTokensJSON = doc.object();

   cout << "GDrive::updateAccessRefreshTokens JSON: isObject=" << doc.isObject()
        << ", isArray=" << doc.isArray() << endl;
   for(int i = 0; i < fTokensJSON.keys().size(); i++) {
      cout << fTokensJSON.keys().at(i).toStdString() << endl;
      const QJsonValue  &val = fTokensJSON.find(fTokensJSON.keys().at(i)).value();
      cout << val.toString().toStdString()
           << ' ' << val.isDouble() << ' ' << val.isString() << ' ' << val.toDouble() << endl;
   }

   if(fTokensJSON.find("access_token") == fTokensJSON.end()) {
      cout << "GDrive::updateAccessRefreshTokens: No access_token key found in JSON " << endl;
      return false;
   }

   if(fTokensJSON.find("refresh_token") == fTokensJSON.end()) {
      cout << "GDrive::updateAccessRefreshTokens: No refresh_token key found in JSON " << endl;
      return false;
   }

   return true;
}

void GDrive::updateAccessTokenExpireTime()
{
   qint64 expTime = QDateTime::currentDateTimeUtc().toTime_t() + fTokensJSON.find("expires_in").
                    value().toDouble();
   if(fTokensJSON.find("expire_time_UTC") == fTokensJSON.end()) {
      cout << "Add expire time key " << endl;
      fTokensJSON.insert("expire_time_UTC", (qint64)expTime);
   } else  fTokensJSON.find("expire_time_UTC").value() = expTime;
   cout << "updated ExpTime =" << setprecision(16) << accessTokenExpireTime() << endl;
}

void GDrive::setAccessToken(const QString & tok)
{
   if(fTokensJSON.find("access_token") == fTokensJSON.end()) {
      cout << "Add access_token key " << endl;
      fTokensJSON.insert("access_token", tok);
   } else  fTokensJSON.find("access_token").value() = tok;
}

bool GDrive::readTokensFile()
{
   if(fTokenPath.isEmpty()) mkDefaultTokenPath();
   QFile file(fTokenPath);
   if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
   return updateAccessRefreshTokens(file);
}

bool GDrive::writeTokensFile()
{
   if(fTokenPath.isEmpty()) mkDefaultTokenPath();
   QFile file(fTokenPath);
   if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
   QJsonDocument doc;
   doc.setObject(fTokensJSON);
   file.write(doc.toJson());
   file.close();
   return true;
}

void GDrive::mkDefaultTokenPath(){
   QFileInfo info(QDir::homePath()+"/.credentials");
   cout<<"path0 "<<info.absoluteFilePath()<<endl;
   if(!info.exists()) QDir::home().mkdir(".credentials");
   cout<<"Test "<<info.isDir()<<' '<<info.isExecutable()<<' '<<info.isWritable()<<endl;
   if(!info.isDir() || !info.isExecutable() || !info.isWritable()) {
      info.setFile(QDir::homePath());
   }
   cout<<"path1 "<<info.absoluteFilePath()<<endl;
   info.setFile(info.absoluteFilePath()+"/.gdrive.json");
   cout<<"path2 "<<info.absoluteFilePath()<<endl;
   if(info.exists() && !(info.isReadable() && info.isWritable()))
      info.setFile(info.absoluteFilePath()+"_2");
   cout<<"path3 "<<info.absoluteFilePath()<<endl;
   fTokenPath=info.absoluteFilePath();
   if(!QFile::exists(fTokenPath)) {
      QFile file(fTokenPath);
      file.open(QIODevice::WriteOnly);
      file.close();
   }
   QFile::setPermissions(fTokenPath,QFileDevice::ReadOwner|QFileDevice::WriteOwner);
}

/////////////////////////////////////////////////////////////////////////////////

const QString GDrive::gClient_id = "688945109318-s0e8kdofnuvdpvnujnpkgje4molkf995.apps.googleusercontent.com";
const QString GDrive::gClient_secret = "QkYBLdMFU7hfhcjMm6xXqZBB";
QString GDrive::clientId() const { return gClient_id; }
QString GDrive::clientSecret() const { return gClient_secret; }


//QNetworkRequest *req=new QNetworkRequest(QUrl(QString("http://localhost:%1/drive/apihuhu").arg(fServer->serverPort())));
