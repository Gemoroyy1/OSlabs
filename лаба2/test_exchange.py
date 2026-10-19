import subprocess,time, pathlib
root=pathlib.Path(__file__).resolve().parent
with (root/'linux-tests.txt').open('w') as log:
    for i,message in enumerate(('Hello, shared memory!','Серов И.С. группа 4312','')):
        path=f'/tmp/oslab2-test-{i}.bin';name=f'/oslab2-test-{i}'
        server=subprocess.Popen([str(root/'build/server'),path,name],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        server.stdin.write('1\n');server.stdin.flush()
        deadline=time.monotonic()+10
        while not pathlib.Path('/dev/shm/sem.'+name[1:]+'-read').exists():
            if time.monotonic()>deadline:server.kill();raise RuntimeError('Server setup timeout')
            time.sleep(.02)
        client=subprocess.Popen([str(root/'build/client'),path,name],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        server.stdin.write('2\n'+message+'\n0\n');server.stdin.flush()
        output,_=client.communicate('1\n2\n0\n',timeout=10)
        sout,_=server.communicate(timeout=10)
        assert 'Client received: '+message+'\n' in output and 'Client acknowledged' in sout
        assert not pathlib.Path(path).exists()
        log.write(f'CASE {i}: PASS\nSERVER\n{sout}\nCLIENT\n{output}\n')
print('3 exchanges passed, files removed')
