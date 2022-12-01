#!/bin/bash

while getopts "m:n:l:" argv
do
    case $argv in
        m)
            n_hosts=$OPTARG
            ;;
        n)
            n_players=$OPTARG
            ;;
        l)
            lucky_number=$OPTARG
            ;;
    esac
done

index=0
rm fifo_*.tmp -f
while [ ${index} -le ${n_hosts} ]; do
    mkfifo "fifo_${index}.tmp"
    eval "exec $((${index} + 3))<> fifo_${index}.tmp"
    index=$((${index} + 1))
done

for ((i=0;i<${n_hosts};i++))
do
    ./host -m $(($i + 1)) -d 0 -l ${lucky_number} &
done

num=0
score=(0 0 0 0 0 0 0 0 0 0 0 0)
for ((i=0;i<2**${n_players};i++))
do
    count=0
    for ((j=0;j<${n_players};j++))
    do
        if [ $(($i & 2**$j)) -gt 0 ]; then
            on[${count}]=$((${j} + 1))
            count=$((${count} + 1))
        fi
    done

    if [ ${count} -eq 8 ]; then
        for ((j=0;j<8;j++))
        do
            ids[$j]=${on[$j]}
        done

        if [ ${num} -lt ${n_hosts} ]; then
            echo ${ids[@]} > "fifo_$((${num} + 1)).tmp"
            num=$((${num} + 1))
        else
            read valid_i < "fifo_0.tmp"
            for ((j=0;j<8;j++))
            do
                read -a result < "fifo_0.tmp"
                score[$((${result[0]} - 1))]=$((${score[$((${result[0]} - 1))]} + ${result[1]}))
            done
            echo ${ids[@]} > "fifo_${valid_i}.tmp"
        fi
    fi
done

for ((i=0;i<${num};i++))
do
    read valid_i < "fifo_0.tmp"
    for ((j=0;j<8;j++))
    do
        read -a result < "fifo_0.tmp"
        score[$((${result[0]} - 1))]=$((${score[$((${result[0]} - 1))]} + ${result[1]}))
    done
done

for ((i=0;i<${n_hosts};i++))
do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" > "fifo_$(($i + 1)).tmp"
done

pre_val=-1
pre_rank=-1
for ((i=0;i<${n_players};i++))
do
    max_i=-1
    max_val=-1
    for ((j=0;j<${n_players};j++))
    do
        if [ ${score[$j]} -gt ${max_val} ]; then
            max_i=$j
            max_val=${score[$j]}
        fi
    done

    if [ ${max_val} -eq ${pre_val} ]; then
        rankings[${max_i}]=${pre_rank}
    else
        rankings[${max_i}]=$(($i + 1))
        pre_rank=$(($i + 1))
        pre_val=${max_val}
    fi
    score[${max_i}]=-1
done

for ((i=0;i<${n_players};i++))
do
    echo "$(($i + 1)) ${rankings[$i]}"
done

rm fifo_*.tmp -f
wait