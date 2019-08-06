#!/bin/bash
# setting:
in_time="13 45"

if [[ -z $1 ]]; then
cat << EOF
Usage:
./script2.sh folder CSV_file
EOF
echo "Exit"
exit 1
fi

function get_time()
{
    local next="$1"
    local in_h="$2"
    local in_min="$3"
    local min=$(( ($next + $in_min)%60 ))
    local h=0
    [[ $(( $1 + 45 )) -gt 59 ]] && h=$(( 1 ))
    [[ $min -lt 10 ]] && min="0$min"
    echo $(($in_h+$h))"_"$min
}

for min in $(seq 0 59); do
    #get_time ${min} $in_time
    kitty +kitten icat "$1/2019-08-06-$(get_time ${min} $in_time).jpg"
    echo 'put from left: `2.5 10 2.5 10`'
    read LINE
    LINE=($LINE)
    echo '"2019-08-06-11-'"${min}"'","'"${LINE[0]}"'","'"${LINE[1]}"'","'"${LINE[2]}"'","'"${LINE[3]}"'"'
    echo '"2019-08-06-11-'"${min}"'",'"${LINE[0]}"','"${LINE[1]}"','"${LINE[2]}"','"${LINE[3]}" >> "$2"
done
