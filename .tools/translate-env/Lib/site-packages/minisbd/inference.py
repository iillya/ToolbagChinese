import onnxruntime as ort
import warnings
import json
warnings.filterwarnings("ignore", message=".*CUDAExecutionProvider.*")

def create_session(model_file, use_gpu=True, max_threads=None):
    options = ort.SessionOptions()
    options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    if max_threads is not None:
        options.execution_mode = ort.ExecutionMode.ORT_SEQUENTIAL
        options.intra_op_num_threads = max_threads

    providers = []
    if use_gpu:
        providers.append("CUDAExecutionProvider")
    providers.append("CPUExecutionProvider")

    session = ort.InferenceSession(model_file, sess_options=options, providers=providers)
    meta = session.get_modelmeta().custom_metadata_map

    lang_code = meta.get('lang_code', '?')
    config = json.loads(meta.get('config', "{}"))
    vocab = json.loads(meta.get('vocab', "{}"))
    lexicon = json.loads(meta.get('lexicon', "null"))
    if len(config) == 0:
        raise Exception("No config found inside model.")
    
    args = {
        'lang_code': lang_code,
        'config': {
            'skip_newline': config.get('skip_newline', False),
            'max_seqlen': config.get('max_seqlen', 1000),
            'no_ssplit': config.get('no_ssplit', False),
            'batch_size': config.get('batch_size', 1),
            'feat_funcs': config.get('feat_funcs', []),
            'use_dictionary': config.get('use_dictionary', False),
            'num_dict_feat': config.get('num_dict_feat', 0),
        },
        'vocab': vocab,
        'lexicon': lexicon
    }
    
    return session, args