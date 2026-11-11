#!/usr/bin/env python3
"""Package only submission sources/text; optionally export this Codex session."""
import argparse
import json
from pathlib import Path
import time
import zipfile

ROOT = Path(__file__).resolve().parent.parent
SOURCE = 'src' if (ROOT / 'src').is_dir() else 'codigo'
AI = 'ai' if (ROOT / 'ai').is_dir() else 'ia'
ALLOWED = {'.cpp', '.h', '.py', '.txt', '.md', '.json', '.jsonl', '.pdf'}


def read_records(session):
    records = []
    complete = False
    for line in session.read_text().splitlines():
        try:
            item = json.loads(line)
        except json.JSONDecodeError:
            continue  # a writer may still be appending the last line
        payload = item.get('payload', {})
        kind = item.get('type')
        if kind == 'event_msg':
            if payload.get('type') == 'task_started':
                complete = False
            elif payload.get('type') == 'task_complete':
                complete = True
        if kind != 'response_item':
            continue
        kind = payload.get('type')
        if kind == 'message' and payload.get('role') in ('user', 'assistant'):
            records.append(dict(timestamp=item.get('timestamp'), type=kind,
                                role=payload['role'], content=payload.get('content', [])))
        elif kind in ('custom_tool_call', 'custom_tool_call_output',
                      'function_call', 'function_call_output'):
            records.append(dict(timestamp=item.get('timestamp'), **{
                key: value for key, value in payload.items()
                if key in ('type', 'call_id', 'name', 'input', 'arguments', 'output')}))
    return records, complete


def export(session):
    records, complete = read_records(session)
    folder = ROOT / AI
    folder.mkdir(exist_ok=True)
    (folder / 'conversa.jsonl').write_text(''.join(json.dumps(record, ensure_ascii=False) + '\n'
                                                for record in records))
    sections = ['# Conversa com IA\n\nRegistro cronológico dos prompts, respostas e ferramentas.\n']
    for record in records:
        label = record.get('role', record['type'])
        sections.append(f"\n## {record.get('timestamp')} — {label}\n")
        if record['type'] == 'message':
            for part in record['content']:
                if isinstance(part, dict) and 'text' in part:
                    sections.append(part['text'] + '\n')
                else:
                    sections.append(json.dumps(part, ensure_ascii=False, indent=2) + '\n')
        else:
            # JSON also preserves exact newlines/escaping of tool inputs/outputs.
            sections.append('```json\n' + json.dumps(record, ensure_ascii=False, indent=2) + '\n```\n')
    (folder / 'conversa.md').write_text('\n'.join(sections))
    (folder / 'estado.json').write_text(json.dumps(dict(
        session=session.name, records=len(records), turn_complete=complete), indent=2) + '\n')
    return complete


def package():
    paths = [ROOT / 'README', ROOT / '.clang-tidy', ROOT / '.clang-format']
    for required in (f'{SOURCE}/Makefile', f'{SOURCE}/branchprice.cpp', f'{AI}/README'):
        if not (ROOT / required).is_file():
            raise FileNotFoundError(f'Missing required submission file: {required}')
    for name in (SOURCE, 'testes', AI, 'docs'):
        for path in sorted((ROOT / name).rglob('*')):
            if not path.is_file() or '__pycache__' in path.parts:
                continue
            if path.name in ('Makefile', 'README') or path.suffix in ALLOWED:
                paths.append(path)
    for path in paths:
        # Disallow binary files even if somebody gives one a source suffix.
        if path.suffix != '.pdf' and b'\x00' in path.read_bytes():
            raise ValueError(f'Binary file in submission: {path}')
    output = ROOT / 'entrega-bin-packing.zip'
    temporary = output.with_suffix('.zip.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in paths:
            relative = path.relative_to(ROOT)
            # Workspace names are preserved locally; submission names follow PDF.
            submission_names = {'src': 'codigo', 'ai': 'ia'}
            relative = Path(submission_names.get(relative.parts[0], relative.parts[0]),
                            *relative.parts[1:])
            archive.write(path, relative)
    temporary.replace(output)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessao', type=Path, help='Explicit local session JSONL to export')
    parser.add_argument('--aguardar-fim', action='store_true',
                        help='Refresh export and archive once the active turn finishes (up to 10 minutes)')
    args = parser.parse_args()
    if args.aguardar_fim and args.sessao is None:
        parser.error('--aguardar-fim requires --sessao')
    if args.sessao:
        complete = export(args.sessao)
        package()
        if args.aguardar_fim:
            end = time.monotonic() + 600
            while not complete and time.monotonic() < end:
                time.sleep(0.5)
                _, complete = read_records(args.sessao)
            export(args.sessao)
    output = package()
    print(f'Created {output.name}; only sources, scripts, instances and documentation.')


if __name__ == '__main__':
    main()
