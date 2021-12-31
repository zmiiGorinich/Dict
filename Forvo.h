#ifndef forvo_h
#define forvo_h
#include<QObject>
#include<QEvent>

class QWebPage;
class TForvo: public QObject{
   Q_OBJECT

   QString fStr;
   QWebPage *wpage;
   enum Action {eNone=0, eSay=1, eShow=2};
   Action fAction;
   void start();
   void doShow();
   void doSay();
public:

   TForvo();
public slots:
   void say(const QString &);
   void show(const QString &);
   void pageLoaded(bool);
   void pageStarted();
   void pageProgress(int i);
 };
#endif
