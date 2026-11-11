#!/usr/bin/env python3
"""Independent subset-DP oracle + output validation; Python standard library."""
import argparse
import functools
import json
from pathlib import Path
import random
import re
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parent / ('src' if (ROOT.parent / 'src').is_dir() else 'codigo')


def read_instance(path):
    tokens = iter(' '.join(line for line in Path(path).read_text().splitlines()
                          if line.strip() and not line.lstrip().startswith('#')).split())
    n, capacity, k = (int(next(tokens)) for _ in range(3))
    sizes = [int(next(tokens)) for _ in range(n)]
    classes = []
    for _ in range(k):
        m = int(next(tokens))
        classes.append(set(int(next(tokens)) - 1 for _ in range(m)))
    return sizes, capacity, classes


def oracle(sizes, capacity, classes):
    n = len(sizes)
    valid = [False] * (1 << n)
    valid[0] = True
    loads = [0] * (1 << n)
    conflicts = [0] * n
    for cls in classes:
        for i in cls:
            conflicts[i] |= sum(1 << j for j in cls if j != i)
    for mask in range(1, 1 << n):
        bit = mask & -mask
        i = bit.bit_length() - 1
        rest = mask ^ bit
        loads[mask] = loads[rest] + sizes[i]
        valid[mask] = valid[rest] and loads[mask] <= capacity and not (rest & conflicts[i])

    @functools.lru_cache(None)
    def dp(mask):
        if not mask:
            return 0
        if valid[mask]:
            return 1
        first = mask & -mask
        best = mask.bit_count()
        subset = mask
        while subset:
            if subset & first and valid[subset]:
                best = min(best, 1 + dp(mask ^ subset))
            subset = (subset - 1) & mask
        return best
    return dp((1 << n) - 1)


def check_output(path, sizes, capacity, classes, optimum):
    lines = [line.strip() for line in Path(path).read_text().splitlines()
             if line.strip() and not line.lstrip().startswith('#')]
    ub, lb, count = map(int, lines[:3])
    bins = [[int(s) - 1 for s in line.split()] for line in lines[3:]]
    assert ub == count == len(bins), (ub, count, bins)
    assert sorted(i for b in bins for i in b) == list(range(len(sizes)))
    for b in bins:
        assert b and sum(sizes[i] for i in b) <= capacity
        assert all(len(set(b) & cls) <= 1 for cls in classes)
    assert 1 <= lb <= optimum <= ub, (lb, optimum, ub)
    return lb, ub


def write_instance(path, sizes, capacity, classes):
    Path(path).write_text(f'{len(sizes)} {capacity} {len(classes)}\n' +
                         ' '.join(map(str, sizes)) + '\n' +
                         ''.join(f'{len(cls)} ' + ' '.join(str(i+1) for i in cls) + '\n'
                                 for cls in classes))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin', default=str(SOURCE / 'binpack'))
    parser.add_argument('--random', type=int, default=120)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    binary = str(Path(args.bin).resolve())
    rng = random.Random(20261111)
    results = []
    branch_count = exact_count = 0
    with tempfile.TemporaryDirectory(prefix='bnp-test-') as directory:
        directory = Path(directory)
        cases = [(path.stem, *read_instance(path)) for path in sorted((ROOT / 'instancias').glob('*.txt'))]
        for index in range(args.random):
            n = rng.randint(4, 12)
            capacity = rng.randint(5, 20)
            sizes = [rng.randint(1, capacity) for _ in range(n)]
            classes = [set(rng.sample(range(n), rng.randint(0, min(n, 4))))
                       for _ in range(rng.randint(0, n))]
            cases.append((f'random-{index:03}', sizes, capacity, classes))
        for index in range(60):
            n = rng.randint(8, 12)
            probability = rng.uniform(0.15, 0.7)
            classes = [{i, j} for i in range(n) for j in range(i+1, n)
                       if rng.random() < probability]
            cases.append((f'graph-{index:03}', [1]*n, n, classes))
        for name, sizes, capacity, classes in cases:
            instance = directory / 'in.txt'
            output = directory / 'out.txt'
            write_instance(instance, sizes, capacity, classes)
            optimum = oracle(sizes, capacity, classes)
            begin = time.monotonic()
            process = subprocess.run([binary, '-f', str(instance), '-o', str(output),
                                      '-t', '5', '-r', '1'], capture_output=True, text=True, timeout=8)
            assert process.returncode == 0, process.stdout + process.stderr
            assert not process.stderr, process.stderr
            lb, ub = check_output(output, sizes, capacity, classes, optimum)
            assert lb == ub == optimum, (name, lb, ub, optimum, process.stdout)
            branch = re.search(r'branches=(\d+)', process.stdout)
            exact = re.search(r'exact_calls=(\d+)', process.stdout)
            branch_count += int(branch[1]) if branch else 0
            exact_count += int(exact[1]) if exact else 0
            results.append(dict(instance=name, n=len(sizes), optimum=optimum,
                                lower_bound=lb, value=ub,
                                branches=int(branch[1]) if branch else 0,
                                exact_calls=int(exact[1]) if exact else 0,
                                seconds=round(time.monotonic()-begin, 4)))
        # An odd cycle on 31 vertices has OPT=3; test interruption at
        # different stages without accepting an invalid global bound.
        assert branch_count > 0, 'suite did not exercise Ryan-Foster branching'
        assert exact_count > 0, 'suite did not exercise exact pricing'
        sizes, capacity, classes = [1]*31, 31, [{i, (i+1)%31} for i in range(31)]
        write_instance(instance, sizes, capacity, classes)
        for limit in ['0', '0.001', '0.03', '0.1']:
            begin = time.monotonic()
            process = subprocess.run([binary, '-f', str(instance), '-o', str(output), '-t', limit],
                                     capture_output=True, text=True, timeout=3)
            assert process.returncode == 0, process.stdout + process.stderr
            check_output(output, sizes, capacity, classes, 3)
            assert time.monotonic()-begin < float(limit)+1.0
        # Parser conventions: duplicates beyond n are ignored, empty classes,
        # leading whitespace comments and maximum 64-bit capacity.
        instance.write_text('  # duplicates\n2 9223372036854775807 2\n'
                            '9223372036854775807 9223372036854775807\n5 1 1 2 2 1\n0\n')
        process = subprocess.run([binary, '-f', str(instance), '-o', str(output), '-t', '1'],
                                 capture_output=True, text=True)
        assert process.returncode == 0, process.stdout + process.stderr
        assert check_output(output, [2**63-1]*2, 2**63-1, [{0,1}, set()], 2) == (2,2)
        for text in ['-1 10 0\n', '4294967297 10 0\n', '1 10 0\n11\n',
                     '1 10 1\n1\n1 2\n', '1 10 0\n999999999999999999999\n']:
            instance.write_text(text)
            assert subprocess.run([binary, '-f', str(instance), '-o', str(output), '-t', '0'],
                                  capture_output=True).returncode != 0
        for option, value in [('-t','nan'), ('-t','inf'), ('-t','abc'), ('-t','-1'),
                              ('-r','-1'), ('-r','abc'), ('-r','4294967296')]:
            command = [binary, '-f', str(ROOT/'instancias/exemplo.txt'), '-o', str(output), '-t','1']
            command += [option,value]
            assert subprocess.run(command,capture_output=True).returncode != 0
    report = dict(cases=len(results), branches=branch_count, exact_calls=exact_count,
                  randomized_seed=20261111, results=results,
                  additional_checks=['timeouts', 'overflow', 'duplicates', 'invalid input', 'CLI'])
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'OK: {len(results)} optima certified; {branch_count} branches; '
          f'{exact_count} exact pricing calls; timeouts/parser/overflow/CLI verified.')


if __name__ == '__main__':
    main()
