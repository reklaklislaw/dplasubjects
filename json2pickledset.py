#Takes subject JSON created by findsubjects.c and writes a pickled set (eliminates duplicates) to a file.

import json, pickle
import sys


def unroll(data):
    subjects = set()
    if isinstance(data, dict):
        values = data.values()
    elif isinstance(data, list):
        values = data
    for d in values:
        if d is None:
            continue
        elif isinstance(d, str):
            subjects.add(d)
        elif isinstance(d, (list, dict)):
            for item in unroll(d):
                subjects.add(item)
        else:
            raise TypeError('encountered a list of ' + str(type(d)))
    return subjects


def main(in_file, out_file):

    with open(in_file, 'r') as f:
        j = json.load(f)

    subjects = set()
    for dct in j:
        s = dct['subject']
        if isinstance(s, (list, dict)):
            for item in unroll(s):
                subjects.add(item)
        elif isinstance(s, str):
            subjects.add(s)
        else:
            raise TypeError('encountered a ' + str(type(s)))

    with open(out_file, 'wb') as f:
        pickle.dump(subjects, f) 


if __name__ == '__main__':
    if len(sys.argv) == 3:
        in_file = sys.argv[1]
        out_file = sys.argv[2]
        main(in_file, out_file)
    else:
        raise ValueError('invalid arguments')
