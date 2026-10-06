"""Build-time validation must reject damaged dictionaries before packaging."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('package_under_test', ROOT / 'source/inno/package.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


class DictionaryPackageTest(unittest.TestCase):
    def test_official_dictionary(self):
        package.validate_dictionary(ROOT / 'translations/dictionary_zh.json')
        entries = package.payload_entries()
        self.assertEqual(len(entries), len({name.casefold() for name, _, _ in entries}))

    def test_rejects_invalid_dictionary(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'dictionary.json'
            for data in (
                b'{"translations":{"A":"one","A":"two"}}',
                b'{"translations":{"A":"valid"}} trailing',
                b'{"translations":{"A":"\\ud800"}}',
                b'{"translations":{"A":"\\u0000bad"}}',
                b'{"translations":{"A":7}}',
                b'{"translations":{}}',
                b'{"translations":{"A":"\\uXXXX"}}',
                b'{"translations":{"A":"ok"},"meta":NaN}',
                b'\xff',
            ):
                with self.subTest(data=data):
                    path.write_bytes(data)
                    with self.assertRaises((ValueError, UnicodeError)):
                        package.validate_dictionary(path)


if __name__ == '__main__':
    unittest.main()
