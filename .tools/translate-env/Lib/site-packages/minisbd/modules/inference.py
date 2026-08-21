import onnxruntime
import numpy as np
import re
from .data import SortedDataset

ID = 'id'
TEXT = 'text'
START_CHAR = 'start_char'
END_CHAR = 'end_char'
MISC = 'misc'


def get_sentences(document, raw_text):
    sentences = []
    for tokens in document:
        begin_idx, end_idx = tokens[0]['start_char'], tokens[-1]['end_char']
        sentences.append(raw_text[begin_idx: end_idx])
    return sentences

def process_sentence(sentence):
    sent = []
    i = 0
    for tok, p, position_info in sentence:
        expansion = None
        if expansion is not None:
            sent.append({ID: (i+1, i+len(expansion)), TEXT: tok})
            if position_info is not None:
                sent[-1][START_CHAR] = position_info[0]
                sent[-1][END_CHAR] = position_info[1]
            for etok in expansion:
                sent.append({ID: (i+1, ), TEXT: etok})
                i += 1
        else:
            if len(tok) <= 0:
                continue
            sent.append({ID: (i+1, ), TEXT: tok})
            if position_info is not None:
                sent[-1][START_CHAR] = position_info[0]
                sent[-1][END_CHAR] = position_info[1]
            if p == 3 or p == 4:# MARK
                sent[-1][MISC] = 'MWT=Yes'
            i += 1
    return sent


def decode_predictions(vocab, orig_text, all_raw, all_preds, no_ssplit, skip_newline, use_la_ittb_shorthand):
    """
    Decode the predictions into a document of words

    Once everything is fed through the tokenizer model, it's time to decode the predictions
    into actual tokens and sentences that the rest of the pipeline uses
    """
    offset = 0
    oov_count = 0
    doc = []

    text = SPACE_RE.sub(' ', orig_text) if orig_text is not None else None
    char_offset = 0

    if vocab is not None:
        UNK_ID = vocab.unit2id('<UNK>')

    for raw, pred in zip(all_raw, all_preds):
        current_tok = ''
        current_sent = []

        for t, p in zip(raw, pred):
            if t == '<PAD>':
                break
            # hack la_ittb
            if use_la_ittb_shorthand and t in (":", ";"):
                p = 2
            offset += 1
            if vocab is not None and vocab.unit2id(t) == UNK_ID:
                oov_count += 1

            current_tok += t
            if p >= 1:
                if vocab is not None:
                    tok = vocab.normalize_token(current_tok)
                else:
                    tok = current_tok
                assert '\t' not in tok, tok
                if len(tok) <= 0:
                    current_tok = ''
                    continue
                if orig_text is not None:
                    st = -1
                    tok_len = 0
                    for part in SPACE_SPLIT_RE.split(current_tok):
                        if len(part) == 0: continue
                        if skip_newline:
                            part_pattern = re.compile(r'\s*'.join(re.escape(c) for c in part))
                            match = part_pattern.search(text, char_offset)
                            st0 = match.start(0) - char_offset
                            partlen = match.end(0) - match.start(0)
                            lstripped = match.group(0).lstrip()
                        else:
                            try:
                                st0 = text.index(part, char_offset) - char_offset
                            except ValueError as e:
                                sub_start = max(0, char_offset - 20)
                                sub_end = min(len(text), char_offset + 20)
                                sub = text[sub_start:sub_end]
                                raise ValueError("Could not find |%s| starting from char_offset %d.  Surrounding text: |%s|" % (part, char_offset, sub)) from e
                            partlen = len(part)
                            lstripped = part.lstrip()
                        if st < 0:
                            st = char_offset + st0 + (partlen - len(lstripped))
                        char_offset += st0 + partlen
                    position_info = (st, char_offset)
                else:
                    position_info = None
                current_sent.append((tok, p, position_info))
                current_tok = ''
                if (p == 2 or p == 4) and not no_ssplit:
                    doc.append(process_sentence(current_sent))
                    current_sent = []

        if len(current_tok) > 0:
            raise ValueError("Finished processing tokens, but there is still text left!")
        if len(current_sent):
            doc.append(process_sentence(current_sent))

    return oov_count, offset, doc

MAX_SEQ_LENGTH_DEFAULT = 1000

def output_predictions(config, session, data_generator, vocab, orig_text=None, use_regex_tokens=True):
    batch_size = config['batch_size']
    max_seqlen = max(MAX_SEQ_LENGTH_DEFAULT, config.get('max_seqlen', MAX_SEQ_LENGTH_DEFAULT))
    no_ssplit=config.get('no_ssplit', False)

    all_preds, all_raw = predict(config, session, data_generator, batch_size, max_seqlen, use_regex_tokens)

    use_la_ittb_shorthand = False # config['shorthand'] == 'la_ittb'
    skip_newline = config['skip_newline']
    oov_count, offset, doc = decode_predictions(vocab, orig_text, all_raw, all_preds, no_ssplit, skip_newline, use_la_ittb_shorthand)

    return oov_count, offset, all_preds, doc

def get_onnx_batches(dataset, batch_size):
    for i in range(0, len(dataset), batch_size):
        batch_samples = [dataset[j] for j in range(i, min(i + batch_size, len(dataset)))]
        
        batch = dataset.collate(batch_samples)
        
        yield batch


def find_spans(raw):
    """
    Return spans of text which don't contain <PAD> and are split by <PAD>
    """
    pads = [idx for idx, char in enumerate(raw) if char == '<PAD>']
    if len(pads) == 0:
        spans = [(0, len(raw))]
    else:
        prev = 0
        spans = []
        for pad in pads:
            if pad != prev:
                spans.append( (prev, pad) )
            prev = pad + 1
        if prev < len(raw):
            spans.append( (prev, len(raw)) )
    return spans

# https://stackoverflow.com/questions/201323/how-to-validate-an-email-address-using-a-regular-expression
EMAIL_RAW_RE = r"""(?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(?:2(?:5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(?:2(?:5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\])"""

# https://stackoverflow.com/questions/3809401/what-is-a-good-regular-expression-to-match-a-url
# modification: disallow " as opposed to all ^\s
URL_RAW_RE = r"""(?:https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s"]{2,}|www\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\.[^\s"]{2,}|https?:\/\/(?:www\.|(?!www))[a-zA-Z0-9]+\.[^\s"]{2,}|www\.[a-zA-Z0-9]+\.[^\s"]{2,})|[a-zA-Z0-9]+\.(?:gov|org|edu|net|com|co)(?:\.[^\s"]{2,})"""

MASK_RE = re.compile(f"(?:{EMAIL_RAW_RE}|{URL_RAW_RE})")
SPACE_RE = re.compile(r'\s')
SPACE_SPLIT_RE = re.compile(r'( *[^ ]+)')

def update_pred_regex(raw, pred):
    """
    Update the results of a tokenization batch by checking the raw text against a couple regular expressions

    Currently, emails and urls are handled
    TODO: this might work better as a constraint on the inference

    for efficiency pred is modified in place
    """
    spans = find_spans(raw)

    for span_begin, span_end in spans:
        text = "".join(raw[span_begin:span_end])
        for match in MASK_RE.finditer(text):
            match_begin, match_end = match.span()
            # first, update all characters touched by the regex to not split
            # with the exception of the last character...
            for char in range(match_begin+span_begin, match_end+span_begin-1):
                pred[char] = 0
            # if the last character is not currently a split, make it a word split
            if pred[match_end+span_begin-1] == 0:
                pred[match_end+span_begin-1] = 1

    return pred

def predict(config, session, data_generator, batch_size, max_seqlen, use_regex_tokens):
    """
    The guts of the prediction method

    Calls predict() over and over until we have predictions for all of the text
    """
    all_preds = []
    all_raw = []

    sorted_data = SortedDataset(data_generator)

    for batch_idx, batch in enumerate(get_onnx_batches(sorted_data, batch_size)):
        num_sentences = len(batch[3])
        N = len(batch[3][0])
        for paragraph in batch[3]:
            all_raw.append(list(paragraph))

        if N <= max_seqlen:
            pred = np.argmax(onnx_predict(session, batch), axis=2)
        else:
            # TODO: we could shortcircuit some processing of
            # long strings of PAD by tracking which rows are finished
            idx = [0] * num_sentences
            adv = [0] * num_sentences
            para_lengths = [x.index('<PAD>') for x in batch[3]]
            pred = [[] for _ in range(num_sentences)]
            while True:
                ens = [min(N - idx1, max_seqlen) for idx1, N in zip(idx, para_lengths)]
                en = max(ens)
                batch1 = batch[0][:, :en], batch[1][:, :en], batch[2][:, :en], [x[:en] for x in batch[3]]
                pred1 = np.argmax(onnx_predict(session, batch1), axis=2)

                for j in range(num_sentences):
                    sentbreaks = np.where((pred1[j] == 2) + (pred1[j] == 4))[0]
                    if len(sentbreaks) <= 0 or idx[j] >= para_lengths[j] - max_seqlen:
                        advance = ens[j]
                    else:
                        advance = np.max(sentbreaks) + 1

                    pred[j] += [pred1[j, :advance]]
                    idx[j] += advance
                    adv[j] = advance

                if all([idx1 >= N for idx1, N in zip(idx, para_lengths)]):
                    break
                # once we've made predictions on a certain number of characters for each paragraph (recorded in `adv`),
                # we skip the first `adv` characters to make the updated batch
                batch = data_generator.advance_old_batch(adv, batch)

            pred = [np.concatenate(p, 0) for p in pred]

        for par_idx in range(num_sentences):
            offset = batch_idx * batch_size + par_idx

            raw = all_raw[offset]
            par_len = raw.index('<PAD>')
            raw = raw[:par_len]
            all_raw[offset] = raw
            if pred[par_idx][par_len-1] < 2:
                pred[par_idx][par_len-1] = 2
            elif pred[par_idx][par_len-1] > 2:
                pred[par_idx][par_len-1] = 4
            if use_regex_tokens:
                all_preds.append(update_pred_regex(raw, pred[par_idx][:par_len]))
            else:
                all_preds.append(pred[par_idx][:par_len])

    all_preds = sorted_data.unsort(all_preds)
    all_raw = sorted_data.unsort(all_raw)

    return all_preds, all_raw

def onnx_predict(session, inputs):
    units, _, features, _ = inputs
    ort_inputs = {'units': units, 'features': features}
    ort_outs = session.run(None, ort_inputs)

    return ort_outs[0]