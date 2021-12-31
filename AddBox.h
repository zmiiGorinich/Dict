#ifndef AddBox_h
#define AddBox_h
#include <QtGui>
#include"TDict.h"
#include<list>
#include<QtWidgets/QWidget>
#include<QtWidgets/QTabWidget>
#include<QtWidgets/QPushButton>
#include<QtWidgets/QButtonGroup>
#include<QtWidgets/QLineEdit>
#include<QtWidgets/QGroupBox>
#include<QtWidgets/QLabel>

class OnlineDict;
class AddPage;
class AddBox : public QWidget
{
   Q_OBJECT

public:
   AddBox(QWidget * parent = 0);

public slots:
   void editEntry(const TDictEntry *);
   void newWord(const QString & word, const QString & tag);
   void setOnlineDictName(const QString & text);
   void addToAddBox(const TDictEntry*);

private slots:
   void reFetch();
   void receiveWord(OnlineDict *);
   void makeDictEntry();
   void cancel();
   // void onWaitForNetRequest();
   // void onNetRequestReceived();
private:
   ////////////////////////////////////
   QTabWidget * fTabs;
   AddPage * fPage;
   QPushButton * addBut;
   AddPage * page();
   void fixTabText();
   void initTabs(const TDictEntry &e);
   QString fOnlineDictName;
signals:
   void editDone(const TDictEntry *);
   void waitForNetRequest();
   void netRequestReceived();
   void checkNewWordRepeated(const TDictEntry&);
};

class AddPage : public QWidget
{
   Q_OBJECT

private slots:
   void addPOStoggled(bool);   

private:
   ////////////////////////////////////
   QButtonGroup * grPOS;
   QButtonGroup * grGen;
   QButtonGroup * grVerbArt;
   QLineEdit * newDe;
   QLineEdit * newDeForm;
   QLineEdit * newRu;
   QGroupBox * gbPOS;
   QGroupBox * gbGen;
   QGroupBox * gbVerbArt;
   QLabel * labNewForm;
   QLineEdit * newTag;
   TDictEntry  fEntry;
   bool  fNewlyFetched;
public:
   AddPage(QWidget * parent = 0);
   void clearEditFields();
   void fillEditFields();
   const TDictEntry& dictEntry() const;
   void setDictEntry(const TDictEntry &e);
   void makeDictEntry();
   void setNewlyFetched(bool a=true);
   bool newlyFetched() const { return fNewlyFetched;}
};

class MyQTabWidget : public QTabWidget
{
   Q_OBJECT
public:
   MyQTabWidget(QWidget * parent = 0):QTabWidget(parent){}
   virtual ~MyQTabWidget(){}
   QTabBar *	tabBar() const{ return QTabWidget::tabBar();}
};

#endif
