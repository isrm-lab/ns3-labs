set term png

set out 'propagation_model.png'
set xlabel 'Distance[m]'
set ylabel 'Throughput [Mbps]'
set title 'Various propagation models of ns-3 and loss with distance'

plot \
    'NAKAGAMI_3LOG' using 2:3 w lp t 'Nakagami+3LogDist', \
    'LOG_DIST_LOSS' using 2:3 w lp t 'LogDistLoss', \
    'THREE_LOG_DIST' using 2:3 w lp t '3LogDist', \
    'FRIIS' using 2:3 w lp t "Friis (5.15 GHz)"