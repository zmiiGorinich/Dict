#ifndef TDICT_H
#define TDICT_H
#include<utility>
#include<time.h>
#include<string>
#include<vector>
#include<list>
#include<vector>
#include<bitset>
#include"TWord.h"
#include<algorithm>
class TDictEntry
{
   std::pair<TWord *, TWord *> fW;
   int fNumTries;
   int fNumSuccess;
   time_t fLastTryTime;
   std::vector<std::string> fTag;
   std::bitset<64> fHistory;
   double fHistoryValue;
public:

   TDictEntry() ;
   TDictEntry(const TDictEntry&);
   void setWords(const TWord & w1, const TWord & w2);
   const std::pair<TWord *, TWord *> & words() const{ return fW;}
   TWord * de() const { return fW.first;}
   TWord * ru() const { return fW.second;}
   void setTags(const std::string & a) ;
   float rate() const { return (fNumTries > 0 ? float(fNumSuccess) / fNumTries : 0);}
   int nSuccess() const {return fNumSuccess;}
   int nTry() const {return fNumTries;}
   std::string  tags() const;
   const std::vector<std::string> & tagVector() const;
   bool wildCardTagCompare(const std::string & mask);
   bool wildCardTagVecCompare(const std::vector<std::string> &vec);
   time_t lastTryTime() const { return fLastTryTime;}
   void addAttempt(bool ok, const time_t & atime) ;
   void updateHistoryValue();
   double historyValue() const { return fHistoryValue;}
   std::bitset<64> history() const { return fHistory;}
   virtual void print(std::ostream &) const;
   virtual void read(std::istream &);
   virtual TDictEntry * clone() const;
   friend std::ostream & operator<<(std::ostream &, const TDictEntry&);
   friend std::istream & operator>>(std::istream &, TDictEntry&);
   virtual ~TDictEntry();
   TDictEntry & operator=(TDictEntry);
   friend void swap(TDictEntry&, TDictEntry&);
   bool operator ==(const TDictEntry&);

};
inline TDictEntry * TDictEntry::clone() const{ return new TDictEntry(*this);}

inline  const std::vector<std::string> &TDictEntry::tagVector() const{
   return fTag;
}
inline TDictEntry::~TDictEntry(){
   delete fW.first;
   delete fW.second;
}
inline std::ostream & operator<<(std::ostream &s, const TDictEntry& w)
{
   w.print(s);
   return s;
}
inline std::istream & operator>>(std::istream &s, TDictEntry&w)
{
   w.read(s);
   return s;
}

class TDict
{
   std::vector<TDictEntry *> fEntries;
   std::list<std::string> fTags;
   int fRevision;

public:
   void addEntry(const TWord & w1, const TWord & w2, const std::string & tag);
   void addEntry(TDictEntry *e);
   bool haveEntry(const TDictEntry *e) const{
      return (std::find(fEntries.begin(),fEntries.end(),e)!=fEntries.end());
   }
   TDictEntry * getRandom(const std::string & tag="") const;
   TDictEntry * getNext(const std::string & tag="",const TDictEntry * cur=NULL, bool fwd=true, bool repeat=true);

   void addAttempt( TDictEntry *, bool ok);
   double weight(int maxS, time_t maxdt, time_t ct, const TDictEntry *e) const;
   double weightHistory(double minH, double maxH, time_t maxdt, time_t ct, const TDictEntry *e);
   TDictEntry * getRandomHistory(const std::string & tag, const TDictEntry * prev = NULL) ;
   std::vector<TDictEntry *> selectTag(const std::string & tag) const;

   TDictEntry *find(const std::string &) const;
   const std::list<std::string> & tags() const;
   std::list<std::string> nearMatch(const std::string &) const;
   std::vector<TDictEntry *> & entries() { return fEntries;}

   virtual void print(std::ostream &) const;
   virtual void read(std::istream &) ;
   TDict(): fRevision(0){}
   TDict(const TDict &d);
   TDict & operator=(TDict d);
   friend void swap(TDict&, TDict&);

   int revision() const;
   void setRevision(int);
   virtual ~TDict(){std::cout<<"TDict DTOR "<<std::endl;}

   bool operator ==(const TDict&);
   friend std::ostream & operator<<(std::ostream &, const TDict&);
   friend std::istream & operator>>(std::istream &, TDict&);
};
inline std::ostream & operator<<(std::ostream &s, const TDict& w)
{
   w.print(s);
   return s;
}
inline std::istream & operator>>(std::istream &s, TDict&w)
{
   w.read(s);
   return s;
}
inline const std::list<std::string> & TDict::tags() const{ return fTags;}
inline int TDict::revision() const{ return fRevision;}
inline  void TDict::setRevision(int i){ fRevision=i;}

TDictEntry * Fetch(const std::string& wort, bool force=false);
bool wildCardCompare(const std::string &mask,  const std::string &str,  
		     const size_t mpos=0, const size_t spos=0);

#endif
