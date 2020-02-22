set term png

set out 'capacitate_erp.png'
set xlabel 'payload[bytes]'
set ylabel 'Throughput [Mbps]'
set title 'Medium capacity ERP OFDM 802.11g'

plot \
    'dataset_ErpOfdmRate6Mbps.txt' using 1:2 w lp t 'erp6mbps', \
    'dataset_ErpOfdmRate9Mbps.txt' using 1:2 w lp t 'erp6mbps', \
    'dataset_ErpOfdmRate12Mbps.txt' using 1:2 w lp t 'erp6mbps', \
    'dataset_ErpOfdmRate18Mbps.txt' using 1:2 w lp t 'erp6mbps', \
    'dataset_ErpOfdmRate24Mbps.txt' using 1:2 w lp t 'erp6mbps', \
    'dataset_ErpOfdmRate36Mbps.txt' using 1:2 w lp t 'erp6mbps', \
    'dataset_ErpOfdmRate54Mbps.txt' using 1:2 w lp t 'erp6mbps'
