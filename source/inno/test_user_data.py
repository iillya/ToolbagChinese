"""Isolated Inno upgrade/uninstall regression for user-owned Toolbag files.

Run from the repository root: python source/inno/test_user_data.py
Only the TestMode AppId and temporary fake host directories are used.
"""
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import unittest

import package


ROOT = package.ROOT
BUILD = ROOT / 'build/inno'
ISCC = ROOT.parent / '_ThirdParty/InnoSetup/7.1.0/ISCC.exe'
TEST_ID = r'ToolbagChinese.Inno.IsolatedTests_is1'
TEST_REGISTRY = 'HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\' + TEST_ID


def run(command, *, cwd=None):
    subprocess.run([str(arg) for arg in command], cwd=cwd, check=True, timeout=120,
                   stdout=subprocess.DEVNULL)


def compile_variant(output, dictionary):
    entries = []
    for name, source, digest in package.payload_entries():
        if name == 'dictionary_zh.json':
            source = dictionary
            digest = hashlib.sha256(source.read_bytes()).hexdigest()
        entries.append((name, source, digest))
    files, code = package.includes(entries)
    files_path, code_path = output / 'payload.iss', output / 'payload-code.iss'
    files_path.write_text(files, encoding='utf-8-sig')
    code_path.write_text(code, encoding='utf-8-sig')
    run([ISCC, f'/DPayloadInclude={files_path}', f'/DPayloadCode={code_path}',
         f'/DSupportDll={BUILD / "support.dll"}', f'/DPackageOutput={output}',
         '/DPackageVersion=1.0.3', '/DTestMode=1', package.HERE / 'ToolbagChinese.iss'],
        cwd=package.HERE)
    return output / 'ToolbagChineseInstaller.exe'


def compile_legacy(output, dictionary=None):
    """Reproduce the released recursive uninstall log under the isolated AppId."""
    sample = output / 'legacy-dictionary.json'
    if dictionary is None:
        sample.write_text('{"original":"旧版默认"}\n', encoding='utf-8')
    else:
        shutil.copy2(dictionary, sample)
    script = output / 'legacy.iss'
    script.write_text('\n'.join([
        '[Setup]',
        'AppId=ToolbagChinese.Inno.IsolatedTests',
        'AppName=Toolbag Legacy Isolated Test',
        'AppVersion=1.0.2',
        'DefaultDirName={userappdata}\\ToolbagInstallerTest',
        'AppendDefaultDirName=no',
        'PrivilegesRequired=lowest',
        'ArchitecturesAllowed=x64os',
        'ArchitecturesInstallIn64BitMode=x64os',
        'UninstallFilesDir={app}\\ChineseLauncher\\.inno',
        f'OutputDir={output}',
        'OutputBaseFilename=legacy',
        '[Files]',
        f'Source: "{sample}"; DestDir: "{{app}}\\ChineseLauncher"; '
        'DestName: "dictionary_zh.json"; Flags: ignoreversion',
        '[UninstallDelete]',
        'Type: filesandordirs; Name: "{app}\\ChineseLauncher"',
    ]) + '\n', encoding='utf-8-sig')
    run([ISCC, script])
    return output / 'legacy.exe'


class UserDataTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for path in (ISCC, BUILD / 'support.dll', ROOT / 'build/out/ToolbagChineseLauncher.exe'):
            if not path.is_file():
                raise unittest.SkipTest(f'Missing build dependency: {path}')
        if subprocess.run(['reg', 'query', TEST_REGISTRY], capture_output=True).returncode == 0:
            raise RuntimeError('The isolated TestMode AppId is already installed; refusing to reuse it')
        (BUILD / 'tests').mkdir(parents=True, exist_ok=True)
        cls.temp = tempfile.TemporaryDirectory(prefix='user-data-', dir=BUILD / 'tests')
        cls.work = Path(cls.temp.name)
        cls.legacy = compile_legacy(cls.work)
        legacy_published = cls.work / 'legacy-published'
        legacy_published.mkdir()
        cls.legacy_published = compile_legacy(legacy_published, ROOT / 'translations/dictionary_zh.json')
        cls.version_one = compile_variant(cls.work, ROOT / 'translations/dictionary_zh.json')
        cls.default_one = (ROOT / 'translations/dictionary_zh.json').read_bytes()
        cls.version_two_default = cls.work / 'v2-dictionary.json'
        cls.version_two_default.write_text('{"new":"新版内置词典"}\n', encoding='utf-8')
        variant = cls.work / 'v2'
        variant.mkdir()
        cls.version_two = compile_variant(variant, cls.version_two_default)

    @classmethod
    def tearDownClass(cls):
        for attempt in range(30):
            try:
                cls.temp.cleanup()
                return
            except (PermissionError, NotADirectoryError):
                if attempt == 29:
                    raise
                time.sleep(0.5)

    def setUp(self):
        self.host = self.work / self.id().rsplit('.', 1)[-1]
        self.host.mkdir()
        shutil.copy2(ROOT / 'build/out/ToolbagChineseLauncher.exe', self.host / 'toolbag.exe')
        (self.host / 'data/gui/font').mkdir(parents=True)
        self.launcher = self.host / 'ChineseLauncher'

    def install(self, installer):
        run([installer, '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART',
             f'/DIR={self.host}'])

    def uninstall(self):
        uninstaller = self.launcher / '.inno/unins000.exe'
        self.assertTrue(uninstaller.is_file())
        run([uninstaller, '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'])
        for _ in range(40):
            registered = subprocess.run(['reg', 'query', TEST_REGISTRY], capture_output=True).returncode == 0
            if not registered and not uninstaller.exists():
                return
            time.sleep(0.25)
        self.fail('Isolated uninstaller did not complete cleanup')

    def test_old_recursive_log_is_replaced(self):
        self.install(self.legacy)
        custom_dictionary = b'{"user":"do not erase"}\n'
        (self.launcher / 'dictionary_zh.json').write_bytes(custom_dictionary)
        (self.launcher / 'settings.ini').write_text('custom=true\n', encoding='utf-8')
        (self.launcher / 'translations').mkdir()
        (self.launcher / 'translations/custom.json').write_text('custom', encoding='utf-8')

        self.install(self.version_one)
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(), custom_dictionary)
        self.assertEqual((self.launcher / '.inno/defaults/dictionary_zh.json').read_bytes(), self.default_one)
        self.uninstall()
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(), custom_dictionary)
        self.assertEqual((self.launcher / 'settings.ini').read_text(encoding='utf-8'), 'custom=true\n')
        self.assertTrue((self.launcher / 'translations/custom.json').is_file())

    def test_default_updates_but_modified_dictionary_survives(self):
        self.install(self.version_one)
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(), self.default_one)
        self.install(self.version_two)
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(),
                         self.version_two_default.read_bytes())
        custom_dictionary = b'{"user":"my translations"}\n'
        (self.launcher / 'dictionary_zh.json').write_bytes(custom_dictionary)
        (self.launcher / 'settings.ini').write_text('custom=true\n', encoding='utf-8')
        self.install(self.version_one)
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(), custom_dictionary)
        self.assertEqual((self.launcher / '.inno/defaults/dictionary_zh.json').read_bytes(), self.default_one)
        self.uninstall()
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(), custom_dictionary)
        self.assertTrue((self.launcher / 'settings.ini').is_file())

    def test_released_default_updates_without_old_hash_journal(self):
        self.install(self.legacy_published)
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(), self.default_one)
        self.install(self.version_two)
        self.assertEqual((self.launcher / 'dictionary_zh.json').read_bytes(),
                         self.version_two_default.read_bytes())
        self.uninstall()
        self.assertFalse((self.launcher / 'dictionary_zh.json').exists())

    def test_unmodified_dictionary_removed_on_uninstall(self):
        self.install(self.version_one)
        self.uninstall()
        self.assertFalse((self.launcher / 'dictionary_zh.json').exists())

    def test_fonts_restore_without_overwriting_external_edits(self):
        fonts = self.host / 'data/gui/font'
        originals = {}
        for name in ('notosans_chinese.slug', 'segoeui.slug', 'selawik.slug'):
            originals[name] = ('original-' + name).encode('ascii')
            (fonts / name).write_bytes(originals[name])
        self.install(self.version_one)
        translated_font = (ROOT / 'fonts/ToolbagChineseFont.slug').read_bytes()
        for name in originals:
            self.assertEqual((fonts / name).read_bytes(), translated_font)
        self.install(self.version_two)
        for name, original in originals.items():
            self.assertEqual((self.launcher / '.inno/font-backups' / name).read_bytes(), original)
        external = b'font changed after localization installation'
        (fonts / 'selawik.slug').write_bytes(external)
        self.uninstall()
        for name, original in originals.items():
            self.assertEqual((fonts / name).read_bytes(), external if name == 'selawik.slug' else original)


if __name__ == '__main__':
    unittest.main()
