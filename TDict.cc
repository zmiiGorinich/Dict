#include"TDict.h"
#include <stdlib.h>
#include<algorithm>
#include<cmath>
#include <stdio.h>
#include<iterator>
#include<numeric>
#include<iomanip>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include<string>
#include <string.h>
#include<vector>
#include<boost/algorithm/string.hpp>
#include<algorithm>
//#include <unistr.h>
using namespace icu;
using namespace std;
static void parseTags(const std::string & str, vector<string> & fTag) { 
    fTag.clear();
    size_t pos0=0, pos1=0;
    while((pos1=str.find(',',pos0))!=str.npos){
       if(pos1-pos0>0) fTag.push_back(boost::trim_copy(str.substr(pos0,pos1-pos0)));
       pos0=pos1+1;
    }
    if(pos0<str.size()) fTag.push_back(boost::trim_copy(str.substr(pos0)));
}

double TDict::weight(int maxS, time_t maxdt, time_t ct, const TDictEntry *e) const{
   time_t dt=0;
   time_t firstt=ct-maxdt;
   if(e->lastTryTime()>=firstt) dt=e->lastTryTime()-firstt;
   if(maxS==0) maxS=1;
   if(maxdt==0) maxdt=1;
   return exp(-e->nSuccess()*5/double(maxS))*exp(-dt/double(maxdt));
}

vector<TDictEntry *> TDict::selectTag(const std::string & tag) const{
   vector<TDictEntry *> ret;
   vector<string> vtag;
   parseTags(tag,vtag);
   for(auto ent: fEntries) {
      if(tag.empty() ||  ent->wildCardTagVecCompare(vtag)) ret.push_back(ent);
   }
   return ret;
}

TDictEntry * TDict::getRandom(const std::string & tag) const{
   vector<TDictEntry *> vent=selectTag(tag);

   if(vent.empty()) return NULL;
   int maxS=0;
   for(auto ent: vent){
      int ns=ent->nSuccess();
      if(ns>maxS) maxS=ns;
   }
   time_t maxdt=0, ct=time(NULL), dt;
   for(auto ent: vent){
      if(ent->lastTryTime()<=ct) dt=ct-ent->lastTryTime();
      else {
	 dt=ent->lastTryTime();
	 cerr<<"Bad time diff "<<ctime(&ct)<<ctime(&dt);
	 dt=0;
      }
      if(maxdt<dt) maxdt=dt;
   }
   

   
   vector<double> w;
   w.reserve(vent.size());
   for(auto ent: vent) {
      w.push_back(weight(maxS,maxdt,ct,ent));
   }
   vector<double> ww=w;
   partial_sum(w.begin(),w.end(),w.begin());
   // for(unsigned int i=0; i<w.size(); i++) {
   //    cout<<setprecision(4)<<fixed<<w[i]<<' '<<ww[i]<<' '<<ent[i]->words().second->data()<<endl;
   // }
   
   //   copy(w.begin(), w.end(), ostream_iterator<double>(cout," "));
   //   cout<<endl;
   double r=(double(random())/RAND_MAX)*w.back();
   vector<double>::iterator lb=lower_bound(w.begin(),w.end(),r);
   //   cout<<r<<' '<<distance(w.begin(),lb)<<endl;
   if(!w.empty() && lb==w.end()) { cerr<<"Bad LB! "<<endl; exit(0);}
   return vent[distance(w.begin(),lb)];
}

double TDict::weightHistory(double minH, double maxH, time_t maxdt, time_t ct, const TDictEntry *e){
   time_t dt=0;
   time_t firstt=ct-maxdt;
   if(e->lastTryTime()>=firstt) dt=e->lastTryTime()-firstt;
   if(maxdt==0) maxdt=1;

   if(maxH<=minH) return 1;
   
   return exp(-(e->historyValue()-minH)*4/(maxH-minH))*exp(-dt/double(maxdt));
}

TDictEntry * TDict::getRandomHistory(const std::string & tag, const TDictEntry * prev) {
   vector<TDictEntry *> vent=selectTag(tag);
   if(vent.empty()) return NULL;

   if(vent.size()>1 && prev){
      auto it=std::find(vent.begin(),vent.end(),prev);
      if(it!=vent.end()) vent.erase(it);
   }

   double minHist=1e14, maxHist=-1e14;
   for(auto ent:vent){
      if(ent->historyValue()<minHist) minHist=ent->historyValue();
      if(ent->historyValue()>maxHist) maxHist=ent->historyValue();
   }

   time_t maxdt=0, ct=time(NULL), dt;
   for(auto ent:vent){
      if(ent->lastTryTime()<=ct) dt=ct-ent->lastTryTime();
      else {
	 dt=ent->lastTryTime();
	 cerr<<"Bad time diff "<<ctime(&ct)<<ctime(&dt);
	 dt=0;
      }
      if(maxdt<dt) maxdt=dt;
   }
   

   
   vector<pair<double,TDictEntry *> > sorted;

   for(auto ent:vent) {
      double w=weightHistory(minHist,maxHist,maxdt,ct,ent);
      sorted.push_back(pair<double,TDictEntry *>(w,ent));
   }

   sort(sorted.begin(),sorted.end());

   vector<double> w;
   w.reserve(sorted.size());
   for(auto v: sorted) w.push_back(v.first);

   partial_sum(w.begin(),w.end(),w.begin());



   for( size_t i=0; i<sorted.size(); i++) {
      cout<<setprecision(4)<<i<<' '<<fixed<<w[i]<<' '<<sorted[i].first<<' '<<sorted[i].second->words().first->data()<<' '<<sorted[i].second->historyValue()
       	  <<' '<<bitset<10>(sorted[i].second->history().to_ulong())<<endl;
   }
   
   double r=(double(random())/RAND_MAX)*w.back();
   vector<double>::iterator lb=lower_bound(w.begin(),w.end(),r);
   cout<<r<<' '<<distance(w.begin(),lb)<<endl;
   if(!w.empty() && lb==w.end()) { cerr<<"Bad LB! "<<endl; exit(0);}
   return sorted[distance(w.begin(),lb)].second;
}

TDictEntry * TDict::getNext(const std::string & tag,const TDictEntry * cur, bool fwd, bool repeat) {
   // for(auto t: fTags){
   //    cerr<<"----------  Test \""<<t<<"\" for \""<<tag<<"\" -------------- ";
   //    bool cmp=wildCardCompare(tag,t);
   //    cerr<<(cmp?"OK":"Bad")<<endl;
   // }
   vector<string> vtag;
   parseTags(tag,vtag);
  
   vector<TDictEntry *> vent;
   for(auto ent: fEntries) {
      if(tag.empty()||ent->wildCardTagVecCompare(vtag)) {
	 if(cur==NULL) return ent;
	 vent.push_back(ent);
      }
   }
   if(vent.empty()) return NULL;

   auto it=std::find(vent.begin(),vent.end(),cur);
   if(it==vent.end()) return vent.front();
   if(fwd){
      it++;
      if(it==vent.end() && repeat) it=vent.begin();
   } else {
      if(it==vent.begin()){
	 it=vent.end();
	 if(repeat) it--;
      } else it--;
   }
   return (it!=vent.end()?*it:NULL);
}

void TDict::addAttempt( TDictEntry *e, bool ok) {
   e->addAttempt(ok,time(NULL));
};

void TDict::print(std::ostream &ost) const {
   cout<<"Dictionary: "<<fEntries.size()<<endl;
   ost<<"Revision: "<<fRevision<<endl;
   for(vector<TDictEntry *>::const_iterator f=fEntries.begin(), l=fEntries.end(); f!=l; f++)
      ost<<*(*f)<<endl<<endl;
};

void TDict::read(std::istream &ist)  {
   if(!ist) return;
   string str;
   streampos pos=ist.tellg();
   getline(ist,str);
   size_t revpos=str.find("Revision:");
   if(revpos==str.npos || !ist){
      cout<<"No revision info found in the Dictionary stream"<<endl;
      if(!ist) ist.clear();
      ist.seekg(pos);
   } else {
      pos+=strlen("Revision:");
      ist.seekg(pos);
      ist>>fRevision;
      cout<<"Dictionary revision "<<fRevision<<endl;
   }
   while(ist){
      TDictEntry * ent=new TDictEntry;
      ist>>*ent;
      if(ist) {addEntry(ent);}
      else delete ent;
   }
   // cout<<" Tags:";
   // copy(fTags.begin(),fTags.end(),ostream_iterator<string>(cout,"\n"));
};

TDict::TDict(const TDict &d){
   fRevision=d.fRevision;
   fTags=d.fTags;
   for(auto pent: d.fEntries) fEntries.push_back(pent->clone());
}

void swap(TDict&d1, TDict&d2){
   using std::swap; 
   swap(d1.fRevision,d2.fRevision);
   swap(d1.fTags,d2.fTags);
   swap(d1.fEntries,d2.fEntries);
}

TDict & TDict::operator=(TDict d){
   swap(*this,d);
   return *this;
}

bool TDict::operator==(const TDict&d){
   if(fRevision!=d.fRevision) return false;
   if(fEntries.size()!=d.fEntries.size()) return false;
   if(fTags!=d.fTags) return false;

   vector<TDictEntry *>::const_iterator 
      f=fEntries.begin(), df=d.fEntries.begin();
   for(; f!=fEntries.end(); f++, df++){
      if(!( *(*f)== *(*df)) ) return false;
   }

   return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////
bool TDictEntry::operator==(const TDictEntry&e){
   if(fHistoryValue!=e.fHistoryValue) return false;
   if(fHistory!=e.fHistory) return false;
   if(fTag!=e.fTag) return false;
   if(fLastTryTime!=e.fLastTryTime) return false;
   if(fNumSuccess!=e.fNumSuccess) return false;
   if(fNumTries!=e.fNumTries) return false;
   if(!(*fW.first == *e.fW.first)) return false;
   if(!(*fW.second == *e.fW.second)) return false;
   return true;
}

TDictEntry *TDict::find(const string &s) const{
   UnicodeString us=UnicodeString::fromUTF8(s).toLower(); 
   for(vector<TDictEntry *>::const_iterator it=fEntries.begin(); it!=fEntries.end(); it++){
      UnicodeString ud=UnicodeString::fromUTF8((*it)->words().first->data()).toLower(); 
      if(us==ud) return (*it);
   }
   return NULL;
}

TDictEntry::TDictEntry() {
   fW.first = new TWord;
   fW.second = new TWord;
   fW.first->setLanguage(TWord::eDe);
   fW.second->setLanguage(TWord::eRu);
   fNumTries = fNumSuccess = 0;
   fLastTryTime = 0;
   fHistory = 1;
   fHistoryValue = 0;
}

void TDictEntry::setWords(const TWord & w1, const TWord & w2) {
   if(fW.first && fW.first!=&w1){
      delete fW.first;
      fW.first = w1.clone();
   }
   if(fW.second && fW.second!=&w2){
      delete fW.second;
      fW.second = w2.clone();
   }
}


void TDictEntry::updateHistoryValue(){
   int f=63; // first bit up - fence
   for(;f>=0; f--) if(fHistory[f]) break;
   
   double w=0, cw=1;
   const double fac=1.3;
   for(int i=0; i<f; i++){
      if(fHistory[i]) w+=cw;
      else {
	 if(i==0) w=-2;
	 else w-=cw;
      }
      cw/=fac;
   }
   for(int i=f; i<5; i++){
      w-=cw;
      cw/=fac;
   }
   fHistoryValue=w;
}

static void addBit(bitset<64> &b, bool ok){
   bitset<64> top=b;
   top&=0x8000000000000000;
   b<<=1;
   b|=(ok&0x1);
   b|=top;
}

void TDictEntry::addAttempt(bool ok, const time_t & atime) {
   fNumTries++;
   if(ok) {
      fNumSuccess++;
      fLastTryTime = atime;
   } else {
      if(fNumSuccess==1) fNumSuccess=0;
      else fNumSuccess/=2;
   }
   std::cout<<"Hist "<<fHistory<<' ';
   addBit(fHistory,ok);
   updateHistoryValue();
   std::cout<<fHistory<<' '<<fHistory[0]<<std::endl;
}

void TDictEntry::print(std::ostream &ost) const {
   char ch[1000];
   sprintf(ch,"#Try: %i  #Suc: %i  LastTime: %li History: %li Tag: ^%s^\n",
	   fNumTries,fNumSuccess,fLastTryTime,fHistory.to_ulong(),tags().c_str());
   ost<<ch<<endl<<"Word:: "<<*fW.first<<endl<<"Word:: "<<*fW.second;
};

// static bitset<64> mkHistory(int fNumTries, int fNumSuccess){
//    //   cout<<"mkH "<<fNumTries<<' '<<fNumSuccess<<endl;
//    bitset<64> ret;
//    if(fNumTries>=64) fNumTries=63;
//    if(fNumSuccess>fNumTries) fNumSuccess=fNumTries;

//    ret[fNumTries]=1; //fence
//    bool inv=(fNumTries<fNumSuccess*2);
//    if(inv) fNumSuccess=fNumTries-fNumSuccess;

//    //   cout<<"mid "<<fNumTries<<' '<<fNumSuccess<<' '<<inv<<endl;

//    for(int i=0; i<fNumTries; i++) ret[i]=0;
//    int nSeed=0;
//    while(nSeed<fNumSuccess){
//       int ib=(double(random())/RAND_MAX)*fNumTries;
//       if(ret[ib]==0) {
// 	 ret[ib]=1;
// 	 nSeed++;
//       }
//    }
//    if(inv) {
//       for(int i=0; i<fNumTries; i++){
// 	 if(ret[i]==0) ret[i]=1;
// 	 else ret[i]=0;
//       }
//    }
//    //   cout<<"e mkH "<<fNumTries<<' '<<fNumSuccess<<endl;
//    return ret;
// }

void TDictEntry::read(std::istream &ist)  {
   string str;
   unsigned long hist;
   ist>>str>>fNumTries>>str>>fNumSuccess>>str>>fLastTryTime>>str;
   ist>>hist;
   fHistory=bitset<64>(hist);
   updateHistoryValue();
   getline(ist,str,'^');
   getline(ist,str,'^');
   setTags(str);
   if(!ist) return;
   //   fHistory = mkHistory(fNumTries,fNumSuccess);
   TWord w1, w2;
   
   ist>>str>>w1>>str>>w2;
   if(w1.language()!=TWord::eDe || w2.language()!=TWord::eRu) cout<<"!!!!!!!!!!!!"<<endl;
   if(w1.language()==TWord::eDe) setWords(w1,w2);
   else setWords(w2,w1);
   //   cout<<w1<<endl<<w2<<endl;
};
bool TDictEntry::wildCardTagVecCompare(const vector<string>& vec){
   for(auto ent: vec)
      if(wildCardTagCompare(ent)) return true;
   return false;
}

bool TDictEntry::wildCardTagCompare(const std::string & mask){
   for(auto &tag: fTag)
      if(wildCardCompare(mask,tag)) return true;
   return false;
}
TDictEntry::TDictEntry(const TDictEntry&e){
   fNumTries=e.fNumTries;
   fNumSuccess=e.fNumSuccess;
   fLastTryTime=e.fLastTryTime;
   fTag=e.fTag;
   fW.first=fW.second=NULL;
   fHistory=e.fHistory;
   fHistoryValue=e.fHistoryValue;
   if(e.fW.first) fW.first=e.fW.first->clone();
   if(e.fW.second) fW.second=e.fW.second->clone();
}
std::string TDictEntry::tags() const { 
   string ret;
   for(auto &s: fTag) ret+=s+",";
   if(!ret.empty()) ret=ret.substr(0,ret.size()-1);
   return ret;
}

void TDictEntry::setTags(const std::string & str) { 
   parseTags(str,fTag);
}


void swap(TDictEntry&e1, TDictEntry&e2){
   using std::swap; 
   swap(e1.fW,e2.fW);
   swap(e1.fNumTries,e2.fNumTries);
   swap(e1.fNumSuccess,e2.fNumSuccess);
   swap(e1.fLastTryTime,e2.fLastTryTime);
   swap(e1.fTag,e2.fTag);
   swap(e1.fHistory,e2.fHistory);
   swap(e1.fHistoryValue,e2.fHistoryValue);
}

TDictEntry & TDictEntry::operator=(TDictEntry e){
   swap(*this,e);
   return *this;
}

void TDict::addEntry(const TWord & w1, const TWord & w2, const std::string & tag) {
   TDictEntry * ent=new TDictEntry;
   ent->setTags(tag);
   ent->setWords(w1, w2);
   addEntry(ent);
}

void TDict::addEntry(TDictEntry *e) {
   if(std::find(fEntries.begin(),fEntries.end(),e)==fEntries.end()){
      fEntries.push_back(e);
      for(auto & t:e->tagVector()){
	 list<string>::iterator it=lower_bound(fTags.begin(), fTags.end(),t);
	 if(it==fTags.end() || (*it)!=t) fTags.insert(it,t);
      }
   }
}

pair<int, int> nModifications(const string &str, const string &pat) {
   //1) change
   //2) extra
   //3) missing
   //minimal transformation to get pat
   pair<int,int> ret(-1,-1);
   if(pat.size()<2) return ret;

   for(std::size_t ip=0; ip<pat.size()-2; ip++){
      string pp=pat.substr(ip,2);
      std::size_t pos=0;
      while((pos=str.find(pp,pos))!=str.npos){
	 // int mnext=0;
	 // std::size_t pnext=pos+2;
	 // for(std::size_t i2=ip+2; i2<pat.size(); i2++){
	    
	 // }

	 pos++;
      }
   }
   return ret;
}

list<string> TDict::nearMatch(const string &s) const{
   list<string> lst;
   vector<pair<int, TDictEntry * > > pp;
   UnicodeString us=UnicodeString::fromUTF8(s).toLower(); 
   for(size_t i=0 ; i<fEntries.size(); i++){
      UnicodeString ud=UnicodeString::fromUTF8(fEntries[i]->words().first->data()).toLower(); 
      int pos=ud.indexOf(us);
      if(pos>=0){
	 pp.push_back(pair<int, TDictEntry * >(pos,fEntries[i]));
      }
   }
   // for(size_t i=0 ; i<pp.size(); i++){
   //    if(pp[i].first<10){
   // 	 cout<<pp[i].second->words().first->data()<<' '<<pp[i].first<<endl;
   //    }
   // }
   sort(pp.begin(),pp.end());
   cout<<"------ Sorted ------"<<endl;
   if(pp.size()>10) pp.resize(10);
   for(size_t i=0 ; i<pp.size(); i++){
      lst.push_back(pp[i].second->words().first->data());
      cout<<pp[i].second->words().first->data()<<' '<<pp[i].first<<endl;
   }
   return lst;
}

bool doWildCardCompare(const std::string &mask,  const std::string &str,  
		     const size_t mpos, const size_t spos){
   // cerr<<"Ent mask=\""<<mask<<"\["<<mpos<<"] str="
   //     <<str<<"\["<<spos<<"]"<<endl;
   // cerr<<mask.substr(mpos)<<" | "<<str.substr(spos)<<" |"<<endl;

   size_t minSkip=0, maxSkip=0, m0=mpos, m1=mpos;
   const size_t infSkip=(size_t)-1;

   for(; m0<mask.size(); m0++){
      if(mask[m0]=='*') {
	 maxSkip=infSkip; 
      } else if(mask[m0]=='.') { 
	 minSkip++; 
	 if(maxSkip<infSkip) maxSkip++; 
      } else if(mask[m0]=='?') { 
	 if(maxSkip<infSkip) maxSkip++;  
      } else break;
   }

   m1=m0;
   while(m1<mask.size() && mask[m1]!='*' && mask[m1]!='.' && mask[m1]!='?') m1++;
   // cerr<<"min="<<minSkip<<", max="<<maxSkip<<", m0="<<m0<<", m1="<<m1
   //     <<", needle="<<mask.substr(m0,m1-m0)<<endl;

   if(m0<mask.size()){
      string needle=mask.substr(m0,m1-m0);
      size_t s0=spos;
      while((s0=str.find(needle,s0))!=str.npos){
	 // cerr<<"Found at "<<s0<<" "<<str.substr(s0)<<endl;
	 if(s0-spos>=minSkip && s0-spos<=maxSkip){
	    // cerr<<"MM passed "<<endl;
	    if(wildCardCompare(mask,str,m1,s0+needle.size())){
	       // cerr<<"Rec passed"<<endl;
	       return true;
	    }// else cerr<<"Rec failed"<<endl;
	 }// else cerr<<"MM failed"<<endl;
	 s0+=needle.size();
      }
      return false;
   } else { //check end position
      size_t s0=str.size();
      // cerr<<"Check end "<<s0-spos<<' '<<(s0-spos>=minSkip && s0-spos<=maxSkip)<<endl;
      return (s0-spos>=minSkip && s0-spos<=maxSkip);
   }
}
bool wildCardCompare(const std::string &mask,  const std::string &str,  
		     const size_t mpos, const size_t spos){
   return doWildCardCompare(boost::trim_copy(mask),boost::trim_copy(str),mpos,spos);
}
