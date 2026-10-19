from pathlib import Path
import subprocess,csv,statistics,random,math,json,platform
root=Path(__file__).resolve().parent
out=root/'results';out.mkdir(exist_ok=True)
rows=[]
jobs=[(impl,n,r) for impl in ('win32','openmp') for n in (1,2,4,8,12,16) for r in range(10)]
random.Random(230616).shuffle(jobs)
for impl in ('win32','openmp'):
    subprocess.run([str(root/f'pi_{impl}.exe'),'4'],check=True,capture_output=True)
for impl,n,r in jobs:
    proc=subprocess.run([str(root/f'pi_{impl}.exe'),str(n)],check=True,capture_output=True,text=True,timeout=15)
    row=next(csv.DictReader(proc.stdout.splitlines()))
    assert int(row['actual'])==n and int(row['block'])==2306160
    assert abs(float(row['pi'])-math.pi)<1e-11
    row.update(implementation=impl,repeat=r,error=abs(float(row['pi'])-math.pi));rows.append(row)
with (out/'measurements.csv').open('w',newline='') as f:
    w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
(out/'environment.json').write_text(json.dumps(dict(platform=platform.platform(),processor=platform.processor(),N=10000000,student=230616,block=2306160,repeats=10,timing='Includes thread creation and teardown for both implementations',compiler='MinGW g++ -O2 C++17; OpenMP -fopenmp'),indent=2))
print('PASS: 120 runs, pi error < 1e-11, actual thread counts verified')
