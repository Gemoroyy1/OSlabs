#include <aio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <iomanip>

struct aio_operation {
    aiocb aio{};
    char* buffer=nullptr;
    int write_operation=0;
    void* next_operation=nullptr;
    std::vector<char> storage;
    size_t transferred=0, length=0;
    off_t offset=0;
    bool active=false;
};
void require(bool ok,const char* what) { if(!ok) throw std::runtime_error(std::string(what)+": "+strerror(errno)); }
// Called by the dispatch loop after aio_error reports completion. SIGEV_NONE
// avoids races between callback threads and the scheduling thread.
void aio_completion_handler(sigval_t value) {
    auto& op=*static_cast<aio_operation*>(value.sival_ptr);
    int error=aio_error(&op.aio);
    ssize_t result=aio_return(&op.aio); // exactly once per submitted request
    op.active=false;
    if(error) { errno=error; require(false,"asynchronous I/O"); }
    require(result>0,"unexpected zero/failed transfer");
    op.transferred+=static_cast<size_t>(result);
}
struct Fd { int value=-1; ~Fd(){if(value>=0)close(value);} };
size_t positive(const char* text) {
    std::string s(text); size_t end=0; auto n=std::stoull(s,&end);
    if(s.empty()||s[0]=='-'||end!=s.size()||!n||n>std::numeric_limits<size_t>::max()) throw std::runtime_error("Invalid positive integer");
    return static_cast<size_t>(n);
}
int main(int argc,char** argv) {
    std::vector<std::unique_ptr<aio_operation>> ops;
    Fd src,dst;
    try {
        if(argc!=5) { std::cerr<<"Usage: aio_copy SOURCE DEST BLOCK_BYTES OPERATIONS\n";return 2; }
        size_t block=positive(argv[3]), count=positive(argv[4]);
        if(count>1024) throw std::runtime_error("At most 1024 operations");
        src.value=open(argv[1],O_RDONLY|O_NONBLOCK); require(src.value>=0,"open source");
        struct stat info{}; require(fstat(src.value,&info)==0,"fstat");
        if(!S_ISREG(info.st_mode)) throw std::runtime_error("Source must be a regular file");
        size_t cluster=static_cast<size_t>(info.st_blksize);
        if(block%cluster) throw std::runtime_error("Block must be a multiple of st_blksize="+std::to_string(cluster));
        // O_EXCL protects existing data and source aliases from truncation.
        dst.value=open(argv[2],O_CREAT|O_EXCL|O_WRONLY|O_NONBLOCK,0666); require(dst.value>=0,"create destination");
        for(size_t i=0;i<count;++i) { auto op=std::make_unique<aio_operation>(); op->storage.resize(block);op->buffer=op->storage.data();ops.push_back(std::move(op)); }
        off_t next=0; size_t completed=0;
        auto begin=std::chrono::steady_clock::now();
        while(completed<static_cast<size_t>(info.st_size)) {
            std::vector<const aiocb*> wait;
            for(auto& p:ops) {
                auto& op=*p;
                if(op.active && aio_error(&op.aio)!=EINPROGRESS) {
                    sigval_t value{};value.sival_ptr=&op;aio_completion_handler(value);
                    if(op.transferred==op.length) {
                        if(op.write_operation) { completed+=op.length;op.length=0; }
                        else { op.write_operation=1;op.transferred=0; }
                    }
                }
                if(!op.active) {
                    if(!op.length && next<info.st_size) {
                        op.offset=next;op.length=std::min(block,static_cast<size_t>(info.st_size-next));next+=static_cast<off_t>(op.length);op.transferred=0;op.write_operation=0;
                    }
                    if(op.length) {
                        op.aio={};op.aio.aio_fildes=op.write_operation?dst.value:src.value;
                        op.aio.aio_buf=op.buffer+op.transferred;op.aio.aio_nbytes=op.length-op.transferred;
                        op.aio.aio_offset=op.offset+static_cast<off_t>(op.transferred);op.aio.aio_sigevent.sigev_notify=SIGEV_NONE;
                        require((op.write_operation?aio_write(&op.aio):aio_read(&op.aio))==0,"submit AIO");op.active=true;
                    }
                }
                if(op.active)wait.push_back(&op.aio);
            }
            if(!wait.empty()) { int r;do {r=aio_suspend(wait.data(),static_cast<int>(wait.size()),nullptr);}while(r<0&&errno==EINTR);require(r==0,"aio_suspend"); }
        }
        require(fsync(dst.value)==0,"fsync");
        double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    std::cout<<std::setprecision(12)<<"bytes,block,operations,seconds,MiB_per_second\n"<<info.st_size<<','<<block<<','<<count<<','<<seconds<<','<<(info.st_size/1048576.0/seconds)<<'\n';
    } catch(const std::exception& e) {
        // Buffers and descriptors must outlive every outstanding operation.
        for(auto& p:ops) if(p->active) {
            aio_cancel(p->aio.aio_fildes,&p->aio);
            const aiocb* request=&p->aio;
            while(aio_error(&p->aio)==EINPROGRESS) aio_suspend(&request,1,nullptr);
            aio_return(&p->aio);p->active=false;
        }
        std::cerr<<e.what()<<'\n';return 1;
    }
}
