"""Package allowlisted runtime files with Inno Setup. Build-time Python only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def iss_string(value):
    value = str(value)
    if any(c in value for c in '\r\n\0"{}'):
        raise ValueError(f"Unsupported Inno path: {value!r}")
    return value


def pascal_string(value):
    value = str(value)
    if any(c in value for c in '\r\n\0'):
        raise ValueError('Invalid Pascal literal')
    return "'" + value.replace("'", "''") + "'"


def validate_dictionary(source):
    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"Duplicate dictionary field: {key!r}")
            result[key] = value
        return result

    def invalid_constant(value):
        raise ValueError(f"Invalid JSON constant: {value}")

    if not 0 < source.stat().st_size <= 64 * 1024 * 1024:
        raise ValueError('Dictionary must be nonempty and at most 64 MiB')
    data = json.loads(source.read_text(encoding='utf-8-sig'),
                      object_pairs_hook=unique_object, parse_constant=invalid_constant)
    translations = data.get('translations') if isinstance(data, dict) else None
    if not isinstance(translations, dict) or not translations:
        raise ValueError('Dictionary requires a nonempty translations object')
    for key, value in translations.items():
        if not isinstance(value, str):
            raise ValueError(f'Non-string translation: {key!r}')
        if '\0' in key or '\0' in value:
            raise ValueError('Dictionary strings cannot contain embedded NUL')
        key.encode('utf-8')
        value.encode('utf-8')  # Reject unpaired Unicode surrogates.
    if not any(key and value for key, value in translations.items()):
        raise ValueError('Dictionary has no usable translations')


def payload_entries():
    cfg = json.loads((HERE / 'product.json').read_text(encoding='utf-8'))
    entries = []
    for name, relative in cfg['payload'].items():
        if not re.fullmatch(r'[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)*', name) or '..' in name.split('/'):
            raise ValueError('Unsafe payload destination')
        source = ROOT / relative
        if source.is_symlink() or not source.is_file() or not source.resolve().is_relative_to(ROOT):
            raise ValueError(f'Unsafe/missing payload: {source}')
        if name == 'dictionary_zh.json':
            validate_dictionary(source)
        entries.append((name.replace('/', '\\'), source, hashlib.sha256(source.read_bytes()).hexdigest()))
    names = [name.casefold() for name, _, _ in entries]
    if len(names) != len(set(names)) or not any(n.endswith('dictionary_zh.json') for n in names):
        raise ValueError('Duplicate payload or missing dictionary')
    return entries


def includes(entries):
    files, code = [], [f'SetArrayLength(PayloadNames, {len(entries)});',
                       f'SetArrayLength(PayloadHashes, {len(entries)});']
    for index, (name, source, digest) in enumerate(entries):
        parent, filename = name.rsplit('\\', 1) if '\\' in name else ('', name)
        target = '{app}\\ChineseLauncher' + ('\\' + parent if parent else '')
        flags, check = 'ignoreversion', ''
        if name.casefold().startswith('translations\\') or name.casefold() in ('dictionary_zh.json', 'settings.ini'):
            flags += ' uninsneveruninstall'
            check = f'; Check: ShouldInstallDictionary({pascal_string(name)}, {pascal_string(digest)})'
            defaults = '{app}\\ChineseLauncher\\.inno\\defaults' + ('\\' + parent if parent else '')
            files.append(f'Source: "{iss_string(source)}"; DestDir: "{defaults}"; '
                         f'DestName: "{iss_string(filename)}"; Flags: ignoreversion')
        files.append(f'Source: "{iss_string(source)}"; DestDir: "{target}"; '
                     f'DestName: "{iss_string(filename)}"; Flags: {flags}{check}')
        code.append(f'PayloadNames[{index}] := {pascal_string(name)};')
        code.append(f'PayloadHashes[{index}] := {pascal_string(digest)};')
    return '\n'.join(files) + '\n', '\n'.join(code) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iscc', type=Path, default=os.environ.get('INNO_ISCC', ROOT.parent / '_ThirdParty/InnoSetup/7.1.0/ISCC.exe'))
    parser.add_argument('--version', default='1.0.4')
    parser.add_argument('--test-mode', action='store_true')
    args = parser.parse_args()
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:\.\d+)?', args.version): parser.error('Invalid version')
    if not args.iscc.is_file(): parser.error('Inno Setup 7.1 is missing; set INNO_ISCC.')
    cfg = json.loads((HERE / 'product.json').read_text(encoding='utf-8'))
    support = ROOT / 'build/inno/support.dll'
    if not support.is_file(): parser.error('Build support.dll first: source/build_inno.bat')
    entries = payload_entries()
    files, code = includes(entries)
    build = ROOT / 'build/inno'
    with tempfile.TemporaryDirectory(prefix='package-', dir=build) as temporary:
        stage = Path(temporary)
        files_path, code_path = stage / 'payload.iss', stage / 'payload-code.iss'
        files_path.write_text(files, encoding='utf-8-sig')
        code_path.write_text(code, encoding='utf-8-sig')
        command = [str(args.iscc), f'/DPayloadInclude={files_path}', f'/DPayloadCode={code_path}',
                   f'/DSupportDll={support}', f'/DPackageOutput={stage}', f'/DPackageVersion={args.version}']
        if args.test_mode: command.append('/DTestMode=1')
        command.append(str(HERE / (cfg['id'] + '.iss')))
        subprocess.run(command, check=True, cwd=HERE)
        target = build / 'test-package' if args.test_mode else ROOT / 'dist'
        target.mkdir(parents=True, exist_ok=True)
        destination = target / (cfg['id'] + 'Installer.exe')
        os.replace(stage / destination.name, destination)
    manifest = {'version': args.version, 'test_mode': args.test_mode, 'installer': str(destination),
                'sha256': hashlib.sha256(destination.read_bytes()).hexdigest(),
                'payload': [{'name': name, 'sha256': digest} for name, _, digest in entries]}
    (build / ('test-manifest.json' if args.test_mode else 'manifest.json')).write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'Built Inno Setup package: {destination}')


if __name__ == '__main__':
    main()
