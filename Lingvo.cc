#include"Lingvo.h"
#include<iostream>
#include<fstream>
#include<QtWidgets/QApplication>
#include <QtXml/QDomElement>
#include <QtXml/QDomDocument>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebPage>
#include<QtWebKit/QWebElement>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include"StrStr.h"
#include"TWord.h"
#include <QJsonDocument>

using namespace std;

void Lingvo::fetch(QString query)
{
   fQuery=query;
   //   QString req = "http://www.lingvo-online.ru/en/Translate/de-ru/" + fQuery;
   QString req = "https://www.lingvolive.com/en-us/translate/de-ru/"+fQuery;
   fReply = manager->get(QNetworkRequest(QUrl(req)));
   connect(fReply, SIGNAL(finished()), this, SLOT(replyFinished()));
   cout << "Lingvo Start " << query << endl;
}
void callBack(const QString &s){
   cout<<"Call back! "<<endl;
   cout<<"S="<<s<<endl;
   exit(0);
}
#include <QWebEnginePage>
void Lingvo::replyFinished()
{
   QString str = QString::fromUtf8(fReply->readAll().constData());
   fReply->disconnect();
   fReply->deleteLater();
   // static string fn;
   // fn += "lcn";
   // ofstream ofs(fn.c_str());
   // ofs << str << endl;

   fHTML = str;

   ofstream ofsh("Lingvo.html");
   ofsh << fHTML << endl;
   QWebEnginePage *p=new QWebEnginePage;
   p->setHtml(str);
   p->toPlainText(callBack);
   parseHTML();

}
static void remFunctions(QString & str){
   int ind=0;
   while((ind=str.indexOf("function ",ind))>=0){
      int idx=ind;
      for(;idx>=0; idx--){
	 if(str[idx]==QChar('\"') || str[idx]==QChar(':')) break;
      } 
      if(idx<0) continue;//???
      if(str[idx]==QChar('\"')) continue;
      int op=str.indexOf("{",ind);
      if(op<0) continue;//???
      int bc=1, cl=op+1;
      for(; cl<str.count(); cl++){
	 if(str[cl]==QChar('{')) bc++;
	 else if(str[cl]==QChar('}')) bc--;
	 if(bc==0) break;
      }
      str.remove(ind,cl-ind);
      str.insert(ind,"{");
   }
}
void Lingvo::parseHTML(){
   for(list<dictElementParser*>::iterator it=fCF.begin(); it!=fCF.end(); it++)
      delete (*it);
   fCF.clear();

   QWebPage wpage;
   wpage.mainFrame()->setHtml(fHTML);
   QWebElement doc = wpage.mainFrame()->documentElement();
   ofstream ofsh("scripts.txt");
   ofsh<<doc.toPlainText()<<endl;;
   cout<<"PT "<<doc.toPlainText()<<endl;
   QWebElementCollection scripts=doc.findAll("script");
   cout<<"Scripts "<<scripts.count()<<endl;
   for (int i = 0; i < scripts.count(); ++i){
      QString str=scripts[i].toPlainText();
      // ofsh<<"===================== "<<i<<" ==========================="<<endl;
      // ofsh<<str<<endl;
      int ind=str.indexOf("window.__initialState__ =");
      cout<<"ind0 "<<ind<<endl;
      ofsh<<"ind0 "<<ind<<endl;
      if(ind<0) continue;
      int jsbeg= str.indexOf("{",ind);
      cout<<"jsbeg "<<jsbeg<<endl;
      if(jsbeg<0) continue;
      int indEnd=str.indexOf("window.__BUILD_HASH__ =");
      cout<<"indEnd "<<indEnd<<endl;
      if(indEnd<0) continue;
      for(;indEnd>=0;indEnd--)
	 if(str[indEnd]==QChar('}')) break;
      str=str.mid(jsbeg,indEnd-jsbeg+1);

      remFunctions(str);
      QJsonParseError err;
      QJsonDocument jdoc=QJsonDocument::fromJson(str.toUtf8(),&err);
      ofsh<<QString::fromUtf8(jdoc.toJson())<<endl;
      cout<<"JS "<<jdoc.isNull()<<' '<<jdoc.isEmpty()
	  <<' '<<jdoc.isObject()<<' '<<err.errorString()<<endl;

   }
   ofsh.close();
   QWebElement js_sec_data=doc.findFirst("div.js-section-data");
   QWebElementCollection gcards=js_sec_data.findAll("div.g-card.js-article-html");

   if(gcards.count() == 0){
      emit ready(this);
      return;
   }

   for (int i = 0; i < gcards.count(); ++i){
      cout<<"Lingvo ---------------- "<<i<<endl;
      cout<<gcards[i].attribute("data-url")<<endl;
      fCF.push_back(new gcardParser(this));
   }

   for(list<dictElementParser *>::iterator it=fCF.begin(); it!=fCF.end(); it++){
      cout<<"Call parse "<<distance(fCF.begin(),it)<<endl;
      (*it)->parseElement(gcards[distance(fCF.begin(),it)]);
   }

   cout<<"******** MATCH **************** "<<endl;
   TDictEntry *me=getMatch();
   if(me) cout<<*me<<endl;

   //   qApp->exit(0);
   cout<<"Linvo done"<<endl;

}

void Lingvo::parserDone(){
   cout<<"ParserDone Lingvo"<<endl;
   for(list<dictElementParser*>::iterator it=fCF.begin(); it!=fCF.end(); it++){
      cout<<"PD "<<(*it)->isDone()<<endl;
      if(!(*it)->isDone()) return;
   }

   for(list<dictElementParser*>::iterator it=fCF.begin(); it!=fCF.end(); it++){
      gcardParser * p=dynamic_cast<gcardParser *>(*it);
      if(p->fDictName=="Universal"){
	 for(list<dictElementParser*>::iterator ir=fCF.begin(); ir!=fCF.end(); ir++){
	    if(it==ir) continue;
	    gcardParser * pr=dynamic_cast<gcardParser *>(*ir);
	    if(pr->fDictName!="Universal" && 
	       pr->fEnt.words().first->data() == p->fEnt.words().first->data())
	       pr->setOK(false);
	 }
      }
   } 

   cout<<"AllPD!"<<fCF.size()<<endl;
   emit ready(this);
}


///////////////////////////////////////////////////
void gcardParser::parseElement(const QWebElement& e)
{
   fDictName=e.attribute("data-url");
   fDictName=fDictName.mid(fDictName.indexOf("dictionarySystemName=")+
			   strlen("dictionarySystemName="));
   fDictName=fDictName.mid(0,fDictName.indexOf("&"));
   fDictName.replace("%20"," ");
   fDictName=fDictName.mid(0,fDictName.indexOf("(")).trimmed();

   cout<<"DictName: "<<fDictName<<endl;
   QWebElement head=e.findFirst("h2>span.Bold");
   cout<<"LWord "<<head.toInnerXml()<<endl;
   fEnt.words().first->setData(str(head.toInnerXml()));
   QWebElement gram=e.findFirst("p.P1").findFirst("span.l-article__abbrev[title]");

   fEnt.words().first->setPartOfSpeech(TWord::eOther);
   cout<<"Gram "<<gram.attribute("title")<<'|'<<gram.toPlainText()
       <<"|"<<gram.attribute("title").contains("Maskulinum")<<endl;
   if(gram.attribute("title").contains("Femininum") ||
      gram.attribute("title").contains(QString::fromUtf8("женский род")) ||
      (gram.attribute("title").isEmpty() && 
       gram.toPlainText()=="f")){
      fEnt.words().first->setPartOfSpeech(TWord::eNoun);
      fEnt.words().first->setNounGender(TWord::eFeminine);
   } else if(gram.attribute("title").contains("Maskulinum") ||
	gram.attribute("title").contains(QString::fromUtf8("мужской род")) ||
	(gram.attribute("title").isEmpty() && 
	 gram.toPlainText()=="m")){
      cout<<"LFound m"<<endl;
      fEnt.words().first->setPartOfSpeech(TWord::eNoun);
      fEnt.words().first->setNounGender(TWord::eMasculine);
   } else if(gram.attribute("title").contains("Neutrum") ||
	gram.attribute("title").contains(QString::fromUtf8("средний род")) ||
	(gram.attribute("title").isEmpty() && 
	 gram.toPlainText()=="n")){
      fEnt.words().first->setPartOfSpeech(TWord::eNoun);
      fEnt.words().first->setNounGender(TWord::eNeutral);
   } else if(gram.attribute("title").contains("Verb")){
      fEnt.words().first->setPartOfSpeech(TWord::eVerb);
   } 
   fEnt.words().second->setPartOfSpeech(fEnt.words().first->partOfSpeech());

   if(fEnt.words().first->partOfSpeech()==TWord::eNoun){
      QWebElement forms=e.findFirst("p.P1").findFirst("span.l-article__abbrev[title] + span[class][style]");
      //      cout<<"Forms "<<forms.isNull()<<' '<<forms.toInnerXml()<<endl;
      QString v=forms.toInnerXml().trimmed();
      if(v.startsWith("&lt;")) v=v.mid(4);
      if(v.endsWith("&gt;")) v=v.mid(0,v.count()-4);

      cout<<e.findFirst("p.P1").toInnerXml()<<endl;
      fEnt.words().first->setForms(str(v));
   }

   QString tr;
   QWebElementCollection trans=e.findAll("p.P1");
   int ntrans=0;
   for(int ip=0; ip<trans.count(); ip++){
      if(!trans[ip].findFirst("span.translation").isNull()){
	 
	 QWebElementCollection span=trans[ip].findAll("span[class]");
	 if(span.count()>0){
	    if(ntrans++>=3) break;
	 }

	 if(tr.count()) tr+=" ";
	 QWebElement href=trans[ip].firstChild();
	 tr+=trans[ip].toInnerXml().mid(0,trans[ip].toInnerXml().indexOf(href.toOuterXml()));

	 for(int is=0; is<span.count(); is++){
	    QString add;
	    if(span[is].attribute("class").contains("translation")){
	       add=span[is].toPlainText();
	    } else if(span[is].attribute("class").contains("l-article__abbrev") ||
		      span[is].attribute("class").contains("l-article__comment") ){
	       add=span[is].toPlainText();
	       if(ip==0 && span[is].hasAttribute("title")) continue;
	       if(!add.startsWith("(")) add="("+add;
	       if(!add.endsWith(")")) add+=")";
	    }
	    add=add.trimmed();
	    if(tr.count()&&!add.startsWith(",")&&!add.startsWith(";")&&!tr.endsWith(" ")) tr+=" ";
	    if( (add.startsWith(",") || add.startsWith(";")) &&tr.endsWith(" ") ) tr=tr.mid(0,tr.count()-1);
	    tr+=add;
	 }
      }
   }
   fEnt.words().second->setData(str(tr.trimmed()));
   if(fEnt.words().first->partOfSpeech()==TWord::eVerb) fetchVerbForms();
   else {
      cout<<fEnt<<endl;
      fDone=true;
      emit ready();
   }
}

void gcardParser::fetchVerbForms(){
   QString req = "http://www.verbformen.com/conjugation/" + 
      qstr(fEnt.words().first->data()) + ".htm";
   fReply = OnlineDict::networkManager()->get(QNetworkRequest(QUrl(req)));
   connect(fReply, SIGNAL(finished()), this, SLOT(replyFinished()));
   cout << "VerbForms Start " << req << endl;
}

void gcardParser::replyFinished(){
   cout<<"VerbForms reply finished"<<endl;
   fDone = true;
   QString HTML = QString::fromUtf8(fReply->readAll().constData());
   fReply->disconnect();
   fReply->deleteLater();
   
   ofstream ofs("VerbForms.html");
   ofs << HTML << endl;

   QWebPage wpage;
   wpage.mainFrame()->setHtml(HTML);
   QWebElement doc = wpage.mainFrame()->documentElement();
   QWebElement unbekant = doc.findFirst("div.einstellung-warnhinweis");
   cout<<"Unbek "<<unbekant.isNull()<<'|'<<unbekant.toPlainText()<<'|'<<endl;
   if(!unbekant.isNull() && unbekant.toPlainText().contains("verbtabelle.unbekanntes_verb")){
      emit ready();
      return;
   }
   QString title = doc.findFirst("head").findFirst("title").toPlainText();
   title=title.section("-",1,2).trimmed();
   cout<<"Title :"<<title<<endl;
   fEnt.words().first->setForms(str(title));
   cout<<"With verbFormen:"<<fEnt<<endl;
   emit ready();
}

