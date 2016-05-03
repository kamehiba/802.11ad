set terminal png
set output "gnuplot.png"
set title "Flow vs Throughput"
set xlabel "Flow"
set ylabel "Throughput"

set xrange [0:]
plot "-"  title "Throughput" with linespoints
0 0
e
