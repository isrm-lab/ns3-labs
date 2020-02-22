#!/bin/bash
 
out=jain.out  

echo "cw sz jain eps" > $out 
for sz in 2 4 6 7 9 10 12 15 17 20; do 
    for cw in 7 31 511; do 
        ./waf --run "scratch/lab8 --minCw=$cw --maxCw=$cw --nn=$sz --pcap=false" > scriptres.txt
		cat scriptres.txt
		stats=`cat scriptres.txt | tail -n 1`
        echo  "$sz $stats" >> "$out_$cw.txt"
    done
    echo "" >> $out
    echo "" >> $out
done

rm scriptres.txt