def create_dictionary(lexicon):
    """
    This function is to create a new dictionary used for improving tokenization model for multi-syllable words languages
    such as vi, zh or th. This function takes the lexicon as input and output a dictionary that contains three set:
    words, prefixes and suffixes where prefixes set should contains all the prefixes in the lexicon and similar for suffixes.
    The point of having prefixes/suffixes sets in the  dictionary is just to make it easier to check during data preparation.

    :param shorthand - language and dataset, eg: vi_vlsp, zh_gsdsimp
    :param lexicon - set of words used to create dictionary
    :return a dictionary object that contains words and their prefixes and suffixes.
    """
    if lexicon is None:
        return None
    lexicon = set(lexicon)
    dictionary = {"words":set(), "prefixes":set(), "suffixes":set()}
    
    def add_word(word):
        if word not in dictionary["words"]:
            dictionary["words"].add(word)
            prefix = ""
            suffix = ""
            for i in range(0,len(word)-1):
                prefix = prefix + word[i]
                suffix = word[len(word) - i - 1] + suffix
                dictionary["prefixes"].add(prefix)
                dictionary["suffixes"].add(suffix)

    for word in lexicon:
        if len(word)>1:
            add_word(word)

    return dictionary