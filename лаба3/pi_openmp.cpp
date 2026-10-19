#include <omp.h>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
int threads=argc>1?std::stoi(argv[1]):1;
if(threads<1||threads>64)throw std::runtime_error("threads 1..64");
constexpr long long N=10000000, block=2306160;
double sum=0;int actual=0;omp_set_dynamic(0);
auto start=std::chrono::steady_clock::now();
#pragma omp parallel num_threads(threads) reduction(+:sum)
{
#pragma omp single
actual=omp_get_num_threads();
#pragma omp for schedule(dynamic,block)
for(long long i=0;i<N;++i){double x=(i+0.5)/N;sum+=4.0/(1+x*x);}
}
double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
std::cout<<std::setprecision(17)<<"threads,actual,N,block,pi,seconds\n"<<threads<<','<<actual<<','<<N<<','<<block<<','<<sum/N<<','<<seconds<<'\n';
return std::abs(sum/N-std::acos(-1.0))<1e-11?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
