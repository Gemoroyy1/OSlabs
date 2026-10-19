"""Linux: python3 benchmark.py --exe ./build/aio_copy --mib 64 --repeats 3."""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import statistics
import subprocess
import tempfile

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for data in iter(lambda: f.read(1024 * 1024), b''):
            h.update(data)
    return h.digest()

def chart(path, rows, key, title):
    groups = {}
    for row in rows:
        groups.setdefault(row[key], []).append(row['speed'])
    points = [(x, statistics.median(y)) for x, y in sorted(groups.items())]
    peak = max(y for _, y in points) or 1
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="900" height="440">',
           '<rect width="100%" height="100%" fill="white"/>',
           f'<text x="40" y="30">{title} (median, MiB/s)</text>',
           '<path d="M70 60 V370 H860" stroke="black" fill="none"/>']
    coords = []
    for i, (x, y) in enumerate(points):
        px, py = 90 + i * 740 / max(1, len(points)-1), 360 - y / peak * 280
        coords.append(f'{px},{py}')
        svg.extend([f'<circle cx="{px}" cy="{py}" r="4" fill="blue"/>',
                    f'<text x="{px}" y="395" text-anchor="middle">{x}</text>',
                    f'<text x="{px}" y="{py-10}" text-anchor="middle">{y:.1f}</text>'])
    svg.append(f'<polyline points="{" ".join(coords)}" fill="none" stroke="blue"/></svg>')
    path.write_text('\n'.join(svg), encoding='utf-8')
    return max(points, key=lambda p: p[1])

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', default='./build/aio_copy')
    p.add_argument('--mib', type=int, default=64)
    p.add_argument('--repeats', type=int, default=3)
    p.add_argument('--out', default='results')
    a = p.parse_args()
    if a.mib <= 0 or a.repeats <= 0:
        p.error('mib and repeats must be positive')
    exe = str(Path(a.exe).resolve())
    rows = []
    out = Path(a.out); out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='oslab-') as temp:
        source, target = Path(temp)/'source.bin', Path(temp)/'target.bin'
        def run(block, count):
            if target.exists(): target.unlink()
            result = subprocess.run([exe, str(source), str(target), str(block), str(count)],
                                    check=True, text=True, capture_output=True)
            row = next(csv.DictReader(result.stdout.splitlines()))
            subprocess.run(['cmp', str(source), str(target)], check=True)
            assert digest(source) == digest(target), 'SHA-256 mismatch'
            return float(row['seconds']), float(row['MiB_per_second'])
        source.write_bytes(b'')
        cluster = source.stat().st_blksize
        checks = []
        for size in (0, 1, cluster-1, cluster, cluster+1, 100003):
            source.write_bytes(os.urandom(size))
            for count in (1, 2, 4, 8, 12, 16):
                run(cluster, count)
                checks.append(f'PASS size={size} operations={count} cmp SHA256')
        # Existing targets (including source itself), invalid input and missing
        # files must be rejected without changing the source.
        before = digest(source)
        for args in ((str(source), str(source), str(cluster), '2'),
                     (str(source), str(target), str(cluster), '2'),
                     (str(source), str(target), '1', '2'),
                     (str(source), str(target), str(cluster), '0'),
                     (str(source)+'missing', str(target), str(cluster), '2')):
            result = subprocess.run([exe, *args], text=True, capture_output=True)
            assert result.returncode != 0, args
            assert digest(source) == before
            checks.append(f'PASS rejected {args}: {result.stderr.strip()}')
        (out/'correctness.txt').write_text('\n'.join(checks)+'\n')
        with source.open('wb') as f:
            for _ in range(a.mib): f.write(os.urandom(1024*1024))
        for block in (cluster, cluster*4, cluster*16, cluster*64, cluster*256):
            for count in (1, 2, 4, 8, 12, 16):
                for repeat in range(a.repeats):
                    seconds, speed = run(block, count)
                    rows.append(dict(block=block, operations=count, repeat=repeat,
                                     seconds=seconds, speed=speed, verified=True))
                    print(rows[-1], flush=True)
    with (out/'measurements.csv').open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    best_block, speed = chart(out/'blocks.svg', [r for r in rows if r['operations']==2],
                             'block', 'Block size, operations=2')
    best_count, speed = chart(out/'operations.svg', [r for r in rows if r['block']==best_block],
                             'operations', f'Concurrent operations, block={best_block}')
    (out/'summary.txt').write_text(f'Best measured block: {best_block}\nBest measured concurrency: {best_count}\nMedian speed: {speed:.3f} MiB/s\n')

if __name__ == '__main__':
    main()
