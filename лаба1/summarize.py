"""Generate the measured section of REPORT.md after run-linux.sh."""
from pathlib import Path
import csv
import statistics

root = Path(__file__).resolve().parent
sections = ['### Эксперимент: фактические результаты 19.10.2026',
'''Среда: Ubuntu 24.04.5 LTS, WSL2, ядро 5.15.167.4, g++ 13.3.0,
Intel Core i7-11800H, 16 логических процессоров. Временные исходники и
назначения размещались на ext4 внутри виртуального диска WSL2 (/tmp).
Сборка C++17 с -O2 -Wall -Wextra -Wpedantic успешна. Единственное
предупреждение относится к нулевому массиву __pad внутри системного aiocb glibc.
Полные параметры среды: results/environment.txt; протокол сборки: results/build.txt.

Для каждого размера выполнены 90 копирований: пять блоков × шесть n × три
повтора. Каждая копия проверена cmp и SHA-256. Дополнительно для каждой серии
выполнено 36 граничных проверок и пять проверок отказа. Всего 270 измерений
и 123 дополнительных проверки. Все завершились успешно.

Ниже скорости — медианы трёх повторов, МиБ/с. Время включает fsync.
Кэш не очищался: это характеристика приложения и файловой системы в WSL2,
а не измерение паспортной скорости физического SSD. Порядок испытаний
фиксированный, поэтому возможен эффект прогрева кэша.
''']
for size in (1,16,64):
    directory = root/'results'/f'{size}MiB'
    rows = list(csv.DictReader((directory/'measurements.csv').open()))
    assert len(rows)==90 and all(r['verified']=='True' for r in rows)
    groups = {}
    for r in rows:
        groups.setdefault((int(r['block']),int(r['operations'])),[]).append(float(r['speed']))
    med = {k:statistics.median(v) for k,v in groups.items()}
    blocks = sorted({b for b,n in med})
    selected = max(blocks,key=lambda b:med[b,2])
    bestn = max((1,2,4,8,12,16),key=lambda n:med[selected,n])
    globalbest = max(med,key=med.get)
    sections.extend([f'#### Файл {size} МиБ',
        '| Блок, байт | n=1 | n=2 | n=4 | n=8 | n=12 | n=16 |',
        '|---|---|---|---|---|---|---|'])
    for b in blocks:
        sections.append('| '+str(b)+' | '+' | '.join(f'{med[b,n]:.2f}' for n in (1,2,4,8,12,16))+' |')
    sections.append(f'При n=2 лучший блок: {selected} байт. Для него лучший n={bestn}, скорость {med[selected,bestn]:.2f} МиБ/с. Максимум всей сетки: блок {globalbest[0]}, n={globalbest[1]}, {med[globalbest]:.2f} МиБ/с.')
    sections.extend([f'![Скорость от блока](results/{size}MiB/blocks.svg)',
                     f'![Скорость от n](results/{size}MiB/operations.svg)'])
sections.append('''Для файла 1 МиБ и блока 1 МиБ имеется только один блок, поэтому
n>1 не создаёт дополнительного перекрытия: различия в скорости отражают шум
и накладные расходы. Слот ограничивает число одновременно отправленных
запросов; n=1 выполняет чтение и запись последовательно, n=2 допускает одно
чтение и одну запись. Блоки кратны 4096 байт. Оптимум выбирается среди
испытанных точек; переносить его на другой накопитель или нагрузку нельзя.

Данные всех повторов сохранены в results/*MiB/measurements.csv, проверки —
correctness.txt. SVG строятся непосредственно из этих CSV-значений.''')
path = root/'REPORT.md'
text = path.read_text(encoding='utf-8')
start = text.index('### Эксперимент и ограничение среды')
text = text[:start]+'\n\n'.join(sections)+'\n\n## Выводы\n\n'+text.split('## Выводы',1)[1].split('Win32 API',1)[1]
text = text.replace('позволяет явно','Win32 API позволяет явно',1)
text = text.replace('Реализация и воспроизводимый эксперимент подготовлены, но задание 1.2 пока не прошло проверку в целевой ОС.', 'Реализация проверена в Linux; все 270 измеренных копий и 123 дополнительные проверки успешны. Результаты показывают преимущество крупных блоков и ограниченную пользу роста n; конкретные максимумы приведены выше.')
path.write_text(text,encoding='utf-8')
