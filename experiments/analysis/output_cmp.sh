#!/bin/bash

if [ -z "$1" ]; then
    echo "输入subject"
    exit 1
fi

subject="$1"

directory="/home/bjtucs/Desktop/Ming-18811237867/new/WinMutASE21Artifact-main/experiments/new-subjects/${subject}/WinMut/winmut-log-dir/run"


declare -a dir_arr=()

for item in "$directory"/*
do
    dir_arr+=($(basename "$item"))
done

echo -e ${dir_arr[@]}




for ((i = 0; i < ${#dir_arr[@]}; i++))
do
    {
        dir_name=${dir_arr[i]}
        python3.8 output_cmp.py "$directory" "$dir_name" > "output_cmp-${subject}-${dir_name}.result" 2>&1
        echo "${dir_name} $?"

        

    } &
done
wait


echo 'all done'