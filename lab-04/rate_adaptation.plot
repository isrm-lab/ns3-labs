
set term png

set out 'rate_adaptation.png'
set xlabel 'Distance[m]'
set ylabel 'Throughput [Mbps]'
set title '802.11g Rate Adaptation vs Fixed Rate'

plot \
    'aarf' using 2:3 w lp t 'AARF adaptation', \
    '6m' using 2:3 w lp t 'Fixed = 6Mbps', \
    '24m' using 2:3 w lp t 'Fixed = 24Mbps', \
    '54m' using 2:3 w lp t "Fixed = 54Mbps"


    