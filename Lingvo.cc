#include"Lingvo.h"
#include<iostream>
#include<fstream>
#include<limits>
#include<algorithm>
#include<QtWidgets/QApplication>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include"StrStr.h"
#include"TWord.h"
#include <QStringList>
#include <qt5/QtCore/qchar.h>
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
    cout << "Lingvo socket error " << socketError << endl;
    if(!fGotReply) emit ready(this);
    else cout << "Reply is already obtained" << endl;
}

void
Lingvo::connected()
{
    qDebug() << "connected...";

    // Hey server, tell me about you.
    fGotReply = false;
    socket->write(("https://www.lingvolive.com/en-us/translate/de-ru/" + fQuery).toUtf8());
    const char endLine[] = "\nEOF\n";
    socket->write(endLine);
}

void
Lingvo::disconnected()
{
    cout << "Lingvo socket disconnected " << endl;
    if(!fGotReply) emit ready(this);
    else cout << "Reply is already obtained" << endl;
}

void
Lingvo::bytesWritten(qint64 bytes)
{
    qDebug() << "Lingvo " << bytes << " bytes written...";
}

void
Lingvo::readyRead()
{
    const char endLine[] = "\nEOF\n";
    if(socket->state() != QAbstractSocket::ConnectedState)
    {
        cout << "Lingvo::readyRead: bad socket state " << socket->state() << endl;
        emit ready(this);
        return;

    }
    //READALL

    fGotReply = true;

    QString readStr = socket->readAll();
    fHTML += readStr;

    cout << "READYREAD " << endl << readStr << endl;

    ofstream ofsh("Lingvo.html");
    ofsh << fHTML << endl;

    if(fHTML.contains(endLine))
    {
        cout << "Found endLine " << endl;
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

    cout << "Parse " << lines.size() << ' ' << fQuery << endl;

    // Dictionary articles
    int lastLine = 0;
    while(1)
    {
        int queryLine = -1;
        // Find a line beginning with the query word
        for(int il = lastLine; il < lines.size() && queryLine < 0; il++)
        {
            if(lines[il].startsWith("Examples from texts") ||
                lines[il].trimmed().startsWith("Unlock all free")) break;
            if(lines[il].startsWith(fQuery, Qt::CaseInsensitive)) queryLine = il;
        }

//        cout << "qLine =" << queryLine << ':' << (queryLine >= 0 ? lines[queryLine] : "---") <<
//            endl;
        if(queryLine < 0) break;

        // Search back for a line not starting with a whitespace -> Dict Name
        int dictLine = - 1;
        for(int sl = queryLine - 1; sl >= lastLine && dictLine < 0; sl--)
        {
            if(!lines[sl][0].isSpace()) dictLine = sl;
        }
        if(dictLine < 0) break;

        // Find past-the-end of the translation block
        int endLine = lines.size();
        for(int el = queryLine + 1;
            el < lines.size() && endLine == lines.size(); el++)
        {
            if(lines[el].trimmed().startsWith("Unlock all free") ||
                !lines[el][0].isSpace()) endLine = el;
        }

        // Break the translation block into articles
        QString head, art;
        head += lines[dictLine]  + '\n';
        head += lines[queryLine]  + '\n';

        int posOffset = std::numeric_limits<int>::max();

        for(int iLine = queryLine + 1; iLine < endLine; ++iLine)
        {
            QString &line = lines[iLine];

            // Find first letter
            int idx = 0;
            for(; idx < line.length(); ++idx)
            {
                if(line[idx].isLetter()) break;
            }

            if(idx == line.length()) continue;

            bool isLatin = (line.at(idx) == QChar(line.at(idx).toLatin1()));
//            cout<<line<<" --> "<<line.at(idx)<<' '<<idx<<' '<<posOffset<<' '<<isLatin<<endl;
            if(idx > posOffset || !isLatin)
            {
                // First letter is cyrillic or the string is right shifted -
                // Add line to the article
                art += line   + '\n';
//                cout<<"Line |"<<line<<"| added "<<endl;
                continue;
            }

//            cout<<"Latin !"<<idx<<' '<<posOffset<<' '<<isLatin<<' '<<art.length()<<endl;
            // First letter is latin1 - Start of a new article
            if(!art.isEmpty())
            {
                articles.push_back(head + art);
                cout << "Parse article #" << articles.size() << ":" << endl << articles.back() << endl;
            }
            art = line + '\n';
            posOffset = idx;
        }

        // Leftover article
        if(!art.isEmpty())
        {
            articles.push_back(head + art);
            cout << "Leftover article #" << articles.size() << ":" << endl << articles.back() << endl;
        }

        lastLine = endLine;
    }

    if(articles.empty())
    {
        cout << "No articles found" << endl;
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
            if(lines[el].trimmed().startsWith("Translate " + fQuery + " to:"))
                endLine = el;
            else if(lines[el].startsWith("References")) break;
        }

        for(int il = fLine; il < endLine; il++)
            forms += lines[il] + '\n';
    }

    for(auto & art : articles)
    {
        cout << "Lingvo ---------------- " << endl;
        fCF.push_back(new LingvoCardParser(this));
        fCF.back()->parseElement(art + forms);
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
    for(auto ep : fCF)
    {
        cout << "PD " << ep->isDone() << endl;
        if(!ep->isDone()) return;
    }


    cout << "AllPD!" << fCF.size() << endl;
    emit ready(this);
}

///////////////////////////////////////////////////
QString
LingvoCardParser::parsePartOfSpeach(const QString & posLine)
{

    int pos0 = -1, tran0 = 0;
    for(; tran0 < posLine.length(); ++tran0)
    {
        auto ch = posLine.at(tran0);
        if(ch.isLetter())
        {
            bool isLatin = (ch == QChar(ch.toLatin1()));
            if(!isLatin) break;

            if(pos0 < 0) pos0 = tran0;
        }
    }

    QString retStr = posLine.mid(tran0);

    if(pos0 < 0) return retStr;

    QString posStr = posLine.mid(pos0, tran0 - pos0);

    if(posStr[0] == 'v') fEnt.de()->setPartOfSpeech(TWord::eVerb);
    else if(posStr == 'a') fEnt.de()->setPartOfSpeech(TWord::eAdjective);
    else if(posStr[0] == 'm' || posStr[0] == 'n' || posStr[0] == 'f')
    {
        //Noun
        fEnt.de()->setPartOfSpeech(TWord::eNoun);

        int gen;
        if(posStr[0] == 'm') gen = TWord::eMasculine;
        else if(posStr[0] == 'f') gen = TWord::eFeminine;
        else gen = TWord::eNeutral;
        fEnt.de()->setNounGender(gen);

        int indLt = posStr.indexOf('<');
        int indGt = posStr.indexOf('>');
        if(indLt >= 0 && indGt >= 0)
            fEnt.de()->setForms(str(posStr.mid(indLt + 1, indGt - indLt - 1)));
    }
    cout<<"************* POS "<<fEnt.de()->partOfSpeech()<<endl;
    fEnt.ru()->setPartOfSpeech(fEnt.de()->partOfSpeech());
    return retStr;
}

///////////////////////////////////////////////////
int
LingvoCardParser::parseWordForms(const QStringList &lines)
{
    int formLine = 0;
    for(; formLine < lines.size(); ++formLine)
    {
        if(lines[formLine].startsWith("Word forms")) break;
    }

    int ret = formLine;
    if(ret == lines.size()) return ret;


    std::vector<QStringList> forms;
    QStringList form;

    while(++formLine < lines.size())
    {
        auto &line = lines[formLine];
        if(line.isEmpty()) continue;

        if(line.startsWith(fEnt.de()->data().c_str(),
                Qt::CaseInsensitive))
        {
            // New Word form entry
            if(!form.isEmpty()) forms.push_back(std::move(form));
            form.clear();
        }
        else
        {
            form += line;
        }
    }

    if(!form.isEmpty()) forms.push_back(std::move(form));

    if(fEnt.de()->partOfSpeech() != TWord::eVerb)  return ret;

    for(const auto & form : forms)
    {
        if(form.isEmpty()) continue;
        if(!form[0].trimmed().startsWith("Verb")) continue;

        int idxPras = -1, idxPrat = -1, idxPerf = -1;
        for(int il = 0; il < form.size(); ++il)
        {
            if(form[il].contains("Indikativ, Präsens, Aktiv")) idxPras = il;
            if(form[il].contains("Indikativ, Präteritum, Aktiv")) idxPrat = il;
            if(form[il].contains("Indikativ, Perfekt, Aktiv")) idxPerf = il;
        }

        if(idxPras < 0 || idxPrat < 0 || idxPerf < 0) continue;

        QString prasStr, pratStr, perfStr;

        const int lineOffset = 3; // --> du
        const QString ss0 = " er/sie/es ", ss1 = " sie ";
        //Präsens, du
        {
            idxPras += lineOffset;
            if(idxPras >= form.size()) continue;

            const auto &line = form[idxPras];

            int idx0 = line.indexOf(ss0);
            if(idx0 < 0) continue;
            idx0 += ss0.length();

            int idx1 = line.indexOf(ss1, idx0);
            if(idx1 < 0) continue;

            prasStr = line.mid(idx0, idx1 - idx0).trimmed();
        }

        //Präteritum, er/sie/es
        {

            idxPrat += lineOffset;
            if(idxPrat >= form.size()) continue;

            const auto &line = form[idxPrat];

            int idx0 = line.indexOf(ss0);
            if(idx0 < 0) continue;
            idx0 += ss0.length();

            int idx1 = line.indexOf(ss1, idx0);
            if(idx1 < 0) continue;

            pratStr = line.mid(idx0, idx1 - idx0).trimmed();
        }

        //Perfect, er/sie/es
        {
            idxPerf += lineOffset;
            if(idxPerf >= form.size()) continue;

            const auto &line = form[idxPerf];

            int idx0 = line.indexOf(ss0);
            if(idx0 < 0) continue;
            idx0 += ss0.length();

            int idx1 = line.indexOf(ss1, idx0);
            if(idx1 < 0) continue;

            perfStr = line.mid(idx0, idx1 - idx0).trimmed();
        }

        auto formStr = prasStr + ",  " + pratStr + " - " + perfStr;
        fEnt.de()->setForms(str(formStr));
    }

    return ret;
}

///////////////////////////////////////////////////
void
LingvoCardParser::parseTranslations(QStringList & lst)
{
    QStringList val;
    const auto maxTrans = 10;

    // Join list to one line, taking care of brackets
    QString str;
    bool contd = false;
    for(int il = 0; il < lst.size(); ++il)
    {
        lst[il] = lst[il].trimmed();
        if(lst[il].isEmpty()) continue;
        
        str += lst[il];
        if(contd) contd = false;

        int idxOpenBracket =  lst[il].lastIndexOf('(');
        int idxCloseBracket = lst[il].lastIndexOf(')');

        if(idxCloseBracket < idxOpenBracket) contd = true;

        if(!contd && il + 1 < lst.size()) str += ";";
        str += " ";
    }
    cout << "Line = " << str << endl;

    //=========================================================================
    // Break line contents into translation units
    
    int pos = 0;
    while(pos < str.length())
    {
        int nextIdx;

        //do not look for commas, semicols inside brackets;
        int searchPos = pos;

        while(1)
        {
            unsigned int idxComma   = str.indexOf(',', searchPos);
            unsigned int idxSemicol = str.indexOf(';', searchPos);
            unsigned int idxOpenBracket = str.indexOf('(', searchPos);

            nextIdx =
            std::min({idxOpenBracket, idxSemicol, idxComma, (unsigned int) str.length()});

            if(nextIdx != (int) idxOpenBracket) break;

            idxOpenBracket = std::min(idxOpenBracket, (unsigned int) str.length());
            unsigned int idxCloseBracket = str.indexOf(')', idxOpenBracket);
            searchPos = std::min(idxCloseBracket, (unsigned int) str.length());
        }

        cout << "CommaPos " << nextIdx << endl;

        // Add a translation
        auto appStr = str.mid(pos, nextIdx - pos).trimmed();

        // remove starting number with .
        int idx = 0;
        while(idx < appStr.length() &&
            (appStr[idx].isDigit() || appStr[idx] == '.')) idx++;

        appStr = appStr.mid(idx).trimmed();
        if(!appStr.isEmpty())
        {
            val.append(appStr);
            cout << "Append " << appStr << endl;
        }

        // Add a separator
        if(nextIdx < str.length())
        {
            val.append(str.mid(nextIdx, 1));
            cout << "Addsep " << str.mid(nextIdx, 1) << endl;
        }

        pos = nextIdx + 1;
    }

//===================================================================================
    cout << "Join Trans " << endl;
    auto nTrans = 0;
    QString translation;
    for(int iv = 0; iv < val.size() && nTrans < maxTrans; ++iv)
    {
        cout << "Item " << iv << ' ' << val[iv] << endl;
        if(val[iv] == "," || val[iv] == ";")
        {
            translation += val[iv] + " ";
            cout << "Add sep " << val[iv] << " --> " << translation << endl;
        }
        else
        {
            int idx = val.indexOf(QRegExp{val[iv], Qt::CaseInsensitive, QRegExp::FixedString});
            if(idx < iv)
            {
                cout << "Skip " << val[iv] << ' ' << idx << endl;
                ++iv; // skip the following sep. too
            }
            else
            {
                translation += val[iv];
                nTrans++;
                cout << "Add " << val[iv] << " --> " << translation << endl;
            }
        }
    }


    fEnt.ru()->setData(::str(translation));
    cout << "End Join Trans " << translation << endl;
}

///////////////////////////////////////////////////
void
LingvoCardParser::parseElement(QString art)
{
    fDone = true;

    // Break article into lines
    auto lines = art.split('\n',  QString::SkipEmptyParts);
    // Format of an article:
    // Line 0:  Dictionary name
    // Line 1:  query
    // translation block
    // a line starting with "Word forms"
    // word forms block

    if(lines.size() < 2)
    {
        emit ready();
        return;
    }


    fDictName = lines[0];

    cout << "DictName: " << fDictName << endl;

    int qLine = 1;

    // Extract query
    QString query;
    for(int ic = 0; ic < lines[qLine].length(); ic++)
    {
        if(lines[qLine][ic].isLetter()) query += lines[qLine][ic];
        else break;
    }
    fEnt.de()->setData(str(query));

    // Part of speech, translation
    QString posLeftOver = parsePartOfSpeach(lines[qLine + 1]);

    // Starke Verben testen

    if(fEnt.de()->partOfSpeech() == TWord::eVerb)
    {
        int idx = lines[qLine].indexOf(fEnt.de()->data().c_str(), Qt::CaseInsensitive);

        if(idx >= 0 && idx + (int)fEnt.de()->data().length() < lines[qLine].length())
        {
            if(lines[qLine].at(idx + fEnt.de()->data().length()) == '*')
            {
                fEnt.de()->setGrammarInfo("*");
            }
        }
    }

    // Extract forms
    int formLine = parseWordForms(lines);

    // Extract translations
    QStringList translation;
    if(!posLeftOver.isEmpty()) translation += posLeftOver;
    translation += lines.mid(qLine + 2, formLine - (qLine + 2));
    parseTranslations(translation);


    
    cout << fEnt << endl;
    fDone = true;
    emit ready();
}

void
LingvoCardParser::fetchVerbForms()
{
    QString req = "http://www.verbformen.com/conjugation/" +
        qstr(fEnt.de()->data()) + ".htm";
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
//    fEnt.de()->setForms(str(title));
    cout << "With verbFormen:" << fEnt << endl;
    emit ready();
}

