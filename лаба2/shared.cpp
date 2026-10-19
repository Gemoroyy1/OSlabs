#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cerrno>
constexpr size_t SIZE=4096;
void check(bool b){if(!b)throw std::runtime_error(strerror(errno));}
void wait_sem(sem_t* s){while(sem_wait(s)<0){if(errno!=EINTR)check(false);}}
int main(int argc,char**argv){
#ifdef SERVER
const bool server=true;
#else
const bool server=false;
#endif
std::string file=argc>1?argv[1]:"/tmp/oslab2-shared.bin";
std::string name=argc>2?argv[2]:"/oslab2";
sem_t *ready=SEM_FAILED,*read=SEM_FAILED;int fd=-1;void* ptr=MAP_FAILED;bool created=false;bool own_ready=false,own_read=false;
try{for(;;){std::cout<<"\n1 Map file 2 "<<(server?"Write data":"Read data")<<" 0 Exit\n> "<<std::flush;std::string choice;if(!std::getline(std::cin,choice)||choice=="0")break;
if(choice=="1"){
if(ptr!=MAP_FAILED)throw std::runtime_error("Already mapped");
fd=open(file.c_str(),server?O_RDWR|O_CREAT|O_EXCL:O_RDONLY,0600);check(fd>=0);created=server;
if(server)check(ftruncate(fd,SIZE)==0);struct stat st{};check(fstat(fd,&st)==0);if(st.st_size!=SIZE)throw std::runtime_error("Unexpected file size");
ready=sem_open((name+"-ready").c_str(),server?O_CREAT|O_EXCL:0,0600,0);check(ready!=SEM_FAILED);
own_ready=server;
read=sem_open((name+"-read").c_str(),server?O_CREAT|O_EXCL:0,0600,0);check(read!=SEM_FAILED);
own_read=server;
ptr=mmap(nullptr,SIZE,server?PROT_READ|PROT_WRITE:PROT_READ,MAP_SHARED,fd,0);check(ptr!=MAP_FAILED);std::cout<<"Mapped\n";
}else if(choice=="2"){
if(ptr==MAP_FAILED)throw std::runtime_error("Map first");
if(server){std::cout<<"Data: "<<std::flush;std::string text;std::getline(std::cin,text);if(text.size()>=SIZE)throw std::runtime_error("Message too long");memcpy(ptr,text.c_str(),text.size()+1);check(sem_post(ready)==0);std::cout<<"Waiting for client\n"<<std::flush;wait_sem(read);std::cout<<"Client acknowledged\n";}
else{std::cout<<"Waiting for data\n"<<std::flush;wait_sem(ready);size_t n=strnlen(static_cast<char*>(ptr),SIZE);if(n==SIZE)throw std::runtime_error("Invalid message");std::cout<<"Client received: "<<std::string(static_cast<char*>(ptr),n)<<'\n';check(sem_post(read)==0);}
}else std::cout<<"Invalid menu item\n";
}}catch(const std::exception&e){std::cerr<<"Error: "<<e.what()<<'\n';}
if(ptr!=MAP_FAILED)munmap(ptr,SIZE);if(fd>=0)close(fd);
if(ready!=SEM_FAILED)sem_close(ready);if(read!=SEM_FAILED)sem_close(read);
if(created)unlink(file.c_str());
if(own_ready)sem_unlink((name+"-ready").c_str());
if(own_read)sem_unlink((name+"-read").c_str());
}
