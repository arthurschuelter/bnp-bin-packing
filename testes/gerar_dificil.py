#!/usr/bin/env python3
"""Gera uma instância difícil, de ótimo conhecido, para o branch-and-price.

Construção (estilo "tripletos" de Falkenauer, com classes de conflito):
  * m recipientes de capacidade C; cada recipiente da solução planejada recebe
    exatamente três itens com tamanhos em (C/4, C/2) e soma igual a C.
    Logo n = 3m, a soma dos tamanhos é m*C e o limitante trivial é m.
  * Nenhum recipiente comporta quatro itens (todos são > C/4), e qualquer
    solução com m recipientes precisa enchê-los exatamente. Há muitas
    combinações quase cheias, o que torna o LP degenerado e as heurísticas
    de construção (FFD) tipicamente erram por um ou mais recipientes.
  * As classes de conflito contêm no máximo um item de cada tripleto
    planejado, então a solução planejada continua viável: o ótimo é m.

Uso: python3 gerar_dificil.py -m 60 -C 1000 -k 30 --seed 7 -o dificil.txt

Com --tipo aleatorio, gera n = 3m itens com tamanhos uniformes em [C/5, C/2]
e classes aleatórias (como medium-1). Nesse caso o ótimo não é conhecido.
"""
import argparse
import random
from pathlib import Path


def triplet(rng, capacity):
    low, high = capacity // 4 + 1, (capacity - 1) // 2
    while True:
        a = rng.randint(low, high)
        b = rng.randint(low, high)
        c = capacity - a - b
        if low <= c <= high:
            return [a, b, c]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-m', type=int, default=60, help='recipientes da solução ótima')
    parser.add_argument('-C', type=int, default=1000, help='capacidade')
    parser.add_argument('-k', type=int, default=30, help='número de classes de conflito')
    parser.add_argument('--tam-classe', type=int, default=6, help='tamanho máximo de cada classe')
    parser.add_argument('--seed', type=int, default=7)
    parser.add_argument('--tipo', choices=['tripletos', 'aleatorio'], default='tripletos')
    parser.add_argument('-o', type=Path, required=True)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    if args.tipo == 'aleatorio':
        n = 3 * args.m
        sizes = [rng.randint(args.C // 5, args.C // 2) for _ in range(n)]
        classes = [sorted(rng.sample(range(1, n + 1), rng.randint(2, args.tam_classe)))
                   for _ in range(args.k)]
        lines = [f'{n} {args.C} {args.k}', ' '.join(map(str, sizes))]
        lines += [f'{len(c)} ' + ' '.join(map(str, c)) for c in classes]
        args.o.write_text('\n'.join(lines) + '\n')
        print(f'{args.o}: n={n}, C={args.C}, k={args.k}, aleatória (ótimo desconhecido)')
        return
    triplets = [triplet(rng, args.C) for _ in range(args.m)]
    # Embaralha a numeração para não revelar os tripletos.
    order = list(range(3 * args.m))
    rng.shuffle(order)
    sizes = [0] * (3 * args.m)
    group = [0] * (3 * args.m)
    for idx, pos in enumerate(order):
        sizes[pos] = triplets[idx // 3][idx % 3]
        group[pos] = idx // 3

    classes = []
    for _ in range(args.k):
        size = rng.randint(2, args.tam_classe)
        chosen_groups = rng.sample(range(args.m), size)
        members = [rng.choice([i for i in range(3 * args.m) if group[i] == g])
                   for g in chosen_groups]
        classes.append(sorted(i + 1 for i in members))

    lines = [f'{3 * args.m} {args.C} {args.k}', ' '.join(map(str, sizes))]
    lines += [f'{len(c)} ' + ' '.join(map(str, c)) for c in classes]
    args.o.write_text('\n'.join(lines) + '\n')
    print(f'{args.o}: n={3 * args.m}, C={args.C}, k={args.k}, ótimo={args.m}')


if __name__ == '__main__':
    main()
