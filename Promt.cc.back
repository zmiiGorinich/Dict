#include"Promt.h"
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
#include <unistd.h>


using namespace std;

void Promt::fetch(QString query)
{
   fQuery=query;
   QString req = "http://www.translate.ru/services/TranslationService.asmx/GetFullWordInfo?dirCode=gr&word=" +
         fQuery + "&lang=ru&key=&ts=&isDict=false";
   fReply = manager->get(QNetworkRequest(QUrl(req)));
   connect(fReply, SIGNAL(finished()), this, SLOT(replyFinished()));
   cout << "GOOOTT " << query << endl;
}

void Promt::getNext(QString resCode)
{
   QString req = "http://www.translate.ru/services/TranslationService.asmx/GetFullWordInfoRestInfo?resCode=" + resCode;
   fReply = manager->get(QNetworkRequest(QUrl(req)));
   connect(fReply, SIGNAL(readyRead()), this, SLOT(replyFinished()));
   cout << "NEXT " << resCode << endl;
}
void Promt::replyFinished()
{
   QString str = QString::fromUtf8(fReply->readAll().constData());
   fReply->disconnect();
   fReply->deleteLater();
   // static string fn;
   // fn += "cn";
   // ofstream ofs(fn.c_str());
   // ofs << str << endl;


   QDomDocument docX("AdBookML");
   docX.setContent(str);
   QDomNodeList nstr = docX.elementsByTagName("resStr");
   if(nstr.count() == 0) {
      emit error(this);
      return ; // emit error
   }
   QDomElement estr = nstr.at(0).toElement();
   fHTML += estr.text();

   QDomNodeList ncomplete = docX.elementsByTagName("isComplete");
   if(ncomplete.count() == 0) {
      emit error(this);
       return ; // emit error
   }
   QDomElement ecomplete = ncomplete.at(0).toElement();

   if(ecomplete.text() == "false") {
      QDomNodeList nres = docX.elementsByTagName("resCode");
      if(nres.count() == 0) { 
	 emit error(this);
	 return ; // emit error
      }
      QDomElement eres = nres.at(0).toElement();
      getNext(eres.text());
   } else {
      ofstream ofs("full.html");
      ofs << fHTML << endl;
      parseHTML();
   }
}

void Promt::parseHTML(){
   for(list<dictElementParser*>::iterator it=fCF.begin(); it!=fCF.end(); it++)  delete (*it);
   fCF.clear();
   QWebPage wpage;
   wpage.mainFrame()->setHtml(fHTML);
   QWebElement doc = wpage.mainFrame()->documentElement();

   QWebElementCollection cc = doc.findAll("div.cforms_result");
   if(cc.count() == 0){
      emit ready(this);
      return;
   }

   for (int i = 0; i < cc.count(); ++i)  fCF.push_back(new cformParser(this));
   cout<<"Created "<<fCF.size()<<endl;

   for(list<dictElementParser *>::iterator it=fCF.begin(); it!=fCF.end(); it++){
      cout<<"Call parse "<<distance(fCF.begin(),it)<<endl;
      (*it)->parseElement(cc[distance(fCF.begin(),it)]);
   }

   cout<<"******** MATCH ****************"<<endl;
   TDictEntry *me=getMatch();
   if(me) cout<<*me<<endl;
}




///////////////////////////////////////////////////
void cformParser::parseElement(const QWebElement& e)
{
   QWebElement ref_cform = e.findFirst("div.ref_cform");
   QWebElementCollection trsl = e.findAll("div.trsl");

   QString rs = ref_cform.findFirst("span.ref_source").toPlainText();
   QString sf_sr = ref_cform.findFirst("span.sf_sr").toPlainText();

   string srs = str(rs.section(sf_sr, 0, 0));
   if(!srs.empty() && srs[srs.length() - 1] == '\n') srs.erase(--srs.end());

   fEnt.setWords(TWord(), TWord());
   fEnt.words().first->setLanguage(TWord::eDe);
   fEnt.words().second->setLanguage(TWord::eRu);

   fEnt.words().first->setData(srs);

   QWebElement wepsp = ref_cform.findFirst("span.ref_psp");
   string psp = str(wepsp.toPlainText());
   if(wepsp.isNull()) {
      cout << "NULL" << endl;
      psp = str(ref_cform.findFirst("span.sf_sr").firstChild().toPlainText().section(" ", 0, 0));
   }
   cout << srs << ' ' << psp << "|" << wepsp.isNull() << endl;

   fEnt.words().first->setPartOfSpeech(TWord::eOther);
   if(psp == "Существительное") fEnt.words().first->setPartOfSpeech(TWord::eNoun);
   else if(psp == "Глагол") fEnt.words().first->setPartOfSpeech(TWord::eVerb);
   fEnt.words().second->setPartOfSpeech(fEnt.words().first->partOfSpeech());

   QWebElement etrsl = e.findFirst("div.trsl");
   trsl = etrsl.findAll("div.r_tr");
   for(int i = 0; i < trsl.count(); ++i) {
      QString rs = trsl[i].findFirst("span.r_rs").toPlainText();
      QString sf_sr = trsl[i].findFirst("span.sf_sr").toPlainText();
      string srs = str(rs.section(sf_sr, 0, 0));
      if(!srs.empty() && srs[srs.length() - 1] == '\n') srs.erase(--srs.end());
      if(!fEnt.words().second->data().empty())
         fEnt.words().second->setData(fEnt.words().second->data() + ", ");
      fEnt.words().second->setData(fEnt.words().second->data() + srs);
   }
   if(fEnt.words().first->partOfSpeech() == TWord::eNoun) {
      // QWebElement er_i = ref_cform.findFirst("span.r_i");
      // fEnt.words().first->setGrammarInfo(str(er_i.toPlainText().mid(0, 1)));
      QWebElement tab = ref_cform.findFirst("span.sf_sr").findFirst("table.wf");
      QWebElementCollection ic = tab.findAll("span.tr_f");
      if(ic.count() > 0) {
	 string art = str(ic[0].toPlainText().section(" ", 0, 0));
	 string gi;
	 if(art=="der") gi="m";
	 else if(art=="die") gi="f";
	 else if(art=="das") gi="n";
	 fEnt.words().first->setGrammarInfo(gi);
      }
      if(ic.count() > 1) {
         string pl = str(ic[1].toPlainText().section(" ", 1, 1));
         size_t pos = pl.find(fEnt.words().first->data());
         if(pos != pl.npos) pl = "-" + pl.substr(fEnt.words().first->data().length());
         if(ic.count() > 2) {
            string gen = str(ic[2].toPlainText().section(" ", 1, 1));
            size_t pos = gen.find(fEnt.words().first->data());
            if(pos != gen.npos) gen = "-" + gen.substr(fEnt.words().first->data().length());
            fEnt.words().first->setForms(gen);
         }
         fEnt.words().first->setForms(fEnt.words().first->forms() + ", " + pl);
      }
   } else if(fEnt.words().first->partOfSpeech() == TWord::eVerb) {
      string forms;
      QString elstr=ref_cform.findFirst("span.sf_sr").firstChild().toPlainText();
      if(elstr.indexOf(QString::fromUtf8("Глагол"))>=0){
	 if(elstr.indexOf(QString::fromUtf8("сильный"))>=0) {
	    fEnt.words().first->setGrammarInfo(fEnt.words().first->grammarInfo()+"*");
	 }
      }
      QWebElementCollection tab = ref_cform.findFirst("span.sf_sr").findAll("table.wf");
      for(int i = 0; i < tab.count(); ++i) {
         cout << i << ":" << tab[i].previousSibling().toPlainText() << endl;
         if(tab[i].previousSibling().toPlainText().indexOf(QString::fromUtf8("Indikativ Präsens")) >= 0) {
            cout << "IP " << endl;
            QWebElementCollection ic = tab[i].findAll("span.tr_f");
	    for(int icc=0; icc<ic.count(); icc++){
	       cout<<"Test Prasens "<<ic.count()<<' '<<icc<<' '<<ic[icc].toPlainText()
		   <<"-->"<<ic[icc].toPlainText().indexOf("er/sie/es")<<endl;
	       if(ic[icc].toPlainText().indexOf("er/sie/es")>=0){
		  forms += str(ic[icc].toPlainText().section("er/sie/es ", 1, -1));
		  forms += ",  ";
	       }
	    }
         } else if(tab[i].previousSibling().toPlainText().indexOf(QString::fromUtf8("Indikativ Präteritum")) >= 0) {
            cout << "IP " << endl;
            QWebElementCollection ic = tab[i].findAll("span.tr_f");
            if(ic.count() > 0) {
               forms += str(ic[0].toPlainText().section("ich ", 1, -1));
            }
         } else if(tab[i].previousSibling().toPlainText().
		   indexOf(QString::fromUtf8("Indikativ Perfekt")) >= 0) {
	    cout << "IPerfekt " << endl;
            QWebElementCollection ic = tab[i].findAll("span.tr_f");
	    for(int icc=0; icc<ic.count(); icc++){
	       if(ic[icc].toPlainText().indexOf("er/sie/es ")>=0){
		  forms += " - ";
		  forms += str(ic[icc].toPlainText().section("er/sie/es ", 1, -1));
	       }
	    }	       
         }
      }
      fEnt.words().first->setForms(forms);
   }
   cout << fEnt << endl;
   cout << fEnt.words().first->show() << endl;
   fDone = true;
   emit ready();
}
