For propagation models:

./waf --run "lab5 --apManager=ns3::ConstantRateWifiManager --phyRate=ErpOfdmRate54Mbps --propagationModel=0" | tee -a ./NAKAGAMI_3LOG
./waf --run "lab5 --apManager=ns3::ConstantRateWifiManager --phyRate=ErpOfdmRate54Mbps --propagationModel=1" | tee -a ./LOG_DIST_LOSS
./waf --run "lab5 --apManager=ns3::ConstantRateWifiManager --phyRate=ErpOfdmRate54Mbps --propagationModel=2" | tee -a ./THREE_LOG_DIST
./waf --run "lab5 --apManager=ns3::ConstantRateWifiManager --phyRate=ErpOfdmRate54Mbps --propagationModel=3" | tee -a ./FRIIS

# manually delete first lines of waf output:
# -----------
# Waf: Entering directory `/home/mihai/facultate/ns-3-dev/build'
# Waf: Leaving directory `/home/mihai/facultate/ns-3-dev/build'
# Build commands will be stored in build/compile_commands.json
# 'build' finished successfully (4.567s)
# [UDP TRAFFIC] src=0 dst=1 rate=54000100bps
# Timestamp(seconds) Distance(m) Throughput(Mbps)
# 1.1 0 0 

For adaptive MCS:

./waf --run "lab5 --apManager=ns3::AarfWifiManager --phyRate=ErpOfdmRate6Mbps" | tee -a ./aarf
./waf --run "lab5 --apManager=ns3::ConstantRateWifiManager --phyRate=ErpOfdmRate6Mbps" | tee -a ./6m
./waf --run "lab5 --apManager=ns3::ConstantRateWifiManager --phyRate=ErpOfdmRate24Mbps" | tee -a ./24
./waf --run "lab5 --apManager=ns3::ConstantRateWifiManager --phyRate=ErpOfdmRate54Mbps" | tee -a ./54m

Plotting:

gnuplot rate_adaptation.plot
gnuplot propagation_model.plot