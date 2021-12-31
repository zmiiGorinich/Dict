#ifndef App_h
#define App_h
#include <QtGui>
#include"TDict.h"
#include<list>
#include<QtWidgets/QWidget>
#include<QtWidgets/QLabel>
#include<QtWidgets/QLineEdit>
#include<QtWidgets/QComboBox>

class qnm;
class TestBox;
class AddBox;
class TForvo;
class QCloseEvent;
class QVariant;
class DictApp : public QWidget
{
   Q_OBJECT

public:
   DictApp(QWidget *parent = 0);
   ~DictApp();			       
		
public slots:
   void setReadyToClose();

   void edit(TDictEntry * );
   void updateStatus(const char *ch);	   
   void editDone(const TDictEntry *e);
   void onWaitForNetRequest();
   void onNetRequestReceived();
   void showWebPageForLabel();
   void checkExistingWordFetched(const TDictEntry&);
private slots:

   void newWordPressed();
   void newTextEdited(const QString &s);
   void start();

protected:

   virtual void	closeEvent(QCloseEvent * event);

private:
   TDictEntry * fEditEntry;

   TestBox * fTestBox;
   AddBox * fAddBox;
   
   QLabel    * newLab;
   QLineEdit * newText;

   QLabel * labStat;

   QComboBox * comboTags;
   QComboBox * comboNewText;
   void updateComboTags();
   bool fReadyToClose;
   bool fFinalized;
   void finalize();
signals:
   void editEntry(const TDictEntry *);
   void newWord(const QString & word, const QString & tag);
   void addToAddBox(const TDictEntry*);
};
#endif
