#include"Dictor.h"
#include"GDrive.h"
#include<sstream>
#include<stdio.h>
#include<string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include<fstream>
#include<iostream>
#include<QErrorMessage>

using namespace std;

Dictor * Dictor::gInstance=NULL;

Dictor::Dictor():fDrive(NULL){
}

Dictor * Dictor::get(){
   if(!gInstance) gInstance=new Dictor;
   return gInstance;
}

extern const string   HTMLhead;
extern const string HTMLTableHead;

static void chmodRSA(){
   struct stat st;
   stat("rsa", &st);
   if( (st.st_mode & 07777) != S_IRWXU)  chmod("rsa", S_IRWXU);
   stat("rsa/id_rsa",&st);
   if( (st.st_mode & 07777) != (S_IRUSR|S_IWUSR))
      chmod("rsa/id_rsa", S_IRUSR|S_IWUSR);  
}

bool Dictor::readGDrive(){
   string status="<font color=blue>Reading RuDeDict.dat from Google Drive ... </font>";
   emit updateStatusLine(status.c_str());
   if(!fDrive) fDrive=new GDrive;
   fDrive->read("RuDeDict.dat"); 
   const QVariant &v=fDrive->operationResult();
   if((int)v.type()!=QMetaType::QByteArray){
      cout<<"****** BAD DRIVE RETURN "<<v.type()<<endl;
      status+=" <font color=red><b>failed!</b></font>";
      emit updateStatusLine(status.c_str());
      return false;
   }

   stringstream dictSS(v.toByteArray().data());
   fDict.read(dictSS);
   fDictRead=fDict;
   status+=" <font color=green><b>OK</b></font>";
   emit updateStatusLine(status.c_str());
   return true;
}

bool Dictor::saveToGDrive(){
   string status;

   if(fDict==fDictRead) {
      emit updateStatusLine("Dictionary UNCHANGED, no saving needed");
      cout<<"Dictionary UNCHANGED, no saving! "<<endl;
      return true;
   } else {
      cout<<"Dictionary CHANGED, saving new revision!"<<endl;
      status="<font color=blue>Saving to Google Drive ... </font>";
      emit updateStatusLine(status.c_str());
   }
   
   if(!fDrive) fDrive=new GDrive;
   stringstream dictSS1;
   fDict.setRevision(fDict.revision()+1);
   fDict.print(dictSS1);
   
   fDrive->write("RuDeDict.dat",dictSS1.str().c_str());
   status+=" <font color=green><b>OK</b></font>";
   emit updateStatusLine(status.c_str());
   
   return true;
}

bool Dictor::sshRead(TDict * dict){
   if(!dict) dict=&fDict;
   FILE *DictStream=popen("ssh -i rsa/id_rsa dymov@ikp450.ikp.kfa-juelich.de cat MyDict/dict.dat", "r");
   stringstream dictSS;
 
   while(1){
      const size_t bufSize=10000;
      char data[bufSize+1];
      size_t nRead=fread(data,1,bufSize,DictStream);
      if(nRead<=bufSize) data[nRead]='\0'; 

      //      cout<<nRead<<' '<<feof(DictStream)<<' '<<ferror(DictStream)<<endl;
      if(ferror(DictStream)) break;
      dictSS.write((const char * )data,nRead);
      if(ferror(DictStream) || feof(DictStream)) break;
   }
   
   pclose(DictStream);
   //   ifstream ifs((string("dict-")+getenv("USER")+".dat").c_str());
   dict->read(dictSS);
   if(dict==&fDict) fDictRead=fDict;

   return true;
}
bool Dictor::sshSave(){
   if(fDict==fDictRead) {
      cout<<"Dictionary UNCHANGED, no saving! "<<endl;
      return true;
   } else cout<<"Dictionary CHANGED, saving new revision!"<<endl;

   chmodRSA();
   // ofstream ofs((string("dict-")+getenv("USER")+".dat").c_str());
   // fDict.print(ofs);
   
   TDict checkRevDict;
   sshRead(&checkRevDict);

   if(checkRevDict.revision()!=fDict.revision()){
      cout<<"Another program have updated Dict while this one was running!"<<endl;
      char ch[10000];
      snprintf(ch,sizeof(ch),"dict_rev%i.dat",checkRevDict.revision());
      cout<<"The other Dict is saved as "<<ch<<endl;
      string comm="ssh -i rsa/id_rsa dymov@ikp450.ikp.kfa-juelich.de cp -p MyDict/dict.dat MyDict/";
      comm+=ch;
      system(comm.c_str());
      QErrorMessage * qm=new QErrorMessage(0);
      string mess="Another program have updated Dict while this one was running!\n";
      mess+="The other Dict is saved as ";
      mess+=ch;
      qm->showMessage(mess.c_str());
      //      connect(qm, SIGNAL(finished(int)), this, SLOT(setReadyToClose()));
      //      fReadyToClose=false;
   }

   stringstream dictSS;
   fDict.setRevision(fDict.revision()+1);
   fDict.print(dictSS);
 
   FILE *DictStream=popen("ssh -i rsa/id_rsa dymov@ikp450.ikp.kfa-juelich.de \"cat > MyDict/dict.dat\"", "w");

   while(1){
      const size_t bufSize=10000;
      char data[bufSize+1];
      dictSS.read(data,bufSize);
      size_t nRead=dictSS.gcount();
      if(nRead<=bufSize) data[nRead]='\0'; 

      if(dictSS.bad()) break;
      //      size_t nWrite=
      fwrite((const char * )data,1,nRead,DictStream);
      if(ferror(DictStream) || dictSS.bad() || dictSS.eof()) break;
   }
   
   pclose(DictStream);
   return true;
}


void Dictor::showWebPageForLabel(string tag){
   //   QWebEngineView* view=new QWebEngineView;
   QString HTML;

   HTML.append(QString::fromUtf8(HTMLhead.c_str()));


   HTML.append("<table style=\"page-break-before: auto; page-break-after: auto;\">\n<caption>");
   HTML.append("</caption>\n");
   
   TDictEntry * fCurrent=NULL;
   bool fwd, repeat;
   int ntot=0;
   while( (fCurrent = fDict.getNext(tag, fCurrent, fwd=true, repeat=false))) ntot++;
   char ch[1000];
   snprintf(ch, sizeof(ch), " [%i]",ntot);


   HTML.append(QString::fromUtf8(HTMLTableHead.c_str()));
   HTML.append("<tr>");
   HTML.append("<th colspan=\"4\" style=\"text-align:center\">");
   HTML.append("<capsty>");
   HTML.append(QString::fromUtf8((tag+ch).c_str()));  
   HTML.append("</capsty>");
   HTML.append("</th>");
   HTML.append("</tr>");

   fCurrent=NULL;
   while( (fCurrent = fDict.getNext(tag, fCurrent, fwd=true, repeat=false))){
      HTML.append("<tr style=\"page-break-inside: avoid;\">\n");
      vector<string> dret=fCurrent->words().first->showTable();
      for(size_t i=0; i<3 && i<dret.size(); i++){
	 HTML.append("<td>");
	 if(i==1) {
	    HTML.append("<ppp>");
	 } 
	 if(i==0) {
	    HTML.append("<pupup>");
	 } 
	 if(i==2) {
	    HTML.append("<p3>");
	 } 
	 HTML.append(QString::fromUtf8(dret[i].c_str()));
	 if(i==1){
	    HTML.append("</ppp>");
	 }
	 if(i==0) {
	    HTML.append("</pupup>");
	 } 
	 if(i==2) {
	    HTML.append("</p3>");
	 } 
	 HTML.append("</td>");
      }
      for(size_t i=dret.size(); i<3 ; i++) HTML.append("<td></td>");

      HTML.append("<td>");
      HTML.append(QString::fromUtf8(fCurrent->words().second->show().c_str()));
      //      HTML.append(fCurrent->words().second->show().c_str());
      HTML.append("</td>");
   }
   HTML.append("</table>\n");

   HTML.append("</body>\n</html>\n");

   ofstream ofs("page.html");
   ofs<<HTML.toUtf8().constData()<<flush;
   system("chromium page.html &");

   // view->setHtml(HTML);
   // view->show();
   // view->page()->printToPdf("label.pdf");
}


const string  HTMLhead=R"V0G0N(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
table {
    font-family: arial, sans-serif;
    border-collapse: collapse;
    width: 100%;
font-size:50%;	
}
capsty { 
    text-align: center;
    font-family: arial, sans-serif;
font-size:160%;	
}
td, th {
    border: 1px solid #dddddd;
    text-align: left;
    padding: 4px;
}

tr:nth-child(even) {
    background-color: #dddddd;
}
ppp {
    font-size:120%;
color:blue;
}
pupup {
    font-size:90%;
font-style: italic;
}
p3 {
    font-size:90%;
}
</style>
</head>
<body>
)V0G0N";


const string HTMLTableHead=   R"V0G0N(
   <colgroup>
   <col style=\"foreground-color:black\">
   <col style=\"foreground-color:red\">
   <col span=\"2\" style=\"foreground-color:black\">
   </colgroup>

)V0G0N";
   // <tr>
   // <th></th>
   // <th>Word</th>
   // <th>Forms</th>
   // <th>Translation</th>
   // </tr>
