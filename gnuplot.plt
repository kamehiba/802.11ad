set terminal png
set output "gnuplot.png"
set title "Packet Byte Count vs. Time \n\nTrace Source Path: /NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx"
set xlabel "Time (Seconds)"
set ylabel "Packet Byte Count"

set key outside center below
set datafile missing "-nan"
plot 
