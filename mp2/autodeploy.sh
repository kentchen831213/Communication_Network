#!/bin/bash
val=171
while true
do 
    echo version$val
    sed -i "s/$val/$((val+1))/g" version.txt
    git add *
    git commit -m '*'
    git push
    val=$((val+1))
    sleep 600
done