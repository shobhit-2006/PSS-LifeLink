// PSS LifeLink: C++ HTTP server for Linux/Docker hosts + MySQL/MariaDB C client.
// Preserves the submitted project's search features: hospital ID, city, organ, blood group.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
string lowerText(string s){ transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return (char)tolower(c);}); return s; }
string decode(const string&s){string o;for(size_t i=0;i<s.size();++i){if(s[i]=='+')o+=' ';else if(s[i]=='%'&&i+2<s.size()){try{o+=(char)stoi(s.substr(i+1,2),nullptr,16);i+=2;}catch(...){o+=s[i];}}else o+=s[i];}return o;}
string jsonEscape(const string&s){string o;for(unsigned char c:s){switch(c){case '"':o+="\\\"";break;case '\\':o+="\\\\";break;case '\n':o+="\\n";break;case '\r':o+="\\r";break;case '\t':o+="\\t";break;default:if(c<32)o+=' ';else o+=(char)c;}}return o;}
string param(const string&q,const string&key){size_t p=0;while(p<q.size()){size_t e=q.find('&',p);if(e==string::npos)e=q.size();string part=q.substr(p,e-p);size_t x=part.find('=');if(x!=string::npos&&part.substr(0,x)==key)return decode(part.substr(x+1));p=e+1;}return "";}
string readFile(const char* path){FILE*f=fopen(path,"rb");if(!f)return "";string s;char b[8192];size_t n;while((n=fread(b,1,sizeof(b),f))>0)s.append(b,n);fclose(f);return s;}
string esc(MYSQL* db,const string&s){vector<char>b(s.size()*2+1);unsigned long n=mysql_real_escape_string(db,b.data(),s.c_str(),(unsigned long)s.size());return string(b.data(),n);}
string getv(MYSQL_ROW row,unsigned int i){return row[i]?row[i]:"";}
struct DB {
 MYSQL* c=nullptr;
 DB(){c=mysql_init(nullptr);if(!c)throw runtime_error("mysql_init failed");const char*h=getenv("MYSQLHOST");if(!h)h=getenv("PSS_DB_HOST");if(!h)h="127.0.0.1";const char*u=getenv("MYSQLUSER");if(!u)u=getenv("PSS_DB_USER");if(!u)u="root";const char*p=getenv("MYSQLPASSWORD");if(!p)p=getenv("PSS_DB_PASSWORD");if(!p)p="";const char*d=getenv("MYSQLDATABASE");if(!d)d="pss_lifelink";const char*ps=getenv("MYSQLPORT");unsigned int port=ps?(unsigned int)atoi(ps):3306;mysql_ssl_set(c,nullptr,nullptr,nullptr,nullptr,nullptr);if(!mysql_real_connect(c,h,u,p,d,port,nullptr,0)){string e=mysql_error(c);mysql_close(c);c=nullptr;throw runtime_error("MySQL connection failed: "+e);} }
 ~DB(){if(c)mysql_close(c);}
 string search(string type,string value,string city){if(value.empty())return "{\"ok\":false,\"message\":\"Please enter a search value.\",\"results\":[]}";string v=esc(c,value),ct=esc(c,city),sql;
 const string fields="h.hospital_ID,h.name AS hospital_name,h.city,h.address,h.authorization_status,h.phone,h.email,";
 if(type=="city") sql="SELECT "+fields+"'Hospital' AS resource_type,'' AS resource_name,'' AS blood_group,0 AS quantity,'' AS status FROM hospital h WHERE LOWER(h.city)=LOWER('"+v+"') OR (LOWER('"+v+"')='bhilai' AND LOWER(h.city)='bhiali') ORDER BY h.hospital_ID";
 else if(type=="hospital"){try{int id=stoi(value);if(id<1||id>1000000)throw runtime_error("invalid");sql="SELECT "+fields+"'Hospital' AS resource_type,'' AS resource_name,'' AS blood_group,0 AS quantity,'' AS status FROM hospital h WHERE h.hospital_ID="+to_string(id);}catch(...){return "{\"ok\":false,\"message\":\"Hospital ID must be numeric.\",\"results\":[]}";}}
 else if(type=="organ")sql="SELECT "+fields+"'Organ' AS resource_type,o.organ_name AS resource_name,COALESCE(o.blood_group,'') AS blood_group,COALESCE(o.quantity,0) AS quantity,COALESCE(o.availability_status,'Unknown') AS status FROM organ o JOIN hospital h ON h.hospital_ID=o.hospital_ID WHERE LOWER(o.organ_name)=LOWER('"+v+"') AND ('"+ct+"'='' OR LOWER(h.city)=LOWER('"+ct+"')) ORDER BY h.city,h.name";
 else if(type=="blood")sql="SELECT "+fields+"'Blood Bank' AS resource_type,bb.name AS resource_name,COALESCE(bb.blood_group,'') AS blood_group,COALESCE(bb.units_available,0) AS quantity,'Listed' AS status FROM blood_bank bb JOIN hospital h ON h.hospital_ID=bb.hospital_ID WHERE UPPER(bb.blood_group)=UPPER('"+v+"') AND ('"+ct+"'='' OR LOWER(h.city)=LOWER('"+ct+"')) ORDER BY h.city,h.name";
 else return "{\"ok\":false,\"message\":\"Unknown search type.\",\"results\":[]}";
 if(mysql_query(c,sql.c_str()))return string("{\"ok\":false,\"message\":\"")+jsonEscape(mysql_error(c))+"\",\"results\":[]}";MYSQL_RES*r=mysql_store_result(c);if(!r)return "{\"ok\":false,\"message\":\"No result set returned.\",\"results\":[]}";MYSQL_FIELD*f=mysql_fetch_fields(r);unsigned int n=mysql_num_fields(r);ostringstream o;o<<"{\"ok\":true,\"results\":[";bool first=true;int count=0;MYSQL_ROW row;while((row=mysql_fetch_row(r))){if(!first)o<<',';first=false;++count;o<<'{';for(unsigned int i=0;i<n;++i){if(i)o<<',';string key=f[i].name;string val=getv(row,i);if(key=="hospital_ID"||key=="quantity"){o<<'\"'<<key<<"\":"<<(val.empty()?"0":val);}else{o<<'\"'<<key<<"\":\""<<jsonEscape(val)<<'\"';}}o<<'}';}mysql_free_result(r);o<<"],\"count\":"<<count<<'}';return o.str();}
};
string http(const string&status,const string&type,const string&body){ostringstream o;o<<"HTTP/1.1 "<<status<<"\r\nContent-Type: "<<type<<"\r\nContent-Length: "<<body.size()<<"\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nX-Content-Type-Options: nosniff\r\n\r\n"<<body;return o.str();}
int main(){try{DB db;int port=8080;if(const char*p=getenv("PORT"))port=atoi(p);int s=socket(AF_INET,SOCK_STREAM,0);if(s<0){perror("socket");return 1;}int yes=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons((unsigned short)port);if(bind(s,(sockaddr*)&a,sizeof(a))<0||listen(s,32)<0){perror("bind/listen");close(s);return 1;}cout<<"PSS LifeLink running on port "<<port<<endl;while(true){int cl=accept(s,nullptr,nullptr);if(cl<0)continue;char b[16384]{};ssize_t n=recv(cl,b,sizeof(b)-1,0);string req=n>0?string(b,(size_t)n):"";size_t p=req.find(' '),e=p==string::npos?string::npos:req.find(' ',p+1);string target=(p!=string::npos&&e!=string::npos)?req.substr(p+1,e-p-1):"/";string body,type="application/json; charset=utf-8",status="200 OK";try{if(target=="/"||target=="/index.html"){body=readFile("PSS-LifeLink-Website.html");if(body.empty()){status="404 Not Found";type="text/plain; charset=utf-8";body="Website file not found.";}else type="text/html; charset=utf-8";}else if(target.rfind("/api/search?",0)==0){string q=target.substr(target.find('?')+1);body=db.search(param(q,"type"),param(q,"q"),param(q,"city"));if(body.find("\"ok\":false")!=string::npos)status="400 Bad Request";}else{status="404 Not Found";body="{\"ok\":false,\"message\":\"Route not found.\",\"results\":[]}";}}catch(const exception&x){status="500 Internal Server Error";body=string("{\"ok\":false,\"message\":\"")+jsonEscape(x.what())+"\",\"results\":[]}";}string packet=http(status,type,body);send(cl,packet.data(),packet.size(),0);shutdown(cl,SHUT_RDWR);close(cl);}close(s);}catch(const exception&e){cerr<<e.what()<<endl;return 1;}return 0;}
