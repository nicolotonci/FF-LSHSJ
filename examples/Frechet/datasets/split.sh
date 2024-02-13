#!/bin/bash
echo "Splitting file $1 in $2 partitions"

# write results to tmp if you plan to distribute the chunks over the computing nodes, in order to save writing on disks intermediate results
split -d -n l/$2 --suffix-length=2 $1 ./$1-


# Uncomment the following lines if you want to distribute chunk over the computing nodes.
# Make sure the scp command is well cformatted for your cluster configuration.

#for i in $(seq 1 $(($2 - 1)))
#do
#    echo "Mooving partition #$1"
#  scp /tmp/$1-$(printf "%02d" $i) compute$i:/tmp/
#   rm /tmp/$1-$(printf "%02d" $i)
#done

echo "Finished"
