import json

urlfile = 'authurls'
subjfile = 'authsubjs'

urls = open(urlfile, 'r')
urldata = urls.read()
urllines = urldata.split(' ')

subjs = open(urlfile, 'r')
subjlines = subjs.readlines()

dct = {}
for num, subj in enumerate(subjlines):
    dct[subj] = urllines[num]

with open('authids.json', 'wb') as f:
    json.dump(dct, f)
