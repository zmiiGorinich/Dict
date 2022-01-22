#include"TWord.h"
using namespace std;

/////////////////////////////////////////////////////////////////
void
TWord::print(std::ostream & ost) const
{
    ost << "Lang: " << fLang
        << " PartOfSpeach: " << fPartOfSpeech
        << " GammarInfo: ^" << fGrammarInfo << "^"
        << " Data: ^" << fData << "^"
        << " Forms: ^" << fForms << "^";
}


/////////////////////////////////////////////////////////////////
void
TWord::read(std::istream & ist)
{
    string str;
    ist >> str >> fLang >> str >> fPartOfSpeech >> str;
    getline(ist, str, '^');
    getline(ist, str, '^');
    fGrammarInfo = str;
    ist >> str;
    getline(ist, str, '^');
    getline(ist, str, '^');
    fData = str;
    ist >> str;
    getline(ist, str, '^');
    getline(ist, str, '^');
    fForms = str;

}


/////////////////////////////////////////////////////////////////
vector<string>
TWord::showTable() const
{
    vector<string> ret(3);
    ret[1] = fData;
    if(fLang == eDe)
    {
        if(fPartOfSpeech == eNoun)
        {
            ret[0] = nounArticle();
        }
        else if(fPartOfSpeech == eVerb)
        {
            ret[0] = fGrammarInfo;
        }
        ret[2] = fForms;
    }

    return ret;
}


/////////////////////////////////////////////////////////////////
string
TWord::show() const
{
    if(fLang != eDe) return fData;

    string str;
    if(fPartOfSpeech == eNoun)
    {
        if(!nounArticle().empty())
            str += nounArticle() + " ";
        str += fData;
        str += "  ";
        str += fForms;
    }
    else if(fPartOfSpeech == eVerb)
    {
        if(!fGrammarInfo.empty()) str += fGrammarInfo + "  ";
        str += fData;
        if(!fForms.empty())
        {
            str += "  ";
            if(fForms[0] != '(') str += "(";
            str += fForms;
            if(fForms[fForms.length() - 1] != ')') str += ")";
        }
    }
    else str = fData + (fForms.empty() ? "" : " " + fForms);
    return str;
}


/////////////////////////////////////////////////////////////////
int
TWord::nounGender() const
{
    if(fPartOfSpeech == eNoun)
    {
        if(!fGrammarInfo.empty())
        {
            if(fGrammarInfo[0] == 'f') return eFeminine;
            if(fGrammarInfo[0] == 'm') return eMasculine;
            if(fGrammarInfo[0] == 'n') return eNeutral;
        }
        else return -1;
    }
    return -1;
}


/////////////////////////////////////////////////////////////////
void
TWord::setNounGender(int g)
{
    if(!fPartOfSpeech == eNoun) return;
    int cg = nounGender();
    if(cg == g) return;
    string l;
    switch(g)
    {
        case eMasculine: l = "m"; break;
        case eFeminine:  l = "f"; break;
        case eNeutral:   l = "n"; break;
        default: return;
    }
    if(!fGrammarInfo.empty() && cg >= 0)
    {
        fGrammarInfo[0] = l[0];
    }
    else fGrammarInfo = l + fGrammarInfo;
}


/////////////////////////////////////////////////////////////////
std::string
TWord::nounArticle() const
{
    switch(nounGender())
    {
        case eMasculine: return "der"; break;
        case eFeminine:  return "die"; break;
        case eNeutral:   return "das"; break;
    }
    return "";
}


/////////////////////////////////////////////////////////////////
bool
TWord::operator==(const TWord&w)
{
    if(fLang != w.fLang) return false;
    if(fPartOfSpeech != w.fPartOfSpeech) return false;
    if(fGrammarInfo != w.fGrammarInfo) return false;
    if(fForms != w.fForms) return false;
    if(fLang != w.fLang) return false;
    if(fData != w.fData) return false;
    return true;
}
