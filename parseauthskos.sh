#!/bin/bash

in_file=$1

echo "getting auth urls"
urls=$(LC_ALL=C grep -o -P "http://id.loc.gov/authorities/subjects/sh[0-9]+" $in_file)
echo "writing urls"
echo $urls > authurls

echo "getting subjects"
IFS="\" \"" 
sd=$(LC_ALL=C grep -o -P "\".+?(?=\"@)\"" $in_file)
echo "writing subjects"
echo $sd > authsubjs
