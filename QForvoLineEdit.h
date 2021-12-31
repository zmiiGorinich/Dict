#ifndef qforvolineedit_h
#define qforvolineedit_h
#include<QtWidgets/QLineEdit>
class TForvo;

class QForvoLineEdit: public QLineEdit{
   Q_OBJECT
   TForvo * fForvo;
   QString fForvoText;
   void setForvo();
   QString forvoText (int ) ;
public:
   ~QForvoLineEdit ();
   QForvoLineEdit ( QWidget * parent = 0 );
   QForvoLineEdit ( const QString & contents, QWidget * parent = 0 );
   void contextMenuEvent ( QContextMenuEvent * event );
public slots:
   void onMenuForvoSay(bool);
   void onMenuForvoShow(bool);
signals:
   void forvoSay(const QString &);
   void forvoShow(const QString &);   
};
#endif
