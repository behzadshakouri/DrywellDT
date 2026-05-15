#!/usr/bin/env gnuplot
#
# Generated paper figure script for OpenHydroTwin.
# Style follows plot_truth_vs_assim.gp: explicit panel placement,
# one legend strip, Helvetica fonts, and the same CSV column mapping.
#
# Run from within deployments/ folder:
#     gnuplot plot_paper_event_zoom.gp
#

truth_file      = "Bioretention_truth/outputs/selected_output.csv"
assim_file      = "Bioretention_assimilation/outputs/selected_output.csv"
reanalysis_file = "Bioretention_assimilation/outputs/reanalysis_output.csv"

set datafile separator ","
set datafile commentschars "#t"

# OHQ day serial -> Unix seconds:
#     x = (serial - 25569) * 86400

# =====================================================================
# Figure: event-scale zoom for visually small fluctuations
# =====================================================================
# Choose the storm/event window here.  Values are OHQ/Excel day-serials.
# Example: 43862 = 2020-02-01.
# Edit these two numbers to center the figure on the storm you want.
x0_serial = 43855
x1_serial = 43862
x0 = (x0_serial - 25569) * 86400
x1 = (x1_serial - 25569) * 86400

set terminal pngcairo size 1800,1600 enhanced font "Helvetica,22"
set output "paper_event_zoom.png"

set multiplot

# ---------- styling ----------
set grid lc rgb "#cccccc"
set border lw 1.5

set style line 1 lc rgb "#1f77b4" lw 2.0 pt 7 ps 0.35   # Truth
set style line 2 lc rgb "#d62728" lw 2.2 pt 7 ps 0.35   # Forward / assimilation
set style line 3 lc rgb "#2ca02c" lw 2.2 dt (10,4)       # Reanalysis
set style line 4 lc rgb "#444444" lw 1.6                 # Precip

# ---------- layout ----------
n_panels = 4
top_y    = 0.91
bottom_y = 0.10
panel_h  = (top_y - bottom_y) / n_panels
panel_w  = 0.84
panel_x  = 0.13
panel_y(i) = top_y - i*panel_h

# ---------- legend strip ----------
set origin panel_x, top_y + 0.012
set size   panel_w, 0.055
unset xtics
unset ytics
unset border
unset xlabel
unset ylabel
unset title
set autoscale
set key horizontal center top font "Helvetica,22" samplen 4 spacing 1.2
plot [0:1][0:1] NaN with linespoints ls 1 title "Truth twin", \
                NaN with linespoints ls 2 title "Forward twin", \
                NaN with lines       ls 3 title "Reanalysis"

# ---------- restore ----------
set border lw 1.5
set xtics
set ytics
set tics font "Helvetica,18" nomirror
unset key
set xdata time
set timefmt "%s"
set xrange [x0:x1]
set format x ""
unset xlabel
set ylabel font "Helvetica,23" offset -1,0

# ---------- (a) precipitation forcing ----------
set origin panel_x, panel_y(1)
set size   panel_w, panel_h
set ylabel "Precipitation\n(mm/day)"
set label 1 "(a)" at graph 0.015,0.84 font "Helvetica,22" front
plot truth_file using (($9-25569)*86400):10 with impulses ls 4 notitle

# ---------- (b) soil moisture; fixed narrow range helps fluctuations ----------
set origin panel_x, panel_y(2)
set size   panel_w, panel_h
set ylabel "Soil\nmoisture (-)"
set label 1 "(b)" at graph 0.015,0.84 font "Helvetica,22" front
# Uncomment/edit if the automatic range still hides small event fluctuations:
# set yrange [0.30:0.45]
plot truth_file      using (($15-25569)*86400):16 with linespoints ls 1 notitle, \
     assim_file      using (($15-25569)*86400):16 with linespoints ls 2 notitle, \
     reanalysis_file using (($11-25569)*86400):12 with lines       ls 3 notitle
set yrange [*:*]

# ---------- (c) underdrain flow ----------
set origin panel_x, panel_y(3)
set size   panel_w, panel_h
set ylabel "Underdrain\nflow (m^3/day)"
set label 1 "(c)" at graph 0.015,0.84 font "Helvetica,22" front
plot truth_file      using (($11-25569)*86400):12 with linespoints ls 1 notitle, \
     assim_file      using (($11-25569)*86400):12 with linespoints ls 2 notitle, \
     reanalysis_file using (($13-25569)*86400):14 with lines       ls 3 notitle

# ---------- (d) pond water depth ----------
set origin panel_x, panel_y(4)
set size   panel_w, panel_h
set ylabel "Pond water\ndepth (m)"
set label 1 "(d)" at graph 0.015,0.84 font "Helvetica,22" front
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3 font "Helvetica,18"
set xlabel "Date" font "Helvetica,23" offset 0,-1.5
set format y "%.1e"
plot truth_file      using (($7-25569)*86400):8 with linespoints ls 1 notitle, \
     assim_file      using (($7-25569)*86400):8 with linespoints ls 2 notitle, \
     reanalysis_file using (($7-25569)*86400):8 with lines       ls 3 notitle
set format y "%g"

unset multiplot
unset output
print "Wrote paper_event_zoom.png"
