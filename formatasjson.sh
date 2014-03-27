#!/bin/bash
# takes a file produced by grepforsubjects and 
# formats its contents as json

infile=$1

echo "formatting..."
sed -i 's/"subject":/{"subject":/g' $infile
sed -i 's/.$/&},/' $infile
sed -i '1i[' $infile
sed -i '$s/.$/]/g' $infile
echo 'verifying json'
jsonlint -v $outfile
