#define UNICODE
#define _UNICODE
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

void check(bool ok) { if (!ok) throw std::runtime_error("Win32 error " + std::to_string(GetLastError())); }
struct Handle {
    HANDLE h;
    explicit Handle(HANDLE value): h(value) { check(h != INVALID_HANDLE_VALUE); }
    ~Handle() { CloseHandle(h); }
};
std::wstring input(const wchar_t* prompt) {
    std::wcout << prompt; std::wstring s; std::getline(std::wcin,s); return s;
}
DWORD number(const wchar_t* prompt) {
    auto s=input(prompt); size_t n=0; auto v=std::stoull(s,&n,0);
    if(n!=s.size() || v>MAXDWORD) throw std::runtime_error("Invalid DWORD");
    return static_cast<DWORD>(v);
}
void showtime(const FILETIME& ft) {
    SYSTEMTIME st{}; check(FileTimeToSystemTime(&ft,&st));
    std::wcout<<st.wYear<<L'-'<<st.wMonth<<L'-'<<st.wDay<<L' '<<st.wHour<<L':'<<st.wMinute<<L':'<<st.wSecond<<L" UTC\n";
}
int main() {
    for (;;) {
        std::wcout << L"\n1 Disk mask  2 Disk list  3 Disk information\n4 Create directory  5 Remove directory  6 Create file\n7 CopyFile  8 MoveFile  9 MoveFileEx\n10 Get attributes  11 Set attributes  12 Handle information\n13 Get times  14 Set last-write time  0 Exit\n";
        try {
            auto choice=input(L"> "); if(!std::wcin || choice==L"0") break;
            int op=std::stoi(choice);
            if(op==1) { DWORD mask=GetLogicalDrives(); check(mask!=0); std::wcout<<L"Mask: "<<mask<<L"\n"; for(int i=0;i<26;++i) if(mask&(1u<<i)) std::wcout<<wchar_t(L'A'+i)<<L":\\\n"; }
            else if(op==2) { DWORD n=GetLogicalDriveStringsW(0,nullptr); check(n!=0); std::vector<wchar_t>b(n+1); check(GetLogicalDriveStringsW(static_cast<DWORD>(b.size()),b.data())!=0); for(auto p=b.data();*p;p+=wcslen(p)+1) std::wcout<<p<<L'\n'; }
            else if(op==3) {
                auto root=input(L"Root (C:\\): "); wchar_t label[MAX_PATH+1]{},fs[MAX_PATH+1]{}; DWORD serial,maxName,flags,spc,bps,free,total;
                std::wcout<<L"Drive type: "<<GetDriveTypeW(root.c_str())<<L'\n';
                check(GetVolumeInformationW(root.c_str(),label,MAX_PATH+1,&serial,&maxName,&flags,fs,MAX_PATH+1));
                check(GetDiskFreeSpaceW(root.c_str(),&spc,&bps,&free,&total));
                std::wcout<<L"Label: "<<label<<L" FS: "<<fs<<L" Serial: "<<serial<<L" Max name: "<<maxName<<L" Flags: "<<flags<<L"\nCluster bytes: "<<static_cast<unsigned long long>(spc)*bps<<L" Free bytes: "<<static_cast<unsigned long long>(free)*spc*bps<<L" Total bytes: "<<static_cast<unsigned long long>(total)*spc*bps<<L'\n';
            } else if(op>=4 && op<=14) {
                auto path=input(L"Path: ");
                if(op==4) check(CreateDirectoryW(path.c_str(),nullptr));
                if(op==5) check(RemoveDirectoryW(path.c_str()));
                if(op==6) { Handle h(CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr)); }
                if(op>=7 && op<=9) {
                    auto dst=input(L"Destination: ");
                    if(op==7) check(CopyFileW(path.c_str(),dst.c_str(),TRUE));
                    if(op==8) check(MoveFileW(path.c_str(),dst.c_str()));
                    if(op==9) { auto replace=input(L"Replace existing destination? (yes/no): "); check(MoveFileExW(path.c_str(),dst.c_str(),replace==L"yes"?MOVEFILE_REPLACE_EXISTING:0)); }
                }
                if(op==10) { DWORD a=GetFileAttributesW(path.c_str()); check(a!=INVALID_FILE_ATTRIBUTES); std::wcout<<L"Attribute mask: "<<a<<L" readonly="<<bool(a&FILE_ATTRIBUTE_READONLY)<<L" hidden="<<bool(a&FILE_ATTRIBUTE_HIDDEN)<<L" system="<<bool(a&FILE_ATTRIBUTE_SYSTEM)<<L" archive="<<bool(a&FILE_ATTRIBUTE_ARCHIVE)<<L" directory="<<bool(a&FILE_ATTRIBUTE_DIRECTORY)<<L'\n'; }
                if(op==11) { DWORD a=number(L"New attribute mask (decimal or 0x...; normal=128): "); check(SetFileAttributesW(path.c_str(),a)); }
                if(op>=12) {
                    Handle h(CreateFileW(path.c_str(),op==14?FILE_WRITE_ATTRIBUTES:FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr));
                    if(op==12) { BY_HANDLE_FILE_INFORMATION i{}; check(GetFileInformationByHandle(h.h,&i)); std::wcout<<L"Size: "<<((static_cast<unsigned long long>(i.nFileSizeHigh)<<32)|i.nFileSizeLow)<<L" Links: "<<i.nNumberOfLinks<<L" Volume serial: "<<i.dwVolumeSerialNumber<<L" File index: "<<((static_cast<unsigned long long>(i.nFileIndexHigh)<<32)|i.nFileIndexLow)<<L'\n'; }
                    if(op==13) { FILETIME c,a,w; check(GetFileTime(h.h,&c,&a,&w)); std::wcout<<L"Created: ";showtime(c); std::wcout<<L"Accessed: ";showtime(a); std::wcout<<L"Written: ";showtime(w); }
                    if(op==14) { SYSTEMTIME st{}; st.wYear=static_cast<WORD>(number(L"UTC year: ")); st.wMonth=static_cast<WORD>(number(L"Month: ")); st.wDay=static_cast<WORD>(number(L"Day: ")); st.wHour=static_cast<WORD>(number(L"Hour: ")); st.wMinute=static_cast<WORD>(number(L"Minute: ")); st.wSecond=static_cast<WORD>(number(L"Second: ")); FILETIME ft; check(SystemTimeToFileTime(&st,&ft)); check(SetFileTime(h.h,nullptr,nullptr,&ft)); }
                }
            } else throw std::runtime_error("Unknown menu item");
            std::wcout<<L"OK\n";
        } catch(const std::exception& e) { std::cerr<<"Error: "<<e.what()<<"\n"; }
    }
}
