set term png

set out 'capacitate_dss.png'
set xlabel 'payload[bytes]'
set ylabel 'Throughput [Mbps]'
set title 'Medium capacity DSSS 802.11b'

plot \
    'dataset_DsssRate1Mbps.txt' using 1:2 w lp t 'dsss1mbps', \
    'dataset_DsssRate2Mbps.txt' using 1:2 w lp t 'dsss2mbps', \
    'dataset_DsssRate5_5Mbps.txt' using 1:2 w lp t 'dsss5_5mbps', \
    'dataset_DsssRate11Mbps.txt' using 1:2 w lp t 'dsss11mbps', \

