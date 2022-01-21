#include"AddBox.h"

#include"Lingvo.h"
#include<string>
#include"StrStr.h"
#include"QForvoLineEdit.h"
#include<QtWidgets/QHBoxLayout>
#include<QtWidgets/QVBoxLayout>
#include<QtWidgets/QGridLayout>
#include<QtWidgets/QRadioButton>
#include<QtWidgets/QCompleter>
#include<QTabBar>
using namespace std;

static void printLine(QLineEdit *e, const string &s)
{
   e->setEchoMode(QLineEdit::NoEcho);
   e->setText(QString::fromUtf8(s.c_str()));
   e->home(false);
   e->setEchoMode(QLineEdit::Normal);
}

AddPage * AddBox::page(){ return dynamic_cast<AddPage * >(fTabs->currentWidget());}

void AddBox::makeDictEntry(){
   page()->makeDictEntry();
   emit editDone(&page()->dictEntry());
}

AddBox::AddBox(QWidget * parent): QWidget(parent){

   addBut = new QPushButton(QString::fromUtf8("В словарь!"));
   connect(addBut, SIGNAL(clicked()), this, SLOT(makeDictEntry()));

   QPushButton * reFetchBut = new QPushButton(QString::fromUtf8("скачать\n снова"));
   connect(reFetchBut, SIGNAL(clicked()), this, SLOT(reFetch()));

   QPushButton * cancelEditBut = new QPushButton(QString::fromUtf8("отмена"));
   connect(cancelEditBut, SIGNAL(clicked()), this, SLOT(cancel()));

   fTabs = new MyQTabWidget;
   fTabs->addTab(new AddPage,"");
   

   QVBoxLayout *layAdd = new QVBoxLayout;
   QHBoxLayout *layEditBut = new QHBoxLayout;
   layEditBut->addWidget(addBut);
   layEditBut->addWidget(reFetchBut);
   layEditBut->addWidget(cancelEditBut);

   layAdd->addWidget(fTabs);
   layAdd->addLayout(layEditBut);
   setLayout(layAdd);  
   
   // connect(this,SIGNAL(waitForNetRequest()),this,SLOT(onWaitForNetRequest()));
   // connect(this,SIGNAL(netRequestReceived()),this,SLOT(onNetRequestReceived()));
   fOnlineDictName = "Lingvo";
}

void AddBox::cancel(){
   emit editDone(NULL);
}

void AddBox::fixTabText(){
}

void AddBox::addToAddBox(const TDictEntry *e){
   bool found=false;
   for(int ind=0; ind<fTabs->count() && !found; ind++){
      AddPage * p=dynamic_cast<AddPage*>(fTabs->widget(ind));
      if(p && !p->newlyFetched() &&  
	 p->dictEntry().words().first->data()==e->words().first->data()) found=true;
   } 
   if(!found){
      AddPage * page=new AddPage;
      page->setDictEntry(*e);
      page->fillEditFields();
      
      fTabs->addTab(page,QString::fromUtf8(e->words().first->data().c_str()));
      page->setNewlyFetched(false);
      cout<<"Added existing Opps "<<e->words().first->data()<<endl;
   }

}
void AddBox::initTabs(const TDictEntry &e){
   fTabs->setCurrentIndex(0);
   page()->setDictEntry(e);
   page()->fillEditFields();
   string data=page()->dictEntry().words().first->data();
   fTabs->setTabText(fTabs->currentIndex(), QString::fromUtf8(data.c_str())); 

   for(int it=fTabs->count()-1; it>0; it--){
      AddPage * p=dynamic_cast<AddPage*>(fTabs->widget(it));
      fTabs->removeTab(it);
      cout<<"Rem "<<it<<' '<<fTabs->count()<<' '<<p->dictEntry()<<endl;
      delete p;
   }
}

void AddBox::editEntry(const TDictEntry *e){
   initTabs(*e);
   page()->setNewlyFetched(false);

   string data=page()->dictEntry().words().second->data();
   if(!data.empty()) addBut->setFocus(Qt::OtherFocusReason);
}

void AddBox::newWord(const QString & word, const QString & tag){
   TDictEntry  ent;
   ent.words().first->setLanguage(TWord::eDe);
   ent.words().second->setLanguage(TWord::eRu);
   ent.setTags(str(tag));
   ent.words().first->setData(str(word));
   initTabs(ent);
   page()->setNewlyFetched(true);

   reFetch();
}

void AddBox::setOnlineDictName(const QString & text){
   fOnlineDictName = text;
}

void AddBox::reFetch()
{
   OnlineDict * q;
   if(fOnlineDictName == "Lingvo") q = new Lingvo;
//   else if(fOnlineDictName == "Promt") q = new Promt;
   else {
      cerr<<"Wrong OnlineDictName "<<fOnlineDictName<<" !"<<endl;
      return;
   }

   connect(q, SIGNAL(ready(OnlineDict*)), this, SLOT(receiveWord(OnlineDict *)));
   connect(q, SIGNAL(error(OnlineDict*)), this, SLOT(receiveWord(OnlineDict *)));
   q->fetch(QString::fromUtf8(page()->dictEntry().words().first->data().c_str()));

   setEnabled(false);
   emit waitForNetRequest();
}

// void AddBox::onWaitForNetRequest(){
//    setEnabled(false);  
// }

// void AddBox::onNetRequestReceived(){
//    setEnabled(true);
// } 

void AddBox::receiveWord(OnlineDict *q)
{
   emit netRequestReceived();
   setEnabled(true);

   string tag=page()->dictEntry().tags();
   list<TDictEntry *> lst=q->getAll();
   for(list<TDictEntry *>::iterator it=lst.begin(); it!=lst.end(); it++){
      TDictEntry *e=*it;
      cout<<"GGG "<<*e<<endl;

      int idx=distance(lst.begin(),it);
      AddPage * page=dynamic_cast<AddPage*>(fTabs->widget(idx));
      if(!page){
	 page=new AddPage;
	 fTabs->addTab(page,"");
      }
      page->setNewlyFetched(true);

      if(e->words().second->data().empty()) page->setFocus();
      e->setTags(tag);
      page->setDictEntry(*e);
      page->fillEditFields();
      string data=e->words().first->data();
      fTabs->setTabText(idx, QString::fromUtf8(data.c_str())); 

      delete e;
   }

   fTabs->setCurrentIndex(0);
   if(lst.empty()) page()->fillEditFields(); // to set focus to newRu

   if(!page()->dictEntry().words().second->data().empty()) {
      addBut->setFocus(Qt::OtherFocusReason);
      cout<<"SF "<<page()->dictEntry().words().first->data()<<endl;
   }
   q->deleteLater();;
   cout<<"Done receiveWord"<<endl;
   for(int ind=0; ind<fTabs->count(); ind++){
      AddPage * p=dynamic_cast<AddPage*>(fTabs->widget(ind));
      if(p && p->newlyFetched()) emit checkNewWordRepeated(p->dictEntry());
   } 
}

void AddPage::setDictEntry(const TDictEntry &e){
   if(&fEntry!=&e) fEntry=e;
}

const TDictEntry& AddPage::dictEntry() const{
   return fEntry;
}

void AddPage::fillEditFields()
{
   cout<<"FEF "<<fEntry<<endl;
   int id = fEntry.words().first->partOfSpeech();
   if(id != TWord::eNoun && id != TWord::eVerb && id != TWord::eAdjective)
       id = TWord::eOther;
   
   cout<<"Id "<<id<<' '<<endl;
   grPOS->button(id)->setChecked(true);

   if(id == TWord::eNoun) {
      int idG = fEntry.words().first->nounGender();
      if(idG >= 0) grGen->button(idG)->setChecked(true);
   } else if(id == TWord::eVerb) {
      int id = (fEntry.words().first->grammarInfo().find("*")==
		fEntry.words().first->grammarInfo().npos?0:1);
      grVerbArt->button(id)->setChecked(true);
   }

   printLine(newDe, fEntry.words().first->data());
   printLine(newDeForm, fEntry.words().first->forms());
   printLine(newRu, fEntry.words().second->data());
   printLine(newTag, fEntry.tags());
   if(fEntry.words().second->data().empty()) {
      cout<<"newRuF "<<endl;
      newRu->setFocus(Qt::OtherFocusReason);
   }
   cout<<"Done fillEditFields"<<endl;
}


void AddPage::addPOStoggled(bool)
{
   int id=grPOS->checkedId(); 
   cout<<"-------- APT "<<id<<endl;
   gbGen->setVisible((id == TWord::eNoun));
   gbVerbArt->setVisible((id == TWord::eVerb));
   newDeForm->setVisible((id != TWord::eOther));
   labNewForm->setVisible(newDeForm->isVisible());
}

void AddPage::makeDictEntry()
{
   fEntry.words().first->setData(newDe->text().toUtf8().constData());
   fEntry.words().second->setData(newRu->text().toUtf8().constData());

   int id = (grPOS->checkedId() >= 0 ? grPOS->checkedId() : TWord::eOther);
   fEntry.words().first->setPartOfSpeech(id);
   fEntry.words().second->setPartOfSpeech(id);

   if(id == TWord::eNoun) {
      int idG = (grGen->checkedId() >= 0 ? grGen->checkedId() : TWord::eMasculine);
      fEntry.words().first->setNounGender(idG);
   } else if(id == TWord::eVerb) {
      string gistr=fEntry.words().first->grammarInfo();
      bool haveInf=(gistr.find("*")!=gistr.npos);
      if(grVerbArt->checkedId()==0 && haveInf){
	 size_t p;
	 while((p=gistr.find("*"))!=gistr.npos){
	    gistr.erase(p,1);
	 }
	 fEntry.words().first->setGrammarInfo(gistr);
      } else if( grVerbArt->checkedId()==1 && !haveInf){
	 fEntry.words().first->setGrammarInfo(gistr+"*");
      }
   }
   if(newDeForm->isVisible())
      fEntry.words().first->setForms(newDeForm->text().toUtf8().constData());
   else 
      fEntry.words().first->setForms("");

   fEntry.setTags(newTag->text().toUtf8().constData());
   //   emit editDone(fEntry);
}

void AddPage::clearEditFields()
{
   newDeForm->clear();
   newDe->clear();
   newRu->clear();
   newTag->clear();
   for(int i = 0; i < grPOS->buttons().size(); i++)
      grPOS->buttons().takeAt(i)->setChecked(false);
   for(int i = 0; i < grGen->buttons().size(); i++)
      grGen->buttons().takeAt(i)->setChecked(false);
   for(int i = 0; i < grVerbArt->buttons().size(); i++)
      grVerbArt->buttons().takeAt(i)->setChecked(false);
   gbGen->hide();
   gbVerbArt->hide();
   newDeForm->hide();
}

AddPage::AddPage(QWidget * parent): QWidget(parent), fNewlyFetched(false){

   gbPOS = new QGroupBox("");
   grPOS = new QButtonGroup;
   grPOS->addButton(new QRadioButton(QString::fromUtf8("Сущ.")), TWord::eNoun);
   grPOS->addButton(new QRadioButton(QString::fromUtf8("Глагол")), TWord::eVerb);
   grPOS->addButton(new QRadioButton(QString::fromUtf8("Прил.")), TWord::eAdjective);
   grPOS->addButton(new QRadioButton(QString::fromUtf8("Другое")), TWord::eOther);
   QHBoxLayout *layPOS = new QHBoxLayout;
   for(int i = 0; i < grPOS->buttons().size(); i++){
      layPOS->addWidget(grPOS->buttons().takeAt(i));
      connect(grPOS->buttons().takeAt(i), SIGNAL(toggled(bool)),
	      this, SLOT(addPOStoggled(bool)));
   }
   layPOS->addStretch(1);
   gbPOS->setLayout(layPOS);



   gbGen = new QGroupBox("");
   grGen = new QButtonGroup;
   grGen->addButton(new QRadioButton(QString::fromUtf8("м")), TWord::eMasculine);
   grGen->addButton(new QRadioButton(QString::fromUtf8("ж")), TWord::eFeminine);
   grGen->addButton(new QRadioButton(QString::fromUtf8("ср")), TWord::eNeutral);
   QHBoxLayout *layGen = new QHBoxLayout;
   for(int i = 0; i < grGen->buttons().size(); i++)
      layGen->addWidget(grGen->buttons().takeAt(i));
   layGen->addStretch(1);
   gbGen->setLayout(layGen);

   gbVerbArt = new QGroupBox("");
   grVerbArt = new QButtonGroup;
   grVerbArt->addButton(new QRadioButton(QString::fromUtf8("Сильный")), 1);
   grVerbArt->addButton(new QRadioButton(QString::fromUtf8("Слабый")), 0);
   QHBoxLayout *layVerbArt = new QHBoxLayout;
   for(int i = 0; i < grVerbArt->buttons().size(); i++)
      layVerbArt->addWidget(grVerbArt->buttons().takeAt(i));
   layVerbArt->addStretch(1);
   gbVerbArt->setLayout(layVerbArt);




   QLabel * tlab = new QLabel(QString::fromUtf8("немецкий:"));
   newDe = new QForvoLineEdit();
   newDe->setReadOnly(false);
   labNewForm = new QLabel(QString::fromUtf8("формы:"));
   newDeForm = new QForvoLineEdit();
   newDeForm->setReadOnly(false);
   QLabel * tlab2 = new QLabel(QString::fromUtf8("русский:"));
   newRu = new QLineEdit();
   newRu->setReadOnly(false);

   QLabel * labNewTag = new QLabel(QString::fromUtf8("метка для нового слова:"));
   newTag = new QLineEdit();
   newTag->setReadOnly(false);

   QVBoxLayout *layAdd = new QVBoxLayout;
   layAdd->addWidget(gbPOS);
   layAdd->addWidget(gbGen);
   layAdd->addWidget(gbVerbArt);
   layAdd->addWidget(tlab);
   layAdd->addWidget(newDe);
   layAdd->addWidget(labNewForm);
   layAdd->addWidget(newDeForm);
   layAdd->addWidget(tlab2);
   layAdd->addWidget(newRu);
   layAdd->addWidget(labNewTag);
   layAdd->addWidget(newTag);
   setLayout(layAdd);      
}

void AddPage::setNewlyFetched(bool a){ 
   fNewlyFetched=a;
   if(parentWidget() && parentWidget()->parentWidget() && 
      parentWidget()->parentWidget()->inherits("MyQTabWidget")){
      cout<<"Inherrr!"<<endl;
      MyQTabWidget * tw=static_cast<MyQTabWidget*> (parentWidget()->parentWidget());
      int myInd=tw->indexOf(this);
      if(myInd>=0) {
	 cout<<"Ind "<<myInd<<endl;
	 tw->tabBar()->setTabTextColor(myInd,fNewlyFetched? Qt::red : Qt::black);
      }
   }
}


