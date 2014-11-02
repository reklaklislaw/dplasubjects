#!/usr/bin/python3.4
import sys, os
import glob
import re
import json
import pprint

def consolidate(in_files, outfile):
    fields = {}
    for file in in_files:
        with open(file, 'r') as f:
            while True:
                sys.stderr.write('search of '+ file + ' ' +
                             str(round((f.tell()/os.path.getsize(file)) * 100, 0)) + 
                             '% complete\r')
                line = f.readline()
                line = line.lstrip('[').rstrip('\n').rstrip(',').rstrip(']') 
                if not line:
                    break
                j = json.loads(line)
                identifier = j.get('identifier')
                field_data = j.get('fields')
                if not identifier or not field_data:
                    sys.stderr.write('warning: missing identifier/field data')
                fset = unroll(field_data)
                for field in fset:
                    while field.startswith(' '):
                        field = field.lstrip(' ')
                    while field.endswith(' '):
                        field = field.rstrip(' ')
                    if field not in fields:
                        fields[field] = {}
                        fields[field]['ids'] = []
                    fields[field]['ids'].append(identifier)
        sys.stderr.write('\n')
    sys.stderr.write('writing to file ' + outfile + '...\n')
    with open(outfile, 'wb') as f:
        f.write(b'[\n')
        for field, data in fields.items():
            string = '{{"field": "{0}", "identifiers": {1}}},\n'.format(field, json.dumps(data['ids']))
            f.write(bytes(string, encoding='utf-8'))
        f.seek(-2, 2)
        f.write(b']')
                            
def unroll(data):
    fset = set()
    if isinstance(data, dict):
        values = data.values()
    elif isinstance(data, list):
        values = data
    for d in values:
        if d is None:
            continue
        elif isinstance(d, str):
            fset.add(d)
        elif isinstance(d, (list, dict)):
            for item in unroll(d):
                fset.add(item)
        else:
            raise TypeError('encountered a list of ' + str(type(d)))
    return fset

if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise Exception('Usage: in_basename outfile')
    in_basename = sys.argv[1]
    outfile = sys.argv[2]
    in_files = [i.group(0) for i in \
                    [re.match('.+[0-9]+$', f) 
                     for f in glob.glob(in_basename+'*')] if i]
    if not in_files:
        raise IOError('failed to find any files with that basename')
    consolidate(in_files, outfile)
