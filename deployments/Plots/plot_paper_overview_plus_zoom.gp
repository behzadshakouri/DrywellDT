#!/usr/bin/env gnuplot
#
# Generated paper figure script for OpenHydroTwin.
# Style follows plot_truth_vs_assim.gp: explicit panel placement,
# one legend strip, Helvetica fonts, and the same CSV column mapping.
#
# Run from within deployments/ folder:
#     gnuplot plot_paper_overview_plus_zoom.gp
#

truth_file      = "Bioretention_truth/outputs/selected_output.csv"
assim_file      = "Bioretention_assimilation/outputs/selected_output.csv"
reanalysis_file = "Bioretention_assimilation/outputs/reanalysis_output.csv"

set datafile separator ","
set datafile commentschars "#t"

# OHQ day serial -> Unix seconds:
#     x = (serial - 25569) * 86400

# =====================================================================
# Figure: long-term overview plus event zooms
# =====================================================================
# This figure solves the "long record hides fluctuations" problem:
# panel (a) shows the full record for continuity; panels (b)-(d) zoom
# into a storm window where soil-moisture and flow dynamics are visible.

# Event window, OHQ/Excel serial days. Edit as needed.
x0_serial = 43855
x1_serial = 43862
x0 = (x0_serial - 25569) * 86400
x1 = (x1_serial - 25569) * 86400

set terminal pngcairo size 1800,1700 enhanced font "Helvetica,22"
set output "paper_overview_plus_zoom.png"

set multiplot

# ---------- styling ----------
set grid lc rgb "#cccccc"
set border lw 1.5
set style line 1 lc rgb "#1f77b4" lw 1.8 pt 7 ps 0.30
set style line 2 lc rgb "#d62728" lw 2.0 pt 7 ps 0.30
set style line 3 lc rgb "#2ca02c" lw 2.0 dt (10,4)
set style line 4 lc rgb "#444444" lw 1.5

# ---------- layout ----------
n_panels = 4
top_y    = 0.91
bottom_y = 0.10
panel_h  = (top_y - bottom_y) / n_panels
panel_w  = 0.84
panel_x  = 0.13
panel_y(i) = top_y - i*panel_h

# ---------- legend ----------
set origin panel_x, top_y + 0.012
set size panel_w, 0.055
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

# ---------- common ----------
set border lw 1.5
set xtics
set ytics
set tics font "Helvetica,18" nomirror
unset key
set xdata time
set timefmt "%s"
set format x ""
unset xlabel
set ylabel font "Helvetica,23" offset -1,0
set autoscale x
set autoscale y

# ---------- (a) full-period soil moisture overview ----------
set origin panel_x, panel_y(1)
set size panel_w, panel_h
set ylabel "Soil moisture\nfull record (-)"
set label 1 "(a)" at graph 0.015,0.84 font "Helvetica,22" front
# Draw the zoom window as two vertical reference lines.
set arrow 11 from x0, graph 0 to x0, graph 1 nohead lc rgb "#777777" lw 1.2 dt (4,4) front
set arrow 12 from x1, graph 0 to x1, graph 1 nohead lc rgb "#777777" lw 1.2 dt (4,4) front
plot truth_file      using (($15-25569)*86400):16 with lines ls 1 notitle, \
     assim_file      using (($15-25569)*86400):16 with lines ls 2 notitle, \
     reanalysis_file using (($11-25569)*86400):12 with lines ls 3 notitle
unset arrow 11
unset arrow 12

# ---------- switch to zoomed x-range ----------
set xrange [x0:x1]

# ---------- (b) zoomed precipitation ----------
set origin panel_x, panel_y(2)
set size panel_w, panel_h
set ylabel "Precipitation\n(mm/day)"
set label 1 "(b)" at graph 0.015,0.84 font "Helvetica,22" front
plot truth_file using (($9-25569)*86400):10 with impulses ls 4 notitle

# ---------- (c) zoomed soil moisture ----------
set origin panel_x, panel_y(3)
set size panel_w, panel_h
set ylabel "Soil moisture\nzoom (-)"
set label 1 "(c)" at graph 0.015,0.84 font "Helvetica,22" front
plot truth_file      using (($15-25569)*86400):16 with linespoints ls 1 notitle, \
     assim_file      using (($15-25569)*86400):16 with linespoints ls 2 notitle, \
     reanalysis_file using (($11-25569)*86400):12 with lines       ls 3 notitle

# ---------- (d) zoomed underdrain flow ----------
set origin panel_x, panel_y(4)
set size panel_w, panel_h
set ylabel "Underdrain\nflow (m^3/day)"
set label 1 "(d)" at graph 0.015,0.84 font "Helvetica,22" front
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3 font "Helvetica,18"
set xlabel "Date" font "Helvetica,23" offset 0,-1.5
plot truth_file      using (($11-25569)*86400):12 with linespoints ls 1 notitle, \
     assim_file      using (($11-25569)*86400):12 with linespoints ls 2 notitle, \
     reanalysis_file using (($13-25569)*86400):14 with lines       ls 3 notitle

unset multiplot
unset output
print "Wrote paper_overview_plus_zoom.png"
