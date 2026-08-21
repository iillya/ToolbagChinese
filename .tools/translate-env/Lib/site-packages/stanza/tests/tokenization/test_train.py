"""
A basic training test - should be able to train & load a tokenizer on a small fake dataset
"""

import pytest

pytestmark = [pytest.mark.pipeline, pytest.mark.travis]


# TODO: build a dataset from a couple sentences, possibly using prepare_tokenizer_treebank, then run the following command:

# ['--label_file', '/home/john/stanza/data/tokenize/sq_combined-ud-train.toklabels', '--txt_file', '/home/john/stanza/data/tokenize/sq_combined.train.txt', '--lang', 'sq', '--max_seqlen', '300', '--mwt_json_file', '/home/john/stanza/data/tokenize/sq_combined-ud-dev-mwt.json', '--dev_txt_file', '/home/john/stanza/data/tokenize/sq_combined.dev.txt', '--dev_label_file', '/home/john/stanza/data/tokenize/sq_combined-ud-dev.toklabels', '--dev_conll_gold', '/home/john/stanza/data/tokenize/sq_combined.dev.gold.conllu', '--conll_file', '/tmp/tmpv675s8gs', '--shorthand', 'sq_combined', '--save_name', 'sq_combined_tokenizer.pt', '--save_dir', 'saved_models/tokenize']

