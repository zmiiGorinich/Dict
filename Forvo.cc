#include"Forvo.h"
#include"StrStr.h"
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QtWebKitWidgets/QWebPage>
#include<fstream>
//#include<QFile>
//#include <QTextStream>
#include<QtWebKitWidgets/QWebView>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
using namespace std;
TForvo::TForvo():wpage(0), fAction(eNone){
}
void TForvo::say(const QString &s){
   cout<<"Say "<<s<<endl;
   fAction=(Action)(fAction|eSay);
   fStr=s;
   start();
}
void TForvo::show(const QString &s){
   cout<<"Show "<<s<<endl;
   fAction=(Action)(fAction|eShow);
   fStr=s;
   start();
}
//QWebView * view=NULL;
QWebEngineView *view =NULL;
  
void TForvo::start(){
   if(!view) view=new QWebEngineView;
   //   view->settings()->setAttribute(QWebSettings::AutoLoadImages,false);
   //   view->settings()->setAttribute(QWebSettings::JavascriptEnabled,false);
   //   view->settings()->setAttribute(QWebSettings::JavaEnabled,false);
   //   view->settings()->setAttribute(QWebSettings::PluginsEnabled,false);
   // view->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,false);
   // view->settings()->setAttribute(QWebSettings::JavascriptCanCloseWindows,false);
   // view->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard,false);
   // view->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled,false);
   //   view->settings()->setAttribute(QWebSettings::WebGLEnabled,false);
   //   if(!wpage) wpage=new QWebPage;
   cout<<"Sta0 "<<endl;
   //   wpage->mainFrame()->setUrl(QUrl("http://www.forvo.com/word/"+fStr));
   //   view->setUrl(QUrl("http://www.forvo.com/word/"+fStr));
   cout<<"Sta1"<<endl;
   connect(view,SIGNAL(loadFinished(bool)),this,SLOT(pageLoaded(bool)));
   connect(view,SIGNAL(loadStarted()),this,SLOT(pageStarted()));
   connect(view,SIGNAL(loadProgress(int)),this,SLOT(pageProgress(int)));
   cout<<"Sta2"<<endl;
   view->load(QUrl("http://www.forvo.com/word/"+fStr));
   //   view->load(QUrl("http://doc.qt.io/archives/qt-5.5/qwebview.html"));
   //   view->load(QUrl("http://bash.im"));
   cout<<"Sta3"<<endl;
}
void TForvo::doShow(){
   // QWebView * view=new QWebView;
   // view->setPage(wpage);
   view->show();
}
void TForvo::doSay(){
   //   wpage=view->page();
   
   QWebElement doc = wpage->mainFrame()->documentElement();
   QWebElement ch=doc.firstChild();

   QWebElement main = doc.findFirst("div#main");
   //   cout<<"NULL "<<doc.isNull()<<' '<<main.isNull()<<endl;
   QWebElementCollection words = main.findAll("div.word");
   for(int iw=0; iw<words.count(); iw++){
      QWebElement lang=words[iw].findFirst("span.lang_xx abbr[title]");
      cout<<"Lang "<<lang.attribute("title")<<endl;
      if(lang.attribute("title")=="Deutsch"){
	 QWebElement ul=words[iw].findFirst("ul");
	 QWebElementCollection li=ul.findAll("li");
	 for(int il=0; il<li.count(); il++){
	    QWebElement a=li[il].findFirst("a[onclick]");
	    cout<<"Click: "<<a.attribute("onclick")<<endl;
	    //	 a.evaluateJavaScript("this.click()");
	    QWebElement img=a.findFirst("img");
	    cout<<img.toOuterXml()<<endl;
	    // QString cmd=a.attribute("onclick").section(";",0,0);
	    // wpage->mainFrame()->evaluateJavaScript(cmd);
	    //	    wpage->mainFrame()->evaluateJavaScript(a.attribute("onclick"));
	    QString ret=img.evaluateJavaScript("this.click()").toString();
	    cout<<"Forvo Return "<<ret<<endl;
	    break;
	 }
      }
   }

}
void TForvo::pageStarted(){
   cout<<"PageStarted "<<endl;
}
void TForvo::pageProgress(int i){
   cout<<"PageProgress "<<i<<endl;
}
void TForvo::pageLoaded(bool){
   cout<<"LLLLLLLLLLLLLLLLLLLLLLLL "<<endl;
   // ofstream ForvoPage("ForvoPage.html");
   // wpage=view->page();
   // ForvoPage<<wpage->mainFrame()->toHtml()<<endl;
   // if(fAction&eShow) doShow();
   // if(fAction&eSay) doSay(); 
   doShow();
   fAction=eNone;
   //   disconnect(wpage->mainFrame(),SIGNAL(loadFinished(bool)),this,SLOT(pageLoaded(bool)));
   //   disconnect(view,SIGNAL(loadFinished(bool)),this,SLOT(pageLoaded(bool)));

}

