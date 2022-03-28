#include"OnlineDict.h"
#include<QtNetwork/QNetworkAccessManager>
#include<QtNetwork/QNetworkReply>
#include"TDict.h"
#include"unicode/unistr.h"
//#include <unicode/ustream.h>

using namespace std;
using icu::UnicodeString;
OnlineDict::OnlineDict()
{
    if(!gNetworkMagnager) gNetworkMagnager = new QNetworkAccessManager();
}

OnlineDict::~OnlineDict()
{
    for(list<dictElementParser*>::iterator it = fCF.begin(); it != fCF.end(); it++)
        delete(*it);
}

list<dictElementParser *>::iterator
OnlineDict::getMatchIterator()
{
    UnicodeString uq = UnicodeString::fromUTF8(fQuery.toUtf8().constData());
    for(list<dictElementParser *>::iterator it = fCF.begin(); it != fCF.end(); it++)
    {
        if(!(*it)->ok()) continue;
        UnicodeString ud = UnicodeString::fromUTF8((*it)->fEnt.words().first->data());
        if(uq == ud) return it;
    }
    uq.toLower();
    for(list<dictElementParser *>::iterator it = fCF.begin(); it != fCF.end(); it++)
    {
        if(!(*it)->ok()) continue;
        UnicodeString ud = UnicodeString::fromUTF8((*it)->fEnt.words().first->data()).toLower();
        if(uq == ud) return it;
    }
    if(!fCF.empty()) return fCF.begin();
    return fCF.end();
}

list<TDictEntry *>
OnlineDict::getAll()
{
    list<TDictEntry *> lst;
    if(fCF.empty()) return lst;
    lst.push_back(getMatch());
    list<dictElementParser *>::iterator im = getMatchIterator();
    for(list<dictElementParser *>::iterator it = fCF.begin(); it != fCF.end(); it++)
    {
        if(!(*it)->ok()) continue;
        if(it != im) lst.push_back(new TDictEntry((*it)->fEnt));
    }
    return lst;
}

TDictEntry *
OnlineDict::getMatch()
{
    list<dictElementParser *>::iterator it = getMatchIterator();
    if(it != fCF.end()) return new TDictEntry((*it)->fEnt);
    return NULL;
}

void
OnlineDict::parserDone()
{
    cout << "ParserDone " << endl;
    for(list<dictElementParser*>::iterator it = fCF.begin(); it != fCF.end(); it++)
    {
        cout << "PD " << (*it)->isDone() << endl;
        if(!(*it)->isDone()) return;
    }
    cout << "AllPD!" << fCF.size() << endl;
    emit ready(this);
}

QNetworkAccessManager *OnlineDict::gNetworkMagnager = nullptr;


dictElementParser::dictElementParser(OnlineDict *dict): fDone(false), fOK(true)
{
    connect(this, SIGNAL(ready()), dict, SLOT(parserDone()));
}

dictElementParser::~dictElementParser()
{
    disconnect();
}

