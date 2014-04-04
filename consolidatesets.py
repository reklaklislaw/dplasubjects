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
            subjects.add(s)
    with open(out_file, 'wb') as f:
        pickle.dump(subjects, f)
        


if __name__ == '__main__':
    if len(sys.argv) == 3:
        glob_pattern = sys.argv[1]
        out_file = sys.argv[2]
        main(glob_pattern, out_file)
    else:
        raise ValueError('invalid arguments')
