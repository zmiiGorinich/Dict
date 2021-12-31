#include"TestBox.h"
#include"QForvoLineEdit.h"
#include<numeric>
#include <stdlib.h>
#include<QtWidgets/QHBoxLayout>
#include<QtWidgets/QVBoxLayout>
#include<QtWidgets/QGridLayout>
using namespace std;

static void printLine(QLineEdit *e, const string &s)
{
   e->setEchoMode(QLineEdit::NoEcho);
   e->setText(QString::fromUtf8(s.c_str()));
   e->home(false);
   e->setEchoMode(QLineEdit::Normal);
}

void TestBox::modeChanged(int idx){
   if(idx==0) comboDir->setCurrentIndex(2);
   if(idx==1) comboDir->setCurrentIndex(0);
}

TestBox::TestBox(QWidget * parent): QGroupBox(parent), fCurrent(0), fDict(0) {
   nTry = nOK = 0;
   hasVoted = hasShown = false;
  
   QHBoxLayout *layDirMode = new QHBoxLayout;

   comboDir = new QComboBox;
   comboDir->addItem(QString::fromUtf8("Вперёд"));
   comboDir->addItem(QString::fromUtf8("Назад"));
   comboDir->addItem(QString::fromUtf8("Случайно"));
   comboDir->setCurrentIndex(2);

   comboMode = new QComboBox;
   comboMode->addItem(QString::fromUtf8("Тест"));
   comboMode->addItem(QString::fromUtf8("Повторение"));
   comboMode->setCurrentIndex(0);
   connect(comboMode,SIGNAL(currentIndexChanged(int)), this, SLOT(modeChanged(int)));

   layDirMode->addWidget(comboDir);
   layDirMode->addWidget(comboMode);
 
   showBut = new QPushButton(QString::fromUtf8("показать"));
   connect(showBut, SIGNAL(clicked()), this, SLOT(showDictEntry()));
   nextBut = new QPushButton(QString::fromUtf8("дальше"));
   connect(nextBut, SIGNAL(clicked()), this, SLOT(nextDictEntry()));
   yesBut = new QPushButton(QString::fromUtf8("знаю"));
   connect(yesBut, SIGNAL(clicked()), this, SLOT(yes()));
   noBut = new QPushButton(QString::fromUtf8("не знаю"));
   connect(noBut, SIGNAL(clicked()), this, SLOT(no()));
   editBut = new QPushButton(QString::fromUtf8("править"));
   connect(editBut, SIGNAL(clicked()), this, SLOT(edit()));

   ruText = new QLineEdit;
   ruText->setPlaceholderText(QString::fromUtf8("русский"));
   ruText->setReadOnly(true);
   deText = new QForvoLineEdit;
   deText->setPlaceholderText(QString::fromUtf8("deutsch"));
   deText->setReadOnly(true);

   QVBoxLayout *mainlayout = new QVBoxLayout;
   mainlayout->addLayout(layDirMode);
   mainlayout->addWidget(ruText);
   mainlayout->addWidget(deText);

   QGridLayout *butlayout = new QGridLayout;
   butlayout->addWidget(yesBut, 0, 0);
   butlayout->addWidget(noBut, 0, 1);
   butlayout->addWidget(showBut, 1, 0);
   butlayout->addWidget(nextBut, 1, 1);
   mainlayout->addLayout(butlayout);
   mainlayout->addWidget(editBut);
   
   setLayout(mainlayout);
}

void TestBox::doShowDictEntry(bool all)
{
   if(fCurrent) {
      if(all) {
	 printLine(deText, fCurrent->words().first->show());
	 printLine(ruText, fCurrent->words().second->show());
      } else {
	 if(double(random())<0.3*double(RAND_MAX)){
	    printLine(deText, fCurrent->words().first->data());	 
	    ruText->clear();
	 } else {
	    printLine(ruText, fCurrent->words().second->show());	
	    deText->clear();
	 }
      }
   } else {
      ruText->clear();
      deText->clear();
   }
   cout << "show" << endl;
}


void TestBox::showDictEntry()
{
   if(!hasShown) {
      yesBut->setText(QString::fromUtf8("знаю - дальше"));
      noBut->setText(QString::fromUtf8("не знаю - дальше"));
      yesBut->setEnabled(true);
      noBut->setEnabled(true);
      showBut->setEnabled(false);
      nextBut->setEnabled(true);
   }

   doShowDictEntry(true);

   hasShown = true;
   cout << "show" << endl;
}

void TestBox::nextDictEntry()
{
   if(comboDir->currentIndex() == 2) fCurrent = fDict->getRandomHistory(fTag,fCurrent);
   else fCurrent = fDict->getNext(fTag, fCurrent, (comboDir->currentIndex() == 0));

   bool repeat = (comboMode->currentIndex() == 1);
   doShowDictEntry(repeat);

   yesBut->setEnabled(!repeat);
   noBut->setEnabled(!repeat);
   showBut->setEnabled(!repeat);
   nextBut->setEnabled(true);
   editBut->setEnabled(true);

   hasVoted = hasShown = false;
   yesBut->setText(QString::fromUtf8("знаю"));
   noBut->setText(QString::fromUtf8("не знаю"));
   cout << "next" << endl;
}


void TestBox::yes_no(bool da)
{
   if(hasVoted || hasShown) {
      if(fCurrent) {
	 fDict->addAttempt(fCurrent, da);
	 incrStats(da);
	 updateStats();
      }
      nextDictEntry();
      return;
   }

   showDictEntry();
   hasShown=true;
   
   QPushButton *b = (da ? yesBut : noBut);
   yesBut->setEnabled(true);
   noBut->setEnabled(true);
   showBut->setEnabled(false);
   nextBut->setEnabled(false);

   b->setText(QString::fromUtf8("дальше"));
   if(da) noBut->setText(QString::fromUtf8("Ой, не знаю!"));
   else   yesBut->setText(QString::fromUtf8("Ой, знаю!"));
}

void TestBox::yes()
{
   yes_no(true);
   cout << "yes" << endl;
}

void TestBox::no()
{
   yes_no(false);
   cout << "no" << endl;
}
void TestBox::edit(){
   emit editEntry(fCurrent);
}

void TestBox::updateStats()
{
   int n20 = accumulate(nOK20.begin(), nOK20.end(), 0);
   int rTot = 0;
   if(nTry > 0) rTot = lrint(100 * (float(nOK) / nTry));
   int r20 = 0;
   if(!nOK20.empty()) r20 = lrint(100 * (float(n20) / nOK20.size()));
   cout << "Stats: " << n20 << ' ' << nOK20.size() << ' ' << r20 << ' ' << endl;
   copy(nOK20.begin(), nOK20.end(), ostream_iterator<int>(cout, " "));
   cout << endl;
   char ch[1000];
   sprintf(ch, "<b>Всего</b>: <font color=red>%i%% </font>из %i, <b>посл.</b> %i: <font color=red>%i%%</font>", rTot, nTry, (int)nOK20.size(), r20);
   emit updateStats(ch);
}
void TestBox::incrStats(bool ok)
{
   nTry++;
   if(ok) nOK++;
   nOK20.push_back(ok);
   if(nOK20.size() > 20) nOK20.pop_front();
}
void TestBox::comboTagEdited(const QString & text)
{
   fTag = text.toUtf8().constData();
   cout << "cte " << fTag << endl;

   nTry = nOK = 0;
   nOK20.clear();
   updateStats();
}

void TestBox::setDict(TDict *d){ fDict=d;}

TDictEntry *TestBox::currentEntry(){ return fCurrent;}		 

