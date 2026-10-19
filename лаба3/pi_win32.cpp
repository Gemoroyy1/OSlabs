#include <windows.h>
#include <atomic>
#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
constexpr long long N=10000000, BLOCK=2306160;
struct Slot{HANDLE thread=nullptr,done=nullptr;long long first=0,last=0;double sum=0;std::atomic<unsigned> generation{0};std::atomic<bool> stop{false};};
DWORD WINAPI run(void* arg){auto&s=*static_cast<Slot*>(arg);unsigned seen=0;for(;;){while(s.generation.load(std::memory_order_acquire)==seen&&!s.stop.load())SwitchToThread();if(s.stop.load())return 0;seen=s.generation.load();double sum=0;for(auto i=s.first;i<s.last;++i){double x=(i+.5)/N;sum+=4/(1+x*x);}s.sum=sum;SetEvent(s.done);}}
void check(bool ok){if(!ok)throw std::runtime_error("Win32 error "+std::to_string(GetLastError()));}
int main(int argc,char**argv){std::vector<std::unique_ptr<Slot>> slots;try{
int count=argc>1?std::stoi(argv[1]):1;if(count<1||count>64)throw std::runtime_error("threads 1..64");
LARGE_INTEGER freq,start,end;check(QueryPerformanceFrequency(&freq));check(QueryPerformanceCounter(&start));
for(int i=0;i<count;++i){auto s=std::make_unique<Slot>();s->done=CreateEventW(nullptr,FALSE,FALSE,nullptr);check(s->done!=nullptr);s->thread=CreateThread(nullptr,0,run,s.get(),CREATE_SUSPENDED,nullptr);if(!s->thread){CloseHandle(s->done);check(false);}slots.push_back(std::move(s));}
long long next=0;int active=0;double sum=0;std::vector<HANDLE> events(count);std::vector<bool> running(count,false);
auto assign=[&](int i){auto&s=*slots[i];s.first=next;s.last=std::min(N,next+BLOCK);next=s.last;s.generation.fetch_add(1,std::memory_order_release);check(ResumeThread(s.thread)!=DWORD(-1));running[i]=true;++active;};
for(int i=0;i<count;++i){events[i]=slots[i]->done;if(next<N)assign(i);}
while(active){DWORD result=WaitForMultipleObjects(count,events.data(),FALSE,INFINITE);check(result>=WAIT_OBJECT_0&&result<WAIT_OBJECT_0+static_cast<DWORD>(count));int i=static_cast<int>(result-WAIT_OBJECT_0);auto&s=*slots[i];check(running[i]);
// Event is signalled after calculation. The worker only polls atomics now;
// it holds no mutex or allocator lock when SuspendThread is called.
check(SuspendThread(s.thread)!=DWORD(-1));sum+=s.sum;running[i]=false;--active;if(next<N)assign(i);
}
for(auto&s:slots){s->stop.store(true);check(ResumeThread(s->thread)!=DWORD(-1));check(WaitForSingleObject(s->thread,INFINITE)==WAIT_OBJECT_0);CloseHandle(s->thread);CloseHandle(s->done);s->thread=s->done=nullptr;}
check(QueryPerformanceCounter(&end));double sec=double(end.QuadPart-start.QuadPart)/freq.QuadPart;
std::cout<<std::setprecision(17)<<"threads,actual,N,block,pi,seconds\n"<<count<<','<<count<<','<<N<<','<<BLOCK<<','<<sum/N<<','<<sec<<'\n';return std::abs(sum/N-std::acos(-1.0))<1e-11?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';
// Failure path: wake suspended workers and request cooperative termination.
for(auto&s:slots){s->stop.store(true);if(s->thread){ResumeThread(s->thread);WaitForSingleObject(s->thread,INFINITE);CloseHandle(s->thread);}if(s->done)CloseHandle(s->done);}return 1;}}
