#!/bin/bash
# Takes a dpla gzip as input, greps for subject entries, formats as json
# and writes out a file.

infile=$1
outfile=$2

#lists
r1="(\"subject\":\[.+?(?=\]\},)[\]]|"

#strings
r2="\"subject\":\"[^\"]*\"[^,^\}]*)"

regex=$r1$r2

LC_ALL=C zegrep -o "$regex" $infile > $outfile

sed -i 's/"subject":/{"subject":/g' $outfile
sed -i 's/.$/&},/g' $outfile
sed -i '1i[' $outfile
sed -i '$s/.$/]/g' $outfile




