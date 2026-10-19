#include <windows.h>
#include <iostream>
#include <string>
#include <cstdint>
unsigned long long ask(const char* s){std::cout<<s;std::string x;std::getline(std::cin,x);size_t n;auto v=std::stoull(x,&n,0);if(n!=x.size())throw std::runtime_error("Invalid number");return v;}
void check(bool b){if(!b)throw std::runtime_error("Win32 error "+std::to_string(GetLastError()));}
int main(){void* last=nullptr;for(;;){try{
std::cout<<"\n1 System 2 Memory status 3 Query address\n4 Reserve 5 Commit 6 Reserve+commit\n7 Decommit 8 Release 9 Write byte 10 Protect 0 Exit\n";
auto op=ask("> ");if(!op)break;
if(op==1){SYSTEM_INFO s;GetSystemInfo(&s);std::cout<<"Page="<<s.dwPageSize<<" Granularity="<<s.dwAllocationGranularity<<" CPUs="<<s.dwNumberOfProcessors<<" Min="<<s.lpMinimumApplicationAddress<<" Max="<<s.lpMaximumApplicationAddress<<'\n';}
else if(op==2){MEMORYSTATUS s{};s.dwLength=sizeof(s);GlobalMemoryStatus(&s);std::cout<<"Load="<<s.dwMemoryLoad<<" Physical="<<s.dwTotalPhys<<" Available="<<s.dwAvailPhys<<" Virtual="<<s.dwTotalVirtual<<" AvailableVirtual="<<s.dwAvailVirtual<<'\n';}
else if(op>=3&&op<=10){std::cout<<"Last allocation="<<last<<" (0 for automatic allocation)\n";auto addr=reinterpret_cast<void*>(static_cast<uintptr_t>(ask("Address (0x...): ")));
if(op==3){MEMORY_BASIC_INFORMATION m{};check(VirtualQuery(addr,&m,sizeof(m))!=0);std::cout<<"Base="<<m.BaseAddress<<" AllocationBase="<<m.AllocationBase<<" Size="<<m.RegionSize<<" State="<<m.State<<" Protect="<<m.Protect<<" Type="<<m.Type<<'\n';}
else if(op>=4&&op<=6){auto size=ask("Bytes: ");last=VirtualAlloc(addr,static_cast<SIZE_T>(size),op==4?MEM_RESERVE:op==5?MEM_COMMIT:MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);check(last!=nullptr);std::cout<<"Allocated="<<last<<'\n';}
else if(op==7){auto size=ask("Bytes: ");check(VirtualFree(addr,static_cast<SIZE_T>(size),MEM_DECOMMIT));}
else if(op==8)check(VirtualFree(addr,0,MEM_RELEASE));
else if(op==9){auto value=ask("Byte 0..255: ");if(value>255)throw std::runtime_error("Byte out of range");unsigned char b=static_cast<unsigned char>(value);SIZE_T written=0;check(WriteProcessMemory(GetCurrentProcess(),addr,&b,1,&written)&&written==1);std::cout<<"Byte written\n";}
else{auto size=ask("Bytes: ");auto protection=ask("Protection (readonly=2, readwrite=4, noaccess=1): ");DWORD old;check(VirtualProtect(addr,static_cast<SIZE_T>(size),static_cast<DWORD>(protection),&old));MEMORY_BASIC_INFORMATION m{};check(VirtualQuery(addr,&m,sizeof(m))!=0);std::cout<<"Previous="<<old<<" Verified protection="<<m.Protect<<'\n';}}
else throw std::runtime_error("Unknown menu item");
}catch(const std::exception& e){if(!std::cin)break;std::cerr<<e.what()<<'\n';}}}
