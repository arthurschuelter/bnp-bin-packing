#!/usr/bin/env python3
"""Exporta os arquivos de continuidade sem renomear a estrutura de trabalho."""
import argparse
import hashlib
import json
from pathlib import Path
import time
import zipfile

from empacotar import ROOT, export, read_records

DIRECTORIES = ('src', 'testes', 'ai', 'docs', 'projeto-template', '.vscode')
ROOT_FILES = ('AGENTS.md', 'README', 'Makefile', '.gitignore', '.clang-format',
              '.clang-tidy', 'enunciado.pdf')
SUFFIXES = {'.cpp', '.h', '.py', '.txt', '.md', '.json', '.jsonl', '.pdf'}


def package_context():
    paths = [ROOT / name for name in ROOT_FILES]
    for directory in DIRECTORIES:
        if not (ROOT / directory).is_dir():
            raise FileNotFoundError(f'Diretório ausente: {directory}')
        for path in sorted((ROOT / directory).rglob('*')):
            if path.is_symlink() or not path.is_file() or '__pycache__' in path.parts:
                continue
            if path.name in ('Makefile', 'README') or path.suffix in SUFFIXES:
                paths.append(path)
    manifest = {}
    output = ROOT / 'contexto-codex.zip'
    temporary = output.with_suffix('.zip.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in paths:
            data = path.read_bytes()
            if path.suffix != '.pdf' and b'\0' in data:
                raise ValueError(f'Arquivo binário inesperado: {path.relative_to(ROOT)}')
            relative = path.relative_to(ROOT).as_posix()
            manifest[relative] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
            archive.writestr(relative, data)
        archive.writestr('CONTEXTO_MANIFESTO.json',
                         json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    temporary.replace(output)
    return output, len(manifest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessao', type=Path, help='Registro local desta conversa para atualizar ai/')
    parser.add_argument('--aguardar-fim', action='store_true',
                        help='Atualiza uma última vez quando a resposta final for registrada')
    args = parser.parse_args()
    if args.aguardar_fim and not args.sessao:
        parser.error('--aguardar-fim requer --sessao')
    if args.sessao:
        export(args.sessao)
    output, count = package_context()
    print(f'{output.name}: {count} arquivos e manifesto SHA-256.', flush=True)
    if args.aguardar_fim:
        end = time.monotonic() + 600
        _, complete = read_records(args.sessao)
        while not complete and time.monotonic() < end:
            time.sleep(0.5)
            _, complete = read_records(args.sessao)
        export(args.sessao)
        output, count = package_context()
        print(f'Atualizado: {output.name}, {count} arquivos.', flush=True)


if __name__ == '__main__':
    main()
