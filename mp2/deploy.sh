#!/bin/bash
val=25
cwnd=4
buffer=512
while true
do 
    echo version$val
    sed -i "s/$val/$((val+1))/g" version.txt
    sed -i "s/CWND $cwnd/CWND $((cwnd+1))/g" /cs438/mp2//src/sender_main.cpp
    sed -i "s/BUFFER_SIZE $buffer/BUFFER_SIZE $((buffer*2))/g" /cs438/mp2//src/sender_main.cpp

    make
    git add *
    git commit -m '*'
    git push
    val=$((val+1))
    if [ $cwnd = 8 ]
    then
        cwnd=4
        echo resetcwnd$cwnd

    else
        cwnd=$((cwnd+1))
        echo addcwnd$cwnd
    fi

    if [ $buffer = 4096 ]
    then
        buffer=512
        echo resetbuffer$buffer

    else
        buffer=$((buffer*2))
        echo addbuffer$buffer
    fi
    sleep 10
done