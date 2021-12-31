#ifndef GDRIVE_H
#define GDRIVE_H
#include<QObject>
#include<QJsonObject>
#include<QVariant>
class QWebEngineView;
class myHTTPserver;
class QNetworkAccessManager;
class HTTPmessage;
class QNetworkReply;
class QIODevice;
class GDrive: public QObject
{
   Q_OBJECT
public:
   GDrive(QObject *parent = 0);
   const QVariant & operationResult(){ return fOperationResult;}
   void  ls(const QString& filename) { query("?q=name = '"+ filename+"'");}
   void  query(const QString& query);
   void  read(const QString& filename);
   void  write(const QString& filename, const QByteArray &data);
   const QString& queriedFileId() const { return fQueriedFileId;}
   const QString& email() const {return fEmail;}
   void setEmail(const QString& s){ fEmail=s;}
   const QString& tokenPath() const {return fTokenPath;}
   void setTokenPath(const QString& s){ fTokenPath=s;}
protected:
   QNetworkAccessManager * fNetManager;
   QWebEngineView * fBrowser;
   myHTTPserver   * fServer;
   QString fStateCode;
   QString fCodeVerifier;
   void requestOAuthCode();
   void requestOAuthTokens(const QString & code);
   void requestNewAccessToken();

   bool updateAccessRefreshTokens(QIODevice & io);
   void updateAccessTokenExpireTime();
   bool readTokensFile();
   bool writeTokensFile();

   QJsonObject fTokensJSON;
   QString accessToken() const;
   QString refreshToken() const;
   double  accessTokenExpireTime() const;
   void    setAccessToken(const QString &);
   static const QString gClient_id;
   static const QString gClient_secret;
   QString clientId() const;
   QString clientSecret() const;

   QString  fQuery;
   QVariant fOperationResult;
   QString fDebugUrl;
   QString fQueriedFileId;
   bool fIntermediateOperation;		
   QByteArray fUploadData;
   QString fUploadFileName;

   void updateDriveFileById();
   void uploadNewDriveFiles();

   QString fEmail;
   QString fTokenPath;
   void mkDefaultTokenPath();
private slots:
   void Start();
   void OAuthCodeReceived(const HTTPmessage&);
   void OAuthTokensReceived(QNetworkReply  *reply);
   void newAccessTokenReceived(QNetworkReply  *reply);
   void queryReplyReceived(QNetworkReply  *reply);
   void readReplyReceived(QNetworkReply  *reply);
   void updateReplyReceived(QNetworkReply  *reply);
   void uploadNewReplyReceived(QNetworkReply  *reply);

   void checkExpireTime();
   void authorize();
   void queryDriveFiles();
   void readFileById();//authorize skipped
   void writeDriveFiles();
   void writeFileByIdOrNew();


signals:
   void accessTokenReadyForChecking();
   void authorizationObtained();
   void driveOperationFinished(const QVariant &);
   void intermediateOperationFinished();
};

#endif
