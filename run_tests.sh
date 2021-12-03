#! /bin/sh


string="/h/zyang11/comp40/hw06/actual_um/in.um"
string2="/h/zyang11/comp40/hw06/actual_um/in2.um"
string3="/h/zyang11/comp40/hw06/actual_um/cat.um"
string4="/h/zyang11/comp40/hw06/actual_um/advent.umz"

for file in /h/zyang11/comp40/hw06/actual_um/*.um
do

    echo $file

    if [[ "$file" == "$string4" ]]
    then 
        ./um "$file" < advent.txt > our_out.txt
        um "$file" < advent.txt > their_out.txt
    else
        if [[ "$file" == "$string2" ]]
        then
            ./um "$file" < in2.0 > our_out.txt
            um "$file" < in2.0 > their_out.txt
        else
            if [ "$file" == "$string" ] || [ "$file" == "$string2" ]
            then
                ./um "$file" < in.0 > our_out.txt
                um "$file" < in.0 > their_out.txt
            else
                ./um "$file" > our_out.txt
                um "$file" > their_out.txt
            fi
        fi

    fi

    # echo $?

    result=$(diff -y -W 72 our_out.txt their_out.txt)

    if [ $? -eq 0 ]
    then
            echo "              files are the same"
    else
            printf "\n"
            echo "^^files are different"
            printf "\n"
    fi

    rm our_out.txt
    rm their_out.txt
done
