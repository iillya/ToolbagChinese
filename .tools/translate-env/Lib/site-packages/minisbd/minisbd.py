import os
from .models import get_model_file, download_models
from .inference import create_session
from minisbd.modules import Vocab, TokenizationDataset, output_predictions, get_sentences, create_dictionary


class SBDetect:
    def __init__(self, lang, use_gpu=True, max_threads=None):
        self.max_threads = max_threads
        self.model_file = get_model_file(lang)
        self.session, args = create_session(self.model_file, use_gpu=use_gpu, max_threads=max_threads)

        self.config = args['config']
        self.vocab = Vocab.load_state_dict(args['vocab'])
        self.dictionary = create_dictionary(args['lexicon'])

    def sentences(self, text):
        text = '\n\n'.join(text) if isinstance(text, list) else text
        batches = TokenizationDataset(self.config, text=text, vocab=self.vocab, dictionary=self.dictionary)

        _, _, _, document = output_predictions(self.config, self.session, batches, self.vocab, orig_text=text)

        return get_sentences(document, text)

