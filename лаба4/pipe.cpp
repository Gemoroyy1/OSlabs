#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
void ck(bool b){if(!b)throw std::runtime_error("Win32 error "+std::to_string(GetLastError()));}
struct Read{OVERLAPPED ov{};char buffer[4096]{};bool done=false;DWORD error=0,bytes=0;};
void CALLBACK complete(DWORD error,DWORD bytes,OVERLAPPED*ov){auto*r=reinterpret_cast<Read*>(ov);r->error=error;r->bytes=bytes;r->done=true;}
int main(int argc,char**argv){
#ifdef SERVER
bool server=true;
#else
bool server=false;
#endif
std::string name=argc>1?argv[1]:"\\\\.\\pipe\\OSlab4";HANDLE pipe=INVALID_HANDLE_VALUE,event=nullptr;bool connected=false;try{for(;;){std::cout<<(server?"\n1 Create pipe 2 Create event 3 Connect 4 Write 5 Disconnect 0 Exit\n> ":"\n1 Connect 2 Read 0 Exit\n> ")<<std::flush;std::string choice;if(!std::getline(std::cin,choice)||choice=="0")break;
if(server){if(choice=="1"){if(pipe!=INVALID_HANDLE_VALUE)throw std::runtime_error("Already created");pipe=CreateNamedPipeA(name.c_str(),PIPE_ACCESS_OUTBOUND|FILE_FLAG_OVERLAPPED,PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,1,4096,4096,0,nullptr);ck(pipe!=INVALID_HANDLE_VALUE);}
else if(choice=="2"){if(event)throw std::runtime_error("Event exists");event=CreateEventW(nullptr,TRUE,FALSE,nullptr);ck(event!=nullptr);}
else if(choice=="3"){if(pipe==INVALID_HANDLE_VALUE||!event||connected)throw std::runtime_error("Create pipe/event first");OVERLAPPED ov{};ov.hEvent=event;ResetEvent(event);if(!ConnectNamedPipe(pipe,&ov)){auto e=GetLastError();if(e==ERROR_IO_PENDING){ck(WaitForSingleObject(event,INFINITE)==WAIT_OBJECT_0);DWORD n;ck(GetOverlappedResult(pipe,&ov,&n,FALSE));}else ck(e==ERROR_PIPE_CONNECTED);}connected=true;std::cout<<"Connected\n";}
else if(choice=="4"){if(!connected||!event)throw std::runtime_error("Connect first");std::cout<<"Data: "<<std::flush;std::string text;std::getline(std::cin,text);if(text.size()>4095)throw std::runtime_error("Message too long");OVERLAPPED ov{};ov.hEvent=event;ResetEvent(event);DWORD n=0;BOOL ok=WriteFile(pipe,text.c_str(),static_cast<DWORD>(text.size()+1),&n,&ov);if(!ok)ck(GetLastError()==ERROR_IO_PENDING);ck(WaitForSingleObject(event,INFINITE)==WAIT_OBJECT_0);ck(GetOverlappedResult(pipe,&ov,&n,FALSE));std::cout<<"Written bytes: "<<n<<'\n';}
else if(choice=="5"){ck(connected);ck(DisconnectNamedPipe(pipe));connected=false;}}
else{if(choice=="1"){if(pipe!=INVALID_HANDLE_VALUE)throw std::runtime_error("Already connected");pipe=CreateFileA(name.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);ck(pipe!=INVALID_HANDLE_VALUE);std::cout<<"Connected\n";}
else if(choice=="2"){ck(pipe!=INVALID_HANDLE_VALUE);Read r;ck(ReadFileEx(pipe,r.buffer,sizeof(r.buffer),&r.ov,complete));while(!r.done)SleepEx(INFINITE,TRUE);if(r.error){SetLastError(r.error);ck(false);}std::cout<<"Received: "<<std::string(r.buffer,r.bytes&&r.buffer[r.bytes-1]==0?r.bytes-1:r.bytes)<<'\n';}}}}
catch(const std::exception&e){std::cerr<<e.what()<<'\n';if(event)CloseHandle(event);if(pipe!=INVALID_HANDLE_VALUE)CloseHandle(pipe);return 1;}if(event)CloseHandle(event);if(pipe!=INVALID_HANDLE_VALUE)CloseHandle(pipe);return 0;}
