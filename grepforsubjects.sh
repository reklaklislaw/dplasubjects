#!/bin/bash
# Takes a dpla gzip as input, greps for subject entries, formats as json
# and writes out a file.

infile=$1
outfile=$2

echo "getting total subject field count..."
total=$(LC_ALL=C zgrep -o -P "\"subject\":" $infile | wc -l)
echo "total number of subject fields: $total" 

echo "getting empty subject field count..."
empty=$(LC_ALL=C zgrep -o -P "\"subject\":(\[\]|null|\[null,\]*)" $infile | wc -l)
echo "total empty: $empty"

echo "getting subject lists..."

l0="\"subject\":("
l1="\[\".+?\"(?=(\]|,))\]|"
l2="\[\{.+?(?=(\}\]))\}\]|"
l3="\[\{.+?\}(?=([\}]{3}))\})"

listregex=$l0$l1$l2$l3

lists=$(LC_ALL=C zgrep -o -P $listregex $infile)
if [ -z "$lists"  ]
then
    list_count=0
else
    list_count=$(echo "{$lists}" | wc -l)
fi
echo "total lists: $list_count"


echo "getting subject dicts..."

d0="\"subject\":("
d1="(\{.+?((?=(\]\}))(\]\}\}|\]\})|(?=\}\})\}))|"
d2="\{\".+?(?=\},)\})"

dictregex=$d0$d1$d2

dicts=$(LC_ALL=C zgrep -o -P $dictregex $infile)
if [ -z "$dicts"  ]
then
    dict_count=0
else
    dict_count=$(echo "{$dicts}" | wc -l)
fi
echo "total dicts: $dict_count"


echo "getting subject strings..."
strings=$(LC_ALL=C zgrep -o -P "\"subject\":\"[^\"]*\"[^,^\}]*" $infile)
if [ -z "$strings"  ]
then
    string_count=0
else
    string_count=$(echo "{$strings}" | wc -l)
fi

echo "total strings: $string_count"


matched=$(($list_count+$dict_count+$string_count+$empty))

if [ $matched == $total ]
then 
    echo "matched all subjects"
    if [ -n "$lists" ]
    then
	echo "$lists" > $outfile
    fi
    if [ -n "$dicts" ]
    then
	echo "$dicts" > $outfile
    fi
    if [ -n "$strings" ]
    then
	echo "$strings" >> $outfile
    fi
    echo "formatting..."
    sed -i 's/"subject":/{"subject":/g' $outfile
    sed -i 's/.$/&},/g' $outfile
    sed -i '1i[' $outfile
    sed -i '$s/.$/]/g' $outfile
    echo 'verifying json'
    jsonlint $outfile
else
    echo "FAILED TO MATCH ALL SUBJECTS... matched: $matched   total: $total" 
fi
