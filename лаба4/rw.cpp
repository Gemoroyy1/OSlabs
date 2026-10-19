#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <stdexcept>
void ck(bool b){if(!b)throw std::runtime_error("Win32 error "+std::to_string(GetLastError()));}
int main(int argc,char**argv){try{if(argc<4)throw std::runtime_error("rw reader|writer ID RUN [cycles]");bool writer=std::string(argv[1])=="writer";int id=std::stoi(argv[2]),cycles=argc>4?std::stoi(argv[4]):3;if(cycles<1||cycles>100)throw std::runtime_error("cycles 1..100");std::string prefix="Local\\OSlab4_"+std::string(argv[3]);SYSTEM_INFO info;GetSystemInfo(&info);SIZE_T bytes=16*info.dwPageSize;
HANDLE file=CreateFileA((std::string(argv[3])+".bin").c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);ck(file!=INVALID_HANDLE_VALUE);
HANDLE mapping=CreateFileMappingA(file,nullptr,PAGE_READWRITE,0,static_cast<DWORD>(bytes),(prefix+"_data").c_str());ck(mapping!=nullptr);auto data=static_cast<char*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,bytes));ck(data!=nullptr);ck(VirtualLock(data,bytes));
HANDLE meta=CreateFileMappingA(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,16*sizeof(LONG),(prefix+"_counts").c_str());ck(meta!=nullptr);auto counts=static_cast<LONG*>(MapViewOfFile(meta,FILE_MAP_ALL_ACCESS,0,0,0));ck(counts!=nullptr);
HANDLE mutexes[16],sems[16];for(int p=0;p<16;++p){mutexes[p]=CreateMutexA(nullptr,FALSE,(prefix+"_mutex"+std::to_string(p)).c_str());sems[p]=CreateSemaphoreA(nullptr,1,1,(prefix+"_sem"+std::to_string(p)).c_str());ck(mutexes[p]&&sems[p]);}
std::ofstream log(std::string(argv[3])+"_"+(writer?"writer":"reader")+std::to_string(id)+".csv");ck(bool(log));log<<"time,qpc_us,pid,role,id,state,page,value\n";auto emit=[&](const char*state,int page,LONG value=0){LARGE_INTEGER ticks,freq;QueryPerformanceCounter(&ticks);QueryPerformanceFrequency(&freq);log<<timeGetTime()<<','<<(ticks.QuadPart*1000000/freq.QuadPart)<<','<<GetCurrentProcessId()<<','<<(writer?"writer":"reader")<<','<<id<<','<<state<<','<<page<<','<<value<<'\n';log.flush();};emit("locked",-1);
std::mt19937 rng(id+(writer?1000:2000));std::uniform_int_distribution<int> page(0,15),delay(500,1500);
for(int c=0;c<cycles;++c){int p=page(rng);emit("waiting",p);if(writer)ck(WaitForSingleObject(sems[p],INFINITE)==WAIT_OBJECT_0);else{ck(WaitForSingleObject(mutexes[p],INFINITE)==WAIT_OBJECT_0);if(counts[p]==0)ck(WaitForSingleObject(sems[p],INFINITE)==WAIT_OBJECT_0);++counts[p];ck(ReleaseMutex(mutexes[p]));}
auto value=reinterpret_cast<LONG*>(data+p*info.dwPageSize);if(writer)++*value;emit(writer?"writing":"reading",p,*value);Sleep(delay(rng));emit("releasing",p,*value);
if(writer)ck(ReleaseSemaphore(sems[p],1,nullptr));else{ck(WaitForSingleObject(mutexes[p],INFINITE)==WAIT_OBJECT_0);if(--counts[p]==0)ck(ReleaseSemaphore(sems[p],1,nullptr));ck(ReleaseMutex(mutexes[p]));}}
emit("finished",-1);VirtualUnlock(data,bytes);UnmapViewOfFile(data);UnmapViewOfFile(counts);for(int p=0;p<16;++p){CloseHandle(mutexes[p]);CloseHandle(sems[p]);}CloseHandle(meta);CloseHandle(mapping);CloseHandle(file);return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
