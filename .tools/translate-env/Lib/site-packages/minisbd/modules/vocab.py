from collections import Counter, OrderedDict
from collections.abc import Iterable
import re
import os
import pickle

PAD = '<PAD>'
PAD_ID = 0
UNK = '<UNK>'
UNK_ID = 1
EMPTY = '<EMPTY>'
EMPTY_ID = 2
ROOT = '<ROOT>'
ROOT_ID = 3
VOCAB_PREFIX = [PAD, UNK, EMPTY, ROOT]
VOCAB_PREFIX_SIZE = len(VOCAB_PREFIX)

class BaseVocab:
    """ A base class for common vocabulary operations. Each subclass should at least 
    implement its own build_vocab() function."""
    def __init__(self, data=None, lang="", idx=0, cutoff=0, lower=False):
        self.data = data
        self.lang = lang
        self.idx = idx
        self.cutoff = cutoff
        self.lower = lower
        if data is not None:
            self.build_vocab()
        self.state_attrs = ['lang', 'idx', 'cutoff', 'lower', '_unit2id', '_id2unit']

    def build_vocab(self):
        raise NotImplementedError("This BaseVocab does not have build_vocab implemented.  This method should create _id2unit and _unit2id")

    def state_dict(self):
        """ Returns a dictionary containing all states that are necessary to recover
        this vocab. Useful for serialization."""
        state = OrderedDict()
        for attr in self.state_attrs:
            if hasattr(self, attr):
                state[attr] = getattr(self, attr)
        return state

    @classmethod
    def load_state_dict(cls, state_dict):
        """ Returns a new Vocab instance constructed from a state dict. """
        new = cls()
        for attr, value in state_dict.items():
            setattr(new, attr, value)
        return new

    def normalize_unit(self, unit):
        # be sure to look in subclasses for other normalization being done
        # especially PretrainWordVocab
        if unit is None:
            return unit
        if self.lower:
            return unit.lower()
        return unit

    def unit2id(self, unit):
        unit = self.normalize_unit(unit)
        if unit in self._unit2id:
            return self._unit2id[unit]
        else:
            return self._unit2id[UNK]

    def id2unit(self, id):
        return self._id2unit[id]

    def map(self, units):
        return [self.unit2id(x) for x in units]

    def unmap(self, ids):
        return [self.id2unit(x) for x in ids]

    def __str__(self):
        lang_str = "(%s)" % self.lang if self.lang else ""
        name = str(type(self)) + lang_str
        return "<%s: %s>" % (name, self._id2unit)

    def __len__(self):
        return len(self._id2unit)

    def __getitem__(self, key):
        if isinstance(key, str):
            return self.unit2id(key)
        elif isinstance(key, int) or isinstance(key, list):
            return self.id2unit(key)
        else:
            raise TypeError("Vocab key must be one of str, list, or int")

    def __contains__(self, key):
        return self.normalize_unit(key) in self._unit2id

    @property
    def size(self):
        return len(self)

SPACE_RE = re.compile(r'\s')

class Vocab(BaseVocab):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.lang_replaces_spaces = any([self.lang.startswith(x) for x in ['zh', 'ja', 'ko']])

    def build_vocab(self):
        paras = self.data
        counter = Counter()
        for para in paras:
            for unit in para:
                normalized = self.normalize_unit(unit[0])
                counter[normalized] += 1

        self._id2unit = [PAD, UNK] + list(sorted(list(counter.keys()), key=lambda k: counter[k], reverse=True))
        self._unit2id = {w:i for i, w in enumerate(self._id2unit)}

    def normalize_unit(self, unit):
        # Normalize minimal units used by the tokenizer
        return unit

    def normalize_token(self, token):
        token = SPACE_RE.sub(' ', token.lstrip())

        if self.lang_replaces_spaces:
            token = token.replace(' ', '')

        return token
