#!/usr/bin/env python3
"""Conferência externa de viabilidade; --exato compara com um ótimo independente."""
import argparse
from pathlib import Path
import sys
from verificar import read_instance, oracle, check_output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-f', '--entrada', type=Path, required=True)
    parser.add_argument('-o', '--saida', type=Path, required=True)
    parser.add_argument('--exato', action='store_true', help='Calcula OPT por DP, até 16 itens')
    args = parser.parse_args()
    if not __debug__:
        parser.error('Execute sem -O/PYTHONOPTIMIZE: as verificações usam assert.')
    try:
        sizes, capacity, classes = read_instance(args.entrada)
        lines = [line.strip() for line in args.saida.read_text().splitlines()
                 if line.strip() and not line.lstrip().startswith('#')]
        ub, lb = map(int, lines[:2])
        if args.exato and len(sizes) > 16:
            parser.error('--exato aceita no máximo 16 itens (custo exponencial).')
        optimum = oracle(sizes, capacity, classes) if args.exato else ub
        check_output(args.saida, sizes, capacity, classes, optimum)
    except (AssertionError, ValueError, IndexError, StopIteration, OSError) as error:
        print(f'FALHA: saída inconsistente/inviável ou limitante incompatível. {error}', file=sys.stderr)
        return 1
    print(f'VIÁVEL: {ub} recipientes; partição, capacidade e conflitos conferidos.')
    if args.exato:
        print(f'Ótimo independente: {optimum}; limitante informado: {lb}.')
        print('SOLUÇÃO ÓTIMA confirmada pelo oráculo.' if ub == optimum
              else f'Solução ainda usa {ub-optimum} recipiente(s) além do ótimo.')
    else:
        print('O limitante inferior e a otimalidade não foram certificados por este comando.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
