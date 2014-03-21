#!/bin/bash
# Takes a dpla gzip as input, greps for subject entries, formats as json
# and writes out a file.

infile=$1
outfile=$2

echo "getting total subject field count..."
total=$(LC_ALL=C grep -o -P "\"subject\":" $infile | wc -l)
echo "total number of subject fields: $total" 

echo "getting empty subject field count..."
empty=$(LC_ALL=C grep -o -P "\"subject\":(\[\]|null|\[null\])" $infile | wc -l)
echo "total empty: $empty"

echo "getting subject lists..."
lists=$(LC_ALL=C grep -o -P \
    "\"subject\":(\[\".+?\"(?=(\]|,))\]|\[\{.+?\}(?=(\]|,|\}))\]|\{.+?\}(?=(\}|,|))\})" $infile)
if [ -z "$lists"  ]
then
    list_count=0
else
    list_count=$(echo "{$lists}" | wc -l)
fi
echo "total lists: $list_count"

echo "getting subject strings..."
strings=$(LC_ALL=C grep -o -P "\"subject\":\"[^\"]*\"[^,^\}]*" $infile)
if [ -z "$strings"  ]
then
    string_count=0
else
    string_count=$(echo "{$strings}" | wc -l)
fi

echo "total strings: $string_count"

matched=$(($list_count+$string_count+$empty))

if [ $matched == $total ]
then 
    echo "matched all subjects"
    echo "{$lists}" > $outfile
    echo "{$strings}" >> $outfile
    #echo "formatting..."
    #sed -i 's/"subject":/{"subject":/g' $outfile
    #sed -i 's/.$/&},/g' $outfile
    #sed -i '1i[' $outfile
    #sed -i '$s/.$/]/g' $outfile
else
    echo "FAILED TO MATCH ALL SUBJECTS... matched: $matched   total: $total" 
fi
