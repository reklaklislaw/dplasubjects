import json

urlfile = 'authurls'
subjfile = 'authsubjs'

urls = open(urlfile, 'r')
ul = urls.readlines()
urllines = []
for line in ul:
    if line not in urllines:
        urllines.append(line)

subjs = open(subjfile, 'r')
subjlines = subjs.readlines()

dct = {}
for num, subj in enumerate(subjlines):
    url = urllines[num]
    sub = subj.lstrip(' ').rstrip(' \n')
    dct[sub] = url.rstrip('\n')
    
with open('authids.json', 'w') as f:
    json.dump(dct, f)
