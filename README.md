metadata2ontology
============

A tool for mapping fields from metadata to ontologies.

(in development)

Current limitations
- metadata must be JSON, with each item line-separated.
- ontology must be SKOS N-triples, which each N-triple line-separated

TODO
- GUI

<h3>Usage</h3>

build: 
```
./build_all.sh
```

The current workflow is to first pull all the fields you would like to map from
the metadata with 'findfield', and to consolidate the output (a set of .json 
files) and write to a single file with the consolidatefoundfields.py.

Example, extracting all "subject" fields from the Digital Public Library of 
America's metadata dump:
```
./findfield dpla.gz subject id dplas
./consolidatefoundfields.py dplas dpla.json
```

Next we parse the ontology into a simplified format as a .json file, and derive
a set of tokens from each item in order to facilitate partial matches to the 
metadata fields if no exact matches are found.

Example, extracting all N-triples from the Library of Congress's subject 
header authority SKOS N-triple dump with "prefLabel" as the predicate:
```
./skosntriple2simplejson subjects-skos-20140306.nt prefLabel locsh.json
./tokenizeontology.py locsh.json
```

Finally we perform the mapping:

```
./searchontology dplas.json locsh.json dpla2locsh
```
