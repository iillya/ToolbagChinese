import argparse
import sys

import stanza.utils.datasets.common as common

def remove_column(sentences, column):
    new_sentences = []
    for sentence in sentences:
        new_lines = []
        for line in sentence:
            if line.startswith("#"):
                new_lines.append(line)
            else:
                pieces = line.split("\t")
                pieces[column] = "_"
                new_lines.append("\t".join(pieces))
        new_sentences.append(new_lines)
    return new_sentences

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--infile', help='Which file to manipulate')
    parser.add_argument('--outfile', help='Where to write the result')
    args = parser.parse_args()

    sentences = common.read_sentences_from_conllu(args.infile)
    sentences = [common.maybe_add_fake_dependencies(sentence) for sentence in sentences]
    sentences = remove_column(sentences, 4)
    sentences = remove_column(sentences, 5)
    common.write_sentences_to_conllu(args.outfile, sentences)
    
if __name__ == "__main__":
    main()
