#!/usr/bin/python3.4
import sys
import json
import re
from copy import copy

def tokenize(in_file):
    newlines = []
    with open(in_file, 'r') as f:
        while True:
            line = f.readline()
            if not line:
                break
            line = line.lstrip('[').rstrip('\n').rstrip(',').rstrip(']') 
            j = json.loads(line)
            tokens = []
            methods = [re.compile('[(\[{].+?[)\]}]'), '--', ';', ',', ' ']
            obj = j.get('object')
            subj = j.get('subject')
            tokens.extend(apply_methods(methods, [obj,]))
            tokens = clean_and_prune(tokens)
            newlines.append( {'object': obj, 'subject': subj, 'tokens': tokens} )
    with open(in_file, 'w') as f:
        f.write('[')
        for num, line in enumerate(newlines):
            if num < len(newlines) - 1:
                string = json.dumps(line) + ',\n'
            else:
                string = json.dumps(line)
            f.write(string)
        f.write(']')
    
def clean_and_prune(tokens):
    cremove = (' ', '(', ')', '[', ']', '{', '}', '-', 
               '.', '<', '>', '\\', '/', '^', '*')
    wremove = ('', 'and', 'in', 'to', 'too', 'for', 'the', 
               'they', 'who', 'what', 'where', 'were', 'why')
    t = []    
    for tok in tokens:
        n = 0
        if tok in wremove:
            continue
        while n < len(cremove):
            c = cremove[n]
            while tok.startswith(c):
                tok = tok.lstrip(c)
                n = -1
            while tok.endswith(c):
                tok = tok.rstrip(c)
                n = -1
            n += 1
        t.append(tok)
    return t
                
def apply_methods(methods, strings):
    mcpy = copy(methods)
    tokens = strings
    for mnum, method in enumerate(methods):
        if isinstance(method, str):
            for snum, string in enumerate(strings):
                if method in string:
                    toks = string.split(method)
                    if len(toks) > 1:
                        if method != ' ':
                            try:
                                spindex = mcpy.index(' ')
                                del mcpy[spindex]
                            except:
                                pass
                        del tokens[snum]
                        tokens.extend([t for t in toks if t])
                        return apply_methods(mcpy, tokens)
        else:
            for snum, string in enumerate(strings):
                matches = re.findall(method, string)
                if matches:
                    for match in matches:
                        string = re.sub(re.escape(match), '', string)
                    del tokens[snum]
                    tokens.append(string)
                    tmp = copy(tokens)
                    withoutspaces = apply_methods(mcpy, tokens)
                    try:
                        spindex = mcpy.index(' ')
                        del mcpy[spindex]
                    except:
                        pass
                    tokens = apply_methods(mcpy, tmp)
                    for t in withoutspaces:
                        if t not in tokens:
                            tokens.append(t)                    
                    for match in matches:
                        tokens.append(match)
                    return tokens
    return tokens        

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise Exception("Usage: in_file")
    in_file = sys.argv[1]
    tokenize(in_file)
