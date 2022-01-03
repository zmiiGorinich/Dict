#include"TWord.h"
#include"TDict.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QTextEdit>
#include <QtGui/QtGui>
#include <unistd.h>
#include"App.h"
#include <QtWebEngine/QtWebEngine>
#include "sigwatch.h"
#include "myhttpserver.h"
#include <unistd.h>

using namespace std;
int main(int argv, char **args)
{

   QtWebEngine::initialize();
   QApplication app(argv, args);
   qRegisterMetaType<HTTPmessage>("HTTPmessage");

   UnixSignalWatcher sigwatch;
   sigwatch.watchForSignal(SIGUSR1);
   sigwatch.watchForSignal(SIGTERM);
   sigwatch.watchForSignal(SIGINT);


   DictApp  dapp;


   QObject::connect(&sigwatch, SIGNAL(unixSignal(int)), &dapp, SLOT(close()));
   
   //   dapp.show();
   int ret= app.exec();
   return ret;
}
