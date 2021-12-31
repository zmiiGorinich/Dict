#ifndef TestBox_h
#define TestBox_h
#include <QtGui>
#include"TDict.h"
#include<list>
#include<QtWidgets/QGroupBox>
#include<QtWidgets/QPushButton>
#include<QtWidgets/QLineEdit>
#include<QtWidgets/QComboBox>
class TestBox : public QGroupBox
{
   Q_OBJECT

public:
   TestBox(QWidget * parent = 0);
   void setDict(TDict *);
   TDictEntry *currentEntry();	
						      
public slots:
   void comboTagEdited(const QString & text);
private slots:
   void showDictEntry();
   void nextDictEntry();
   void yes();
   void no();
   void edit();
   void modeChanged(int);
private:
   TDictEntry* fCurrent;
   TDict * fDict;
   std::string fTag;

   QPushButton * showBut;
   QPushButton * nextBut;
   QPushButton * yesBut;
   QPushButton * noBut;
   QPushButton * editBut;

   QLineEdit * ruText;
   QLineEdit * deText;

   int nTry, nOK;
   std::list<int> nOK20;

   bool hasVoted;
   bool hasShown;
   
   void updateStats();
   void incrStats(bool ok);
   void doShowDictEntry(bool all);
   void yes_no(bool da);
   QComboBox * comboDir;
   QComboBox * comboMode;
signals:
   void updateStats(const char *);
   void editEntry(TDictEntry*);
   
};
#endif
