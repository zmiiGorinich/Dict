#ifndef TWORD_H
#define TWORD_H
#include<string>
#include<vector>
#include<iostream>
class TWord
{
   std::string fData;
   std::string fForms;
   std::string fGrammarInfo;
   int fPartOfSpeech;
   int fLang;
public:
   enum {eMasculine, eFeminine, eNeutral};
   enum {eRu, eDe, eMaxLang};
   enum {eNoun, eVerb, eAdjective, eOther, eMaxPartOfSpeech};
   const std::string & data() const { return fData;}
   int partOfSpeech() const {return  fPartOfSpeech;}
   int language() const {return fLang;}
   void setData(const std::string &v) { fData = v;}
   void setPartOfSpeech(int i) { fPartOfSpeech = (i >= 0 && i < eMaxPartOfSpeech ? i : eOther);}
   void setLanguage(int i) { fLang = (i >= 0 && i < eMaxLang ? i : eRu);}
   void setForms(const std::string &v){ fForms=v;}
   const std::string& forms() const{ return fForms;}
   void setGrammarInfo(const std::string &v){ fGrammarInfo=v;}
   const std::string& grammarInfo() const{ return fGrammarInfo;}  
   int nounGender() const;
   void setNounGender(int g);
   std::string nounArticle() const; 
   virtual void print(std::ostream &) const;
   virtual void read(std::istream &);
   virtual std::string show() const;
   std::vector<std::string> showTable() const;
   virtual TWord * clone() const;
   friend std::ostream & operator<<(std::ostream &, const TWord&);
   friend std::istream & operator>>(std::istream &, TWord&);
   bool operator ==(const TWord&);

   virtual ~TWord(){}
};

inline std::ostream & operator<<(std::ostream &s, const TWord& w)
{
   w.print(s);
   return s;
}
inline std::istream & operator>>(std::istream &s, TWord&w)
{
   w.read(s);
   return s;
}
inline TWord * TWord::clone() const{ return new TWord(*this);}
#endif
