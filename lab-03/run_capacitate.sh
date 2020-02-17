#!/bin/bash

out=capacitate_mediu.out
declare -a phyRates=("DsssRate1Mbps"     \
"DsssRate2Mbps"     \
"DsssRate5_5Mbps"   \
"DsssRate11Mbps"    \
"ErpOfdmRate6Mbps"  \
"ErpOfdmRate9Mbps"  \
"ErpOfdmRate12Mbps" \
"ErpOfdmRate18Mbps" \
"ErpOfdmRate24Mbps" \
"ErpOfdmRate36Mbps" \
"ErpOfdmRate48Mbps" \
"ErpOfdmRate54Mbps" \
)

for mcs in "${phyRates[@]}"; do
	for payloadSz in 20 50 100 500 1000 1500; do
		# echo $payloadSz dataset_$mcs
		./waf --run "lab3 --simulationTime=1 --payloadSize=$payloadSz --numberOfNodes=2 --dataRate=54Mbps --phyRate=$mcs" | grep Throughput | cut -d':' -f2 | cut -d' ' -f2 > thrPut.txt
		thrPut=`cat thrPut.txt`
		echo $payloadSz $thrPut >> dataset_$mcs.txt
		echo -ne "$payloadSz $thrPut | "
	done
	echo -ne "\n"
done