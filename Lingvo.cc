#include"Lingvo.h"
#include<iostream>
#include<fstream>
#include<QtWidgets/QApplication>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include"StrStr.h"
#include"TWord.h"
#include <QStringList>
using namespace std;

void
Lingvo::fetch(QString query)
{
    fQuery = query;
    if(socket) delete socket;
    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(connected()), this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this,
        SLOT(socketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
        SLOT(socketError(QAbstractSocket::SocketError)));

    qDebug() << "connecting...";

    // this is not blocking call
    socket->connectToHost("91.238.104.225", 9999);

    cout << "Lingvo Start " << query << endl;
}

void
Lingvo::socketError(QAbstractSocket::SocketError socketError)
{
    cout<<"Lingvo socket error "<<socketError<<endl;
    if(!fGotReply) emit ready(this);
    else cout<<"Reply is already obtained"<<endl;
}

void
Lingvo::connected()
{
    qDebug() << "connected...";

    // Hey server, tell me about you.
    fGotReply = false;
    socket->write(("https://www.lingvolive.com/en-us/translate/de-ru/" + fQuery).toUtf8());
    const char endLine[]="\nEOF\n";
    socket->write(endLine);
}

void
Lingvo::disconnected()
{
    cout<<"Lingvo socket disconnected "<<endl;
    if(!fGotReply) emit ready(this);
    else cout<<"Reply is already obtained"<<endl;
}

void
Lingvo::bytesWritten(qint64 bytes)
{
    qDebug() << "Lingvo "<<bytes << " bytes written...";
}

void
Lingvo::readyRead()
{
    const char endLine[]="\nEOF\n";
    if(socket->state() != QAbstractSocket::ConnectedState)
    {
        cout<<"Lingvo::readyRead: bad socket state "<<socket->state()<<endl;
        emit ready(this);
        return;
        
    }
    //READALL

    fGotReply = true;

    QString readStr = socket->readAll();
    fHTML += readStr; 

    cout << "READYREAD "<<endl<<readStr<< endl;

    ofstream ofsh("Lingvo.html");
    ofsh << fHTML << endl;

    if(fHTML.contains(endLine))
    {
        cout<<"Found endLine "<<endl;
        socket->disconnectFromHost();
        parseHTML();
    }
}

void
Lingvo::parseHTML()
{
    for(list<dictElementParser*>::iterator it = fCF.begin(); it != fCF.end(); it++)
        delete(*it);
    fCF.clear();

    auto lines = fHTML.split('\n',  QString::SkipEmptyParts); 
    vector<QString> articles;
    
    cout<<"Parse "<<lines.size()<<' '<<fQuery<<endl;
    
    // Dictionary articles
    int lastLine = 0; 
    while(1)
    {              
        int qLine = -1;        
        for(int il = lastLine; il < lines.size() && qLine < 0; il++)
        {
            if(lines[il].startsWith("Examples from texts") ||
                    lines[il].trimmed().startsWith("Unlock all free")) break;
            if(lines[il].startsWith(fQuery,Qt::CaseInsensitive)) qLine = il;
        }
         
        cout<<"qLine ="<<qLine<<':'<<(qLine>=0?lines[qLine]:"---")<<endl;
        if(qLine < 0) break;
        

        int startLine = - 1;
        for(int sl = qLine -1; sl >= lastLine && startLine < 0; sl--)
        {
            if(!lines[sl][0].isSpace()) startLine = sl;
        }
        
        if(startLine < 0) break;
                
        int endLine = lines.size();
        for(int el = qLine + 1; 
                el < lines.size() && endLine == lines.size(); el++)
        {
            if(lines[el].trimmed().startsWith("Unlock all free") ||
                    !lines[el][0].isSpace()) endLine = el;
        }
     
        QString art;
        for(int il = startLine; il < endLine; il++)
            art += lines[il] + '\n';
        
        if(art.startsWith("The Comprehensive German-Russian Dictionary"))
            articles.push_back(art);
        
        cout<<"Parse article #"<<articles.size()<<":"<<endl<<art<<endl;
        lastLine = endLine;
    }

    if(articles.empty())
    {
        cout<<"No articles found"<<endl;
        emit ready(this);
        return;
    }

    
    QString forms;
    //Word forms
    int fLine = -1;
    for(int el = lastLine; el < lines.size() && fLine < 0; el++)
    {
        if(lines[el][0].isSpace()) continue;

        if(lines[el].startsWith("Word forms")) fLine = el;
        else if(lines[el].startsWith("References")) break;
    }
    
    if(fLine > 0) 
    {
        int endLine = lines.size();
        for(int el = fLine; el < lines.size() && endLine == lines.size(); el++)
        {
            if(lines[el].trimmed().startsWith("Translate "+fQuery+" to:"))
                endLine = el;
            else if(lines[el].startsWith("References")) break;
        }
        
        for(int il = fLine; il < endLine; il++)
            forms += lines[il] + '\n';
    }
    cout<<"Forms: "<<fLine<<':'<<forms<<endl;
  
   for (auto & art: articles){
      cout<<"Lingvo ---------------- "<<endl;
      fCF.push_back(new LingvoCardParser(this));
      fCF.back()->parseElement(art+forms);
   }

    cout << "******** MATCH **************** " << endl;
    TDictEntry *me = getMatch();
    if(me) cout << *me << endl;

    //   qApp->exit(0);
    cout << "Linvo done" << endl;

}

void
Lingvo::parserDone()
{
    cout << "ParserDone Lingvo" << endl;
    for(list<dictElementParser*>::iterator it = fCF.begin(); it != fCF.end(); it++)
    {
        cout << "PD " << (*it)->isDone() << endl;
        if(!(*it)->isDone()) return;
    }

    for(auto el : fCF)
    {
        LingvoCardParser * p = dynamic_cast<LingvoCardParser *>(el);

        if(p->fDictName == "Universal")
        {
            for(auto el2 : fCF)
            {
                if(el == el2) continue;

                LingvoCardParser * pr = dynamic_cast<LingvoCardParser *>(el2);
                if(pr->fDictName != "Universal" &&
                    pr->fEnt.words().first->data() == p->fEnt.words().first->data())
                    pr->setOK(false);
            }
        }
    }

    cout << "AllPD!" << fCF.size() << endl;
    emit ready(this);
}


///////////////////////////////////////////////////
void
LingvoCardParser::parseElement(QString art)
{
    fDone = true;

    auto lines = art.split('\n',  QString::SkipEmptyParts); 
    if(lines.size() == 0) 
    {
        emit ready();
        return;
    }

    
    fDictName = lines[0];

    cout << "DictName: " << fDictName << endl;
    
    int qLine = -1;
    for(int il = 1; il < lines.size() && qLine < 0; il++)
    {
        if(!lines[il][0].isSpace()) qLine = il;
        if(lines[il].startsWith("Word forms")) break;
    }
    
    // Translated word
    if(qLine < 0 || qLine == lines.size() - 1)
    {
        emit ready();
        return;
    }

    QString query;
    for(int ic = 0; ic < lines[qLine].length(); ic++ )
    {
        if(lines[qLine][ic].isLetter()) query+=lines[qLine][ic];
        else break;
    }
    
    // Part of speech
    QString posLine = lines[qLine+1];
    QString posStr;
    for(int ic = 0; ic < posLine.length(); ic++ )
    {
        if(!posLine[ic].isLetter() && posStr.isEmpty()) continue;
        if(!posLine[ic].isSpace()) posStr += posLine[ic];
    }
    
    if(posStr[0]=='v') fEnt.words().first->setPartOfSpeech(TWord::eVerb);
    else if(posStr == "a") fEnt.words().first->setPartOfSpeech(TWord::eAdjective);
    else if(posStr[0] == 'm' || posStr[0] == 'n' || posStr[0] == 'f') 
    { //Noun
        fEnt.words().first->setPartOfSpeech(TWord::eNoun);
        int gen;
        if(posStr[0] == 'm') gen=TWord::eMasculine;
        else if(posStr[0] == 'f') gen=TWord::eFeminine;
        else gen=TWord::eNeutral;
        fEnt.words().first->setNounGender(gen);
        
        int indLt = posStr.indexOf('<');
        int indGt = posStr.indexOf('>');
        if(indLt >= 0 && indGt >= 0)
        fEnt.words().first->setForms(str(posStr.mid(indLt+1,indGt - indLt -1 )));
    }
    
    
//    QWebElement head = e.findFirst("h2>span.Bold");
//    cout << "LWord " << head.toInnerXml() << endl;
//    fEnt.words().first->setData(str(head.toInnerXml()));
//    QWebElement gram = e.findFirst("p.P1").findFirst("span.l-article__abbrev[title]");
//
//    fEnt.words().first->setPartOfSpeech(TWord::eOther);
//    cout << "Gram " << gram.attribute("title") << '|' << gram.toPlainText()
//        << "|" << gram.attribute("title").contains("Maskulinum") << endl;
//    if(gram.attribute("title").contains("Femininum") ||
//        gram.attribute("title").contains(QString::fromUtf8("женский род")) ||
//        (gram.attribute("title").isEmpty() &&
//            gram.toPlainText() == "f"))
//    {
//        fEnt.words().first->setPartOfSpeech(TWord::eNoun);
//        fEnt.words().first->setNounGender(TWord::eFeminine);
//    }
//    else if(gram.attribute("title").contains("Maskulinum") ||
//        gram.attribute("title").contains(QString::fromUtf8("мужской род")) ||
//        (gram.attribute("title").isEmpty() &&
//            gram.toPlainText() == "m"))
//    {
//        cout << "LFound m" << endl;
//        fEnt.words().first->setPartOfSpeech(TWord::eNoun);
//        fEnt.words().first->setNounGender(TWord::eMasculine);
//    }
//    else if(gram.attribute("title").contains("Neutrum") ||
//        gram.attribute("title").contains(QString::fromUtf8("средний род")) ||
//        (gram.attribute("title").isEmpty() &&
//            gram.toPlainText() == "n"))
//    {
//        fEnt.words().first->setPartOfSpeech(TWord::eNoun);
//        fEnt.words().first->setNounGender(TWord::eNeutral);
//    }
//    else if(gram.attribute("title").contains("Verb"))
//    {
//        fEnt.words().first->setPartOfSpeech(TWord::eVerb);
//    }
//    fEnt.words().second->setPartOfSpeech(fEnt.words().first->partOfSpeech());
//
//    if(fEnt.words().first->partOfSpeech() == TWord::eNoun)
//    {
//        QWebElement forms =
//            e.findFirst("p.P1").findFirst("span.l-article__abbrev[title] + span[class][style]");
//        //      cout<<"Forms "<<forms.isNull()<<' '<<forms.toInnerXml()<<endl;
//        QString v = forms.toInnerXml().trimmed();
//        if(v.startsWith("&lt;")) v = v.mid(4);
//        if(v.endsWith("&gt;")) v = v.mid(0, v.count() - 4);
//
//        cout << e.findFirst("p.P1").toInnerXml() << endl;
//        fEnt.words().first->setForms(str(v));
//    }
//
//    QString tr;
//    QWebElementCollection trans = e.findAll("p.P1");
//    int ntrans = 0;
//    for(int ip = 0; ip < trans.count(); ip++)
//    {
//        if(!trans[ip].findFirst("span.translation").isNull())
//        {
//
//            QWebElementCollection span = trans[ip].findAll("span[class]");
//            if(span.count() > 0)
//            {
//                if(ntrans++ >= 3) break;
//            }
//
//            if(tr.count()) tr += " ";
//            QWebElement href = trans[ip].firstChild();
//            tr += trans[ip].toInnerXml().mid(0, trans[ip].toInnerXml().indexOf(href.toOuterXml()));
//
//            for(int is = 0; is < span.count(); is++)
//            {
//                QString add;
//                if(span[is].attribute("class").contains("translation"))
//                {
//                    add = span[is].toPlainText();
//                }
//                else if(span[is].attribute("class").contains("l-article__abbrev") ||
//                    span[is].attribute("class").contains("l-article__comment"))
//                {
//                    add = span[is].toPlainText();
//                    if(ip == 0 && span[is].hasAttribute("title")) continue;
//                    if(!add.startsWith("(")) add = "(" + add;
//                    if(!add.endsWith(")")) add += ")";
//                }
//                add = add.trimmed();
//                if(tr.count() && !add.startsWith(",") && !add.startsWith(";") &&
//                    !tr.endsWith(" ")) tr += " ";
//                if((add.startsWith(",") || add.startsWith(";")) &&
//                    tr.endsWith(" ")) tr = tr.mid(0, tr.count() - 1);
//                tr += add;
//            }
//        }
//    }
//    fEnt.words().second->setData(str(tr.trimmed()));
    if(fEnt.words().first->partOfSpeech() == TWord::eVerb) fetchVerbForms();
    else
    {
        cout << fEnt << endl;
        fDone = true;
        emit ready();
    }
}

void
LingvoCardParser::fetchVerbForms()
{
    QString req = "http://www.verbformen.com/conjugation/" +
        qstr(fEnt.words().first->data()) + ".htm";
//    fReply = OnlineDict::networkManager()->get(QNetworkRequest(QUrl(req)));
//    connect(fReply, SIGNAL(finished()), this, SLOT(replyFinished()));
    cout << "VerbForms Start " << req << endl;
}

void
LingvoCardParser::replyFinished()
{
    cout << "VerbForms reply finished" << endl;
    fDone = true;
//    QString HTML = QString::fromUtf8(fReply->readAll().constData());
//    fReply->disconnect();
//    fReply->deleteLater();
//
//    ofstream ofs("VerbForms.html");
//    ofs << HTML << endl;
//
//    QWebPage wpage;
//    wpage.mainFrame()->setHtml(HTML);
//    QWebElement doc = wpage.mainFrame()->documentElement();
//    QWebElement unbekant = doc.findFirst("div.einstellung-warnhinweis");
//    cout << "Unbek " << unbekant.isNull() << '|' << unbekant.toPlainText() << '|' << endl;
//    if(!unbekant.isNull() && unbekant.toPlainText().contains("verbtabelle.unbekanntes_verb"))
//    {
//        emit ready();
//        return;
//    }
//    QString title = doc.findFirst("head").findFirst("title").toPlainText();
//    title = title.section("-", 1, 2).trimmed();
//    cout << "Title :" << title << endl;
//    fEnt.words().first->setForms(str(title));
    cout << "With verbFormen:" << fEnt << endl;
    emit ready();
}

