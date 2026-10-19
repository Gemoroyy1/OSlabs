import subprocess,csv,time,uuid
from pathlib import Path
root=Path(__file__).resolve().parent;out=root/'results';out.mkdir(exist_ok=True)
run='test_'+uuid.uuid4().hex[:8]
processes=[]
for role in ('reader','writer'):
    for i in range(1,9):processes.append(subprocess.Popen([str(root/'rw.exe'),role,str(i),run,'3'],cwd=out,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True))
for proc in processes:
    log=proc.communicate(timeout=40)[0]
    if proc.returncode:raise RuntimeError(log)
rows=[]
for file in out.glob(run+'_*.csv'):
    data=list(csv.DictReader(file.open()));assert len([r for r in data if r['state'] in ('reading','writing')])==3
    assert data[0]['state']=='locked' and data[-1]['state']=='finished';rows+=data
# Releasing is logged before the actual unlock. Therefore another access must
# never start before the prior release record; equal millisecond timestamps
# cannot establish ordering, so only strictly overlapping intervals are checked.
intervals=[]
for pid in {r['pid'] for r in rows}:
    active=None
    for r in [r for r in rows if r['pid']==pid]:
        if r['state'] in ('reading','writing'):active=r
        elif r['state']=='releasing':
            assert active is not None
            duration=((int(r['qpc_us'])-int(active['qpc_us']))/1000)%(2**32)
            assert 490<=duration<=2000
            intervals.append(dict(pid=pid,role=r['role'],id=r['id'],page=int(r['page']),start=int(active['qpc_us'])/1000,end=int(r['qpc_us'])/1000,duration=duration));active=None
for i,a in enumerate(intervals):
    for b in intervals[i+1:]:
        if a['page']==b['page'] and 'writer' in (a['role'],b['role']):assert max(a['start'],b['start'])>=min(a['end'],b['end'])
with (out/'intervals.csv').open('w',newline='') as f:
    w=csv.DictWriter(f,fieldnames=list(intervals[0]));w.writeheader();w.writerows(intervals)
proof=['PASS 16 processes, 16 pages, VirtualLock confirmed in every process','PASS 48 operations and no forbidden overlapping access']
for i,message in enumerate(('Hello named pipe','Serov IS 4312','')):
    pipe='\\\\.\\pipe\\OSlab4_'+uuid.uuid4().hex
    server=subprocess.Popen([str(root/'pipe_server.exe'),pipe],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    server.stdin.write('1\n2\n3\n');server.stdin.flush();time.sleep(.3)
    client=subprocess.Popen([str(root/'pipe_client.exe'),pipe],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    server.stdin.write('4\n'+message+'\n');server.stdin.flush()
    cout=client.communicate('1\n2\n0\n',timeout=10)[0]
    assert client.returncode==0 and 'Received: '+message+'\n' in cout
    sout=server.communicate('5\n0\n',timeout=10)[0];assert server.returncode==0
    (out/f'pipe-{i}.txt').write_text('SERVER\n'+sout+'\nCLIENT\n'+cout)
    proof.append(f'PASS pipe case {i}: {message!r}')
(out/'checks.txt').write_text('\n'.join(proof)+'\n')
print('\n'.join(proof))
