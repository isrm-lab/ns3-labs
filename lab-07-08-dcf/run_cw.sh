#!/bin/bash
 
out=cwsim.out  

echo "#sz cw sent_agt recv_agt col_cbr sent_mac recv_mac avgTput" > $out 
for sz in 4 6 7 20 40; do 
    for cw in 3 7 15 31 63 127 255 511 1023 2047 4095; do 
        ./waf --run "scratch/lab11 --ns=$sz --nd=$sz --minCw=$cw --maxCw=$cw --pcap=false" > scriptres.txt
		cat scriptres.txt
		stats=`cat scriptres.txt | tail -n 1`
        echo  "$sz $cw $stats" >> $out
    done
    echo "" >> $out
    echo "" >> $out
	./waf --run "scratch/lab11 --ns=$sz --nd=$sz --pcap=false" > scriptres.txt
	stats=`cat scriptres.txt | tail -n 1`
	echo  "$sz 802.11 $stats" >> $out
    echo "" >> $out
    echo "" >> $out
done

rm scriptres.txt