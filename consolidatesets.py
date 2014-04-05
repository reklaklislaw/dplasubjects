import pickle
import glob
import sys


def main(glob_pattern, out_file):
    subjects = set()
    pset_files = glob.glob(glob_pattern)
    for pset_f in pset_files:
        with open(pset_f, 'rb') as f:
            sset = pickle.load(f)
        for s in sset:
            if s:
                subjects.add(s)
                for i in get_substrings(s):
                    subjects.add(i)
    with open(out_file, 'wb') as f:
        pickle.dump(subjects, f)
         
        
def get_substrings(string):
    sub = set()
    patterns = (';', ',', '--', ' ')
    for p in patterns:
        s = string.split(p)
        for i in s:
            item = i
            for r in patterns:
                item = item.rstrip(r).lstrip(r)
            if item:
                sub.add(item)
                for _p in patterns:
                    _s = item.split(_p)
                    for _i in _s:
                        _item = _i
                        for r in patterns:
                            _item = _item.rstrip(r).lstrip(r)
                        if _item:
                            sub.add(_item)
    if len(s) > 1:
        return sub
    else:
        return []


if __name__ == '__main__':
    if len(sys.argv) == 3:
        glob_pattern = sys.argv[1]
        out_file = sys.argv[2]
        main(glob_pattern, out_file)
    else:
        raise ValueError('invalid arguments')
