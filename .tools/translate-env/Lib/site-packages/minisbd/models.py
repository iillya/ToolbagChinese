import os
import urllib.request
import time
from filelock import FileLock

REPO_URL = "https://github.com/LibreTranslate/MiniSBD/releases/download/v0.0.1/"
MODELS = {
    'en': 'en.onnx',
    'fr': 'fr.onnx',
    'af': 'af.onnx',
    'grc': 'grc.onnx',
    'hbo': 'hbo.onnx',
    'ar': 'ar.onnx',
    'hy': 'hy.onnx',
    'eu': 'eu.onnx',
    'be': 'be.onnx',
    'bg': 'bg.onnx',
    'bxr': 'bxr.onnx',
    'ca': 'ca.onnx',
    'zh-hans': 'zh-hans.onnx',
    'zh-hant': 'zh-hant.onnx',
    'lzh': 'lzh.onnx',
    'cop': 'cop.onnx',
    'hr': 'hr.onnx',
    'cs': 'cs.onnx',
    'da': 'da.onnx',
    'nl': 'nl.onnx',
    'myv': 'myv.onnx',
    'et': 'et.onnx',
    'fo': 'fo.onnx',
    'fi': 'fi.onnx',
    'gl': 'gl.onnx',
    'de': 'de.onnx',
    'got': 'got.onnx',
    'el': 'el.onnx',
    'he': 'he.onnx',
    'hi': 'hi.onnx',
    'hu': 'hu.onnx',
    'is': 'is.onnx',
    'id': 'id.onnx',
    'ga': 'ga.onnx',
    'it': 'it.onnx',
    'ja': 'ja.onnx',
    'kk': 'kk.onnx',
    'ko': 'ko.onnx',
    'kmr': 'kmr.onnx',
    'ky': 'ky.onnx',
    'la': 'la.onnx',
    'lv': 'lv.onnx',
    'lij': 'lij.onnx',
    'lt': 'lt.onnx',
    'qaf': 'qaf.onnx',
    'mt': 'mt.onnx',
    'gv': 'gv.onnx',
    'mr': 'mr.onnx',
    'pcm': 'pcm.onnx',
    'sme': 'sme.onnx',
    'nb': 'nb.onnx',
    'nn': 'nn.onnx',
    'cu': 'cu.onnx',
    'orv': 'orv.onnx',
    'fro': 'fro.onnx',
    'fa': 'fa.onnx',
    'pl': 'pl.onnx',
    'qpm': 'qpm.onnx',
    'pt': 'pt.onnx',
    'ro': 'ro.onnx',
    'ru': 'ru.onnx',
    'sa': 'sa.onnx',
    'gd': 'gd.onnx',
    'sr': 'sr.onnx',
    'sk': 'sk.onnx',
    'sl': 'sl.onnx',
    'es': 'es.onnx',
    'sv': 'sv.onnx',
    'sq': 'sq.onnx',
    'ta': 'ta.onnx',
    'te': 'te.onnx',
    'tr': 'tr.onnx',
    'qtd': 'qtd.onnx',
    'uk': 'uk.onnx',
    'hsb': 'hsb.onnx',
    'ur': 'ur.onnx',
    'ug': 'ug.onnx',
    'vi': 'vi.onnx',
    'cy': 'cy.onnx',
    'hyw': 'hyw.onnx',
    'wo': 'wo.onnx',
    'th': 'th.onnx',
}

def get_user_cache_dir():
    if os.name == 'nt':  # Windows
        return os.path.join(os.getenv('LOCALAPPDATA', os.path.expanduser('~')), 'Cache')
    elif os.name == 'posix':  # Linux or macOS
        if 'darwin' in os.sys.platform:  # macOS
            return os.path.expanduser('~/Library/Caches')
        else:  # Linux
            return os.path.expanduser('~/.cache')
    else:
        return ""

# This can be overridden at runtime    
cache_dir = os.path.join(get_user_cache_dir(), "minisbd")

def list_models():
    return list(MODELS.keys())

def download_models(load_only=None, output=None):
    all_models = list_models()
    if load_only is not None:
        all_models = list(set(load_only) & set(all_models))

    for model in all_models:
        max_retries = 10
        retry_delay = 1
        for attempt in range(max_retries):
            try:
                if output is not None:
                    output(f"Downloading model: {model}")
                get_model_file(model)
                break
            except Exception as e:
                if attempt < max_retries - 1:
                    print(f"Failed to download {model}: {e}. Retrying in {retry_delay}s...")
                    time.sleep(retry_delay)
                    retry_delay *= 2
                else:
                    print(f"Failed to download {model} after {max_retries} attempts: {e}")
                    raise

def get_model_file(name, progress_callback=None):
    if name.startswith("http"):
        url = name
    else:
        model_filename = MODELS.get(name)
        if model_filename is None:
            if os.path.isfile(name):
                return name
            else:
                raise Exception(f"Invalid model: {name}, not in {list_models()}")
        else:
            url = REPO_URL + model_filename
    
    try:
        filename = os.path.basename(url)
        model_path = os.path.join(cache_dir, filename)
        last_update = 0

        def progress(block_num, block_size, total_size):
            nonlocal last_update
            now = time.time()
            if progress_callback is not None and total_size > 0 and now - last_update >= 1:
                progress_callback(f"Downloading model {name}", block_num * block_size / total_size * 5)
                last_update = now

        if not os.path.isfile(model_path):
            os.makedirs(cache_dir, exist_ok=True)

            lock_file = model_path + ".lock"
            lock = FileLock(lock_file, timeout=30)
            with lock:
                if not os.path.isfile(model_path):
                    urllib.request.urlretrieve(url, model_path, progress)
            if os.path.isfile(lock_file):
                try:
                    os.unlink(lock_file)
                except:
                    pass
        return os.path.abspath(model_path)
    except Exception as e:
        # Cleanup possibly corrupted file
        if os.path.isfile(model_path):
            os.unlink(model_path)
        raise e