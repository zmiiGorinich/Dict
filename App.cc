// -*- coding: utf-8 -*-
#include"App.h"
#include<iostream>
#include<fstream>
#include<iomanip>
#include<cmath>
#include"TestBox.h"
#include"AddBox.h"
#include"StrStr.h"
#include"QForvoLineEdit.h"
#include<QtWidgets/QHBoxLayout>
#include<QtWidgets/QVBoxLayout>
#include<QtWidgets/QGridLayout>
#include<QtWidgets/QCompleter>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include<QtWebKitWidgets/QWebView>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <stdlib.h>
#include <QtWidgets/QErrorMessage>
#include<QVariant>
#include<QTimer>
#include<QMetaType>
#include"Dictor.h"
using namespace std;

static void printLine(QLineEdit *e, const string &s)
{
   e->setEchoMode(QLineEdit::NoEcho);
   e->setText(QString::fromUtf8(s.c_str()));
   e->home(false);
   e->setEchoMode(QLineEdit::Normal);
}
void DictApp::showWebPageForLabel(){
   Dictor::get()->showWebPageForLabel(comboTags->lineEdit()->text().toUtf8().constData());
}

void DictApp::edit(TDictEntry * fCurrent)
{
   if(!fCurrent) return;
   printLine(newText, fCurrent->words().first->data());
   newWordPressed();
}


///////////////////////////////////////////////////////////////////////////

void DictApp::newWordPressed()
{
   fEditEntry= Dictor::dict().find(newText->text().toUtf8().constData());

   fTestBox->setEnabled(false);
   fAddBox->show();
   if(fEditEntry) emit editEntry(fEditEntry);
   else emit newWord(newText->text(),comboTags->lineEdit()->text());
   fAddBox->setFocus();
}


///////////////////////////////////////////////////////////////////////////
void DictApp::updateStatus(const char *ch)
{
   labStat->setText(QString::fromUtf8(ch));
}

void DictApp::updateComboTags()
{
   QString ttag = comboTags->lineEdit()->text();
   comboTags->clear();

   for(auto tag: Dictor::dict().tags()) comboTags->addItem(QString::fromUtf8(tag.c_str()));

   comboTags->lineEdit()->setText(ttag);
}

void DictApp::newTextEdited(const QString &s){
   list<string> lst=Dictor::dict().nearMatch(str(s));
   QCompleter * c=newText->completer();
   QStringListModel *model = (QStringListModel*)(c->model());
   QStringList stringList;
   for(list<string>::const_iterator it = lst.begin(); it != lst.end(); it++) {
      stringList.append(QString::fromUtf8(it->c_str()));
   }
   model->setStringList(stringList);
}

bool check_Proc();

void DictApp::start(){
   Dictor::get()->readGDrive();
   updateComboTags();
   connect(comboTags, SIGNAL(editTextChanged(const QString &)),
           fTestBox, SLOT(comboTagEdited(const QString &)));
   show();
}

void DictApp::checkExistingWordFetched(const TDictEntry&e){
   cout<<"To CHECK "<<e.words().first->data()<<endl;
   TDictEntry * ent=Dictor::dict().find(e.words().first->data());
   if(ent) emit addToAddBox(ent);
}

DictApp::DictApp(QWidget *parent): QWidget(parent), fEditEntry(NULL), 
				   fReadyToClose(false), fFinalized(false)
{
   QTimer::singleShot(0, this, SLOT(start()));

   check_Proc();

   connect(Dictor::get(), SIGNAL(updateStatusLine(const char *)), this, SLOT(updateStatus(const char *)));

   //////////////////////////////////////////////////////////////////////////////////////
   fTestBox=new TestBox;
   fTestBox->setDict(&Dictor::dict());
   connect(fTestBox, SIGNAL(updateStats(const char *)), this, SLOT(updateStatus(const char *)));
   connect(fTestBox, SIGNAL(editEntry(TDictEntry *)),   this, SLOT(edit(TDictEntry *)));


   //////////////////////////////////////////////////////////////////////////////////////

   fAddBox = new AddBox;
   fAddBox->setVisible(false);
   connect(this, SIGNAL(editEntry(const TDictEntry *)), fAddBox, SLOT(editEntry(const TDictEntry *)));
   connect(this, SIGNAL(newWord(const QString & , const QString & )), 
	   fAddBox, SLOT(newWord(const QString & , const QString & )));
   connect(fAddBox, SIGNAL(editDone(const TDictEntry *)), 
	   this, SLOT(editDone(const TDictEntry *)));
   connect(fAddBox,SIGNAL(waitForNetRequest()),this,SLOT(onWaitForNetRequest()));
   connect(fAddBox,SIGNAL(netRequestReceived()),this,SLOT(onNetRequestReceived()));
   connect(fAddBox,SIGNAL(checkNewWordRepeated(const TDictEntry&)),
	   this,SLOT(checkExistingWordFetched(const TDictEntry&)));
   connect(this,SIGNAL(addToAddBox(const TDictEntry*)),
	   fAddBox,SLOT(addToAddBox(const TDictEntry*)));
   connect(fAddBox, SIGNAL(updateStatusLine(const char *)), this, SLOT(updateStatus(const char *)));

   //////////////////////////////////////////////////////////////////////////////////
   QHBoxLayout *taglayout = new QHBoxLayout;

   QLabel * tagLab = new QLabel(QString::fromUtf8("метка:"));

   comboTags = new QComboBox;
   comboTags->setInsertPolicy(QComboBox::NoInsert);
   comboTags->setEditable(true);

   QPushButton *WebPageBut = new QPushButton(QString::fromUtf8("WebPage"));
   connect(WebPageBut, SIGNAL(clicked()), this, SLOT(showWebPageForLabel()));


   taglayout->addWidget(tagLab);
   taglayout->addWidget(comboTags);
   taglayout->setStretchFactor(comboTags,10);
   taglayout->addWidget(WebPageBut);

   ///////////////////////////////////////////////////////////////////////////////
   newLab = new QLabel(QString::fromUtf8("найти или добавить слово:"));
   newText = new QForvoLineEdit();
   newText->setReadOnly(false);
   newText->setCompleter(new QCompleter(new QStringListModel,this));

   connect(newText, SIGNAL(returnPressed()), this, SLOT(newWordPressed()));
   connect(newText, SIGNAL(textEdited(const QString &)), this, SLOT(newTextEdited(const QString &)));
   newText->completer()->setMaxVisibleItems(10);
   newText->completer()->setCaseSensitivity(Qt::CaseInsensitive);
   newText->completer()->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

   /////////////////////////////////////////////////////////////////////////////////////
   QHBoxLayout *dictlayout = new QHBoxLayout;

   //   QLabel * dictLab = new QLabel(qstr("Словарь:"));

   QComboBox *comboDicts = new QComboBox;
   comboDicts->setInsertPolicy(QComboBox::NoInsert);
   comboDicts->setEditable(false);
   comboDicts->addItem(qstr("Promt"));
   comboDicts->addItem(qstr("Lingvo"));
   comboDicts->setCurrentIndex(0);

   dictlayout->addWidget(newText);
   dictlayout->addWidget(comboDicts);
   dictlayout->setStretchFactor(comboDicts,0.1);

   connect(comboDicts, SIGNAL(currentIndexChanged ( const QString & )),
           fAddBox, SLOT(setOnlineDictName(const QString &)));
   //////////////////////////////////////////////////////////////
  

   labStat = new QLabel(QString::fromUtf8("Статистика:"));


   QVBoxLayout *mainlayout = new QVBoxLayout;
   mainlayout->addWidget(labStat);
   mainlayout->addLayout(taglayout);
   mainlayout->addWidget(fTestBox);
   mainlayout->addWidget(newLab);
   //   mainlayout->addWidget(newText);
   mainlayout->addLayout(dictlayout);
   mainlayout->addWidget(fAddBox);
   mainlayout->addStretch();
   setLayout(mainlayout);
   setWindowTitle("DictApp");

} 

void DictApp::editDone(const TDictEntry *e){
   cout<<"EDDDDD "<<e<<endl;
   if(e) cout<<*e<<endl;
   fAddBox->hide();
   fTestBox->setEnabled(true);
   newText->setReadOnly(false);
   newText->clear();
   if(e){
      if(fEditEntry){
	 fEditEntry->setWords(*e->words().first,*e->words().second);
	 fEditEntry->setTags(e->tags());
      } else {
	 Dictor::dict().addEntry(new TDictEntry(*e));
      }
   }
   newText->setFocus(Qt::OtherFocusReason);
   fEditEntry = NULL;
}

void DictApp::onWaitForNetRequest(){
   newText->setReadOnly(true);
}
void DictApp::onNetRequestReceived(){
   newText->setReadOnly(false);
}


DictApp::~DictApp(){
}
void DictApp::finalize()
{
   cout << "finalize" << endl;

   fReadyToClose=true;
   Dictor::get()->saveToGDrive();

   cout << "FINALIZE done" << endl;
   fFinalized=true;
   return;




   cout << "FINALIZE done" << endl;
   fFinalized=true;

}
void DictApp::setReadyToClose(){
   fReadyToClose=true;
   close();
}
void DictApp::closeEvent(QCloseEvent *event)
{
   event->accept();
   if(!fReadyToClose) {
      if(!fFinalized) finalize();
      if(!fReadyToClose) event->ignore();
   }
}
