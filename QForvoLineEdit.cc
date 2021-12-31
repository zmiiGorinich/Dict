#include"QForvoLineEdit.h"
#include<iostream>
#include"StrStr.h"
#include<QtWidgets/QMenu>
#include <QtGui/QContextMenuEvent>
#include"Forvo.h"

using namespace std;
void QForvoLineEdit::setForvo(){
   fForvo=new TForvo;
   connect(this,SIGNAL(forvoSay(const QString &)), fForvo, SLOT(say(const QString &)));
   connect(this,SIGNAL(forvoShow(const QString &)), fForvo, SLOT(show(const QString &)));
}

QForvoLineEdit::QForvoLineEdit ( QWidget * parent  ):QLineEdit(parent){
   setForvo();
}

QForvoLineEdit::QForvoLineEdit ( const QString & contents, QWidget * parent ):
   QLineEdit(contents,parent){
   setForvo();
}
QForvoLineEdit::~QForvoLineEdit (){
   delete fForvo;
}

void QForvoLineEdit::onMenuForvoSay(bool){
   cout<<"Say "<<fForvoText<<"|"<<text()<<endl;
   emit forvoSay(fForvoText);
}

void QForvoLineEdit::onMenuForvoShow(bool){
   cout<<"Show "<<fForvoText<<"|"<<text()<<endl;
   emit forvoShow(fForvoText);
}

void QForvoLineEdit::contextMenuEvent ( QContextMenuEvent * event ){
   fForvoText =  forvoText(event->x());

   QMenu *menu = createStandardContextMenu();
   menu->addAction(QString::fromUtf8("произнести \"")+fForvoText+"\"",
		   this,SLOT(onMenuForvoSay(bool)));
   menu->addAction(QString::fromUtf8("открыть \"")+fForvoText+
		   QString::fromUtf8("\" в Forvo"),
		   this,SLOT(onMenuForvoShow(bool)));
   menu->exec(event->globalPos());
   delete menu;
}

QString QForvoLineEdit::forvoText (int  fMouseX) {
   if(hasSelectedText()) return selectedText();

   QFont font = this->font();
   QFontMetrics metrics(font);

   int pos=fMouseX-metrics.averageCharWidth();

   if(pos>metrics.width(displayText())+metrics.width(" ")/2) return text();

   for(int ip=0; ip<=displayText().count(); ip++){
      float x=metrics.width(displayText().mid(0,ip));
      // cout<<ip<<' '<<displayText().mid(0,ip)
      // 	  <<' '<<x<<' '<<pos<<endl;
      if(x>=pos || (ip==displayText().count()&&x+metrics.width(" ")/2>=pos) ) {
	 //	 cout<<"At "<<displayText().mid(0,ip)<<"|"<<endl;
	 int posd=text().indexOf(displayText());
	 ip+=posd;
	 // cout<<"ATT "<<ip<<' '<<text().mid(0,ip)
	 //     <<"|"<<text().at(ip).isLetter()<<' '<<text().count()<<endl;
	 if(!text().at(ip).isLetter()) {
	    int p0=ip-1, p1=ip+1;
	    while(p0>=0 || p1<text().count()){
	       if(p0>=0 && text().at(p0).isLetter()){
		  ip=p0;
		  break;
	       }
	       if(p1<text().count() && text().at(p1).isLetter()){
		  ip=p1;
		  break;
	       }
	       p0--; p1++;
	    }
	 }
	 if(!text().at(ip).isLetter()) return "";
	 int p0=ip-1;
	 for( ; p0>=0 ; p0--) {
	    if(!text().at(p0).isLetter()) {
	       p0++;
	       break;
	    }
	 }
	 if(p0<0) p0=0;

	 int p1=ip+1;
	 for( ; p1<text().count() ; p1++) {
	    if(!text().at(p1).isLetter()) {
	       break;
	    }
	 }
	 
	 return text().mid(p0,p1-p0);
      }
   }
   return "";
}
