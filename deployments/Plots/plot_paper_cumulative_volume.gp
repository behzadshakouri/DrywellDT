#!/usr/bin/env gnuplot
#
# Generated paper figure script for OpenHydroTwin.
# Style follows plot_truth_vs_assim.gp: explicit panel placement,
# one legend strip, Helvetica fonts, and the same CSV column mapping.
#
# Run from within deployments/ folder:
#     gnuplot plot_paper_cumulative_volume.gp
#

truth_file      = "Bioretention_truth/outputs/selected_output.csv"
assim_file      = "Bioretention_assimilation/outputs/selected_output.csv"
reanalysis_file = "Bioretention_assimilation/outputs/reanalysis_output.csv"

set datafile separator ","
set datafile commentschars "#t"

# OHQ day serial -> Unix seconds:
#     x = (serial - 25569) * 86400

# =====================================================================
# Figure: cumulative volume accounting
# =====================================================================
# Flow outputs are in m^3/day and the selected-output save interval is
# 1 hour in the current configs. Cumulative volume is therefore computed
# as cumulative sum(flow * dt_days), where dt_days = 1/24.
#
# If save_interval changes, update dt_days below.
dt_days = 1.0/24.0

set terminal pngcairo size 1800,1500 enhanced font "Helvetica,22"
set output "paper_cumulative_volume.png"

set multiplot

# ---------- styling ----------
set grid lc rgb "#cccccc"
set border lw 1.5
set style line 1 lc rgb "#1f77b4" lw 2.0
set style line 2 lc rgb "#d62728" lw 2.2
set style line 3 lc rgb "#2ca02c" lw 2.2 dt (10,4)

# ---------- layout ----------
n_panels = 3
top_y    = 0.91
bottom_y = 0.13
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
plot [0:1][0:1] NaN with lines ls 1 title "Truth twin", \
                NaN with lines ls 2 title "Forward twin", \
                NaN with lines ls 3 title "Reanalysis"

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

# Gnuplot smooth cumulative sums y-values in row order.
# We multiply each flow value by dt_days before cumulating.

# ---------- (a) cumulative underdrain volume ----------
set origin panel_x, panel_y(1)
set size panel_w, panel_h
set ylabel "Cumulative\nunderdrain\nvolume (m^3)"
set label 1 "(a)" at graph 0.015,0.84 font "Helvetica,22" front
plot truth_file      using (($11-25569)*86400):($12*dt_days) smooth cumulative with lines ls 1 notitle, \
     assim_file      using (($11-25569)*86400):($12*dt_days) smooth cumulative with lines ls 2 notitle, \
     reanalysis_file using (($13-25569)*86400):($14*dt_days) smooth cumulative with lines ls 3 notitle

# ---------- (b) cumulative groundwater recharge ----------
set origin panel_x, panel_y(2)
set size panel_w, panel_h
set ylabel "Cumulative\nrecharge\nvolume (m^3)"
set label 1 "(b)" at graph 0.015,0.84 font "Helvetica,22" front
plot truth_file      using (($1-25569)*86400):($2*dt_days) smooth cumulative with lines ls 1 notitle, \
     assim_file      using (($1-25569)*86400):($2*dt_days) smooth cumulative with lines ls 2 notitle, \
     reanalysis_file using (($1-25569)*86400):($2*dt_days) smooth cumulative with lines ls 3 notitle

# ---------- (c) cumulative inflow volume ----------
set origin panel_x, panel_y(3)
set size panel_w, panel_h
set ylabel "Cumulative\ninflow\nvolume (m^3)"
set label 1 "(c)" at graph 0.015,0.84 font "Helvetica,22" front
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3 font "Helvetica,18"
set xlabel "Date" font "Helvetica,23" offset 0,-1.5
plot truth_file      using (($13-25569)*86400):($14*dt_days) smooth cumulative with lines ls 1 notitle, \
     assim_file      using (($13-25569)*86400):($14*dt_days) smooth cumulative with lines ls 2 notitle, \
     reanalysis_file using (($15-25569)*86400):($16*dt_days) smooth cumulative with lines ls 3 notitle

unset multiplot
unset output
print "Wrote paper_cumulative_volume.png"
