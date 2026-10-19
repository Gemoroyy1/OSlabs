from pathlib import Path
r=Path(__file__).resolve().parent
p=r/'rw.cpp';s=p.read_text(encoding='utf-8');s=s.replace('time,pid,role,id,state,page,value','time,qpc_us,pid,role,id,state,page,value').replace('log<<timeGetTime()',"LARGE_INTEGER ticks,freq;QueryPerformanceCounter(&ticks);QueryPerformanceFrequency(&freq);log<<timeGetTime()<<','<<(ticks.QuadPart*1000000/freq.QuadPart)");p.write_text(s,encoding='utf-8')
p=r/'test_lab.py';s=p.read_text(encoding='utf-8').replace("int(r['time'])-int(active['time'])","(int(r['qpc_us'])-int(active['qpc_us']))/1000").replace("int(active['time'])","int(active['qpc_us'])/1000").replace("end=int(r['time'])","end=int(r['qpc_us'])/1000");p.write_text(s,encoding='utf-8')
