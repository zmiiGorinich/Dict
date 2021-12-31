#include<iostream>
#include<fstream>
#include <sys/types.h>
#include <unistd.h>
#include<string>
#include<string.h>
#include <signal.h>
using namespace std;

string getCommand(int pid){
   char ch[1000];
   snprintf(ch,sizeof(ch),"/proc/%i/comm",pid);
   
   ifstream ifs(ch);
   string str;
   getline(ifs,str);
   if(ifs) return str;
   return "";
}

bool check_Proc(){
   pid_t pid=getpid();
   string myCommand=getCommand(pid);
   cout<<"My pid =="<<pid<<' '<<myCommand<<endl;
   
   {
      ifstream ifsPID("/tmp/DeRuDict.pid");
      int runPid=-1;
      ifsPID>>runPid;
      if(ifsPID && runPid>0){
	 string runCommand=getCommand(runPid);
	 cout<<"runPid= "<<runPid<<' '<<runCommand<<endl;
	 if(!runCommand.empty() && runCommand==myCommand){
	    do{
	       cout<<"Kill "<<runPid<<endl;
	       kill(runPid,SIGUSR1);
	       usleep(500000);
	    } while(getCommand(runPid)==myCommand);
	    cout<<"Killed "<<endl;
	 }
      }
   }

   ofstream ofsPID("/tmp/DeRuDict.pid");
   ofsPID<<pid<<endl;
   return true;
}
