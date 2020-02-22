set term png

set out 'mac-phy-pdr.png'
set xlabel 'Distance[m]'
set ylabel 'PDR(packed delivery ratio)'  
set title 'PDR at MAC and PHY levels'
set yrange [0:1.5]
set xrange [*:220]

#probabilitatea de reușită în maximum r încercări
# unque_recv/unique_sent 
f(x)=1-(1-x)**r

plot \
 'pdr.data' using 2:($7/$6) t 'simulation tries=7 (PHY-PDR)' w l lw 3, \
 r=7 '' using 2:(f($7/$6)) t 'analysis tries=7' w p pt 5, \
 'pdr.data' using 2:($5/$4) t "simulation tries=7 (MAC-PDR)" w l lw 3, \
 r=7 '' using 2:(f($5/$4)) t 'analysis tries=7' w p pt 4, \
