#!/usr/bin/env gnuplot
#
# Generated paper figure script for OpenHydroTwin.
# Style follows plot_truth_vs_assim.gp: explicit panel placement,
# one legend strip, Helvetica fonts, and the same CSV column mapping.
#
# Run from within deployments/ folder:
#     gnuplot plot_paper_assimilation_residuals.gp
#

truth_file      = "Bioretention_truth/outputs/selected_output.csv"
assim_file      = "Bioretention_assimilation/outputs/selected_output.csv"
reanalysis_file = "Bioretention_assimilation/outputs/reanalysis_output.csv"

set datafile separator ","
set datafile commentschars "#t"

# OHQ day serial -> Unix seconds:
#     x = (serial - 25569) * 86400

# =====================================================================
# Figure: assimilation residuals / correction signals
# =====================================================================
# This figure plots deviations from the truth twin.  It is useful when
# raw time series hide small fluctuations, especially soil moisture.
#
# It uses Unix `paste` to combine CSV files column-wise. Run on Linux/macOS.
# Column shifts:
#   truth + forward file: forward columns are truth columns + 16
#   truth + reanalysis file: reanalysis columns are truth columns + 16
#
# Difference definitions:
#   Forward residual   = forward - truth
#   Reanalysis residual = reanalysis - truth

ta = "< paste -d, Bioretention_truth/outputs/selected_output.csv Bioretention_assimilation/outputs/selected_output.csv"
tr = "< paste -d, Bioretention_truth/outputs/selected_output.csv Bioretention_assimilation/outputs/reanalysis_output.csv"

set terminal pngcairo size 1800,1500 enhanced font "Helvetica,22"
set output "paper_assimilation_residuals.png"

set multiplot

# ---------- styling ----------
set grid lc rgb "#cccccc"
set border lw 1.5
set style line 1 lc rgb "#d62728" lw 2.2 pt 7 ps 0.35      # Forward - truth
set style line 2 lc rgb "#2ca02c" lw 2.2 dt (10,4)          # Reanalysis - truth
set style line 3 lc rgb "#555555" lw 1.2 dt (4,4)           # zero line

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
plot [0:1][0:1] NaN with linespoints ls 1 title "Forward - truth", \
                NaN with lines       ls 2 title "Reanalysis - truth"

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

# ---------- (a) soil moisture residual ----------
set origin panel_x, panel_y(1)
set size panel_w, panel_h
set ylabel "Soil moisture\nresidual (-)"
set label 1 "(a)" at graph 0.015,0.84 font "Helvetica,22" front
plot 0 with lines ls 3 notitle, \
     ta using (($15-25569)*86400):($32-$16) with linespoints ls 1 notitle, \
     tr using (($15-25569)*86400):($28-$16) with lines       ls 2 notitle

# ---------- (b) underdrain flow residual ----------
set origin panel_x, panel_y(2)
set size panel_w, panel_h
set ylabel "Underdrain\nresidual\n(m^3/day)"
set label 1 "(b)" at graph 0.015,0.84 font "Helvetica,22" front
plot 0 with lines ls 3 notitle, \
     ta using (($11-25569)*86400):($28-$12) with linespoints ls 1 notitle, \
     tr using (($11-25569)*86400):($30-$12) with lines       ls 2 notitle

# ---------- (c) pond depth residual ----------
set origin panel_x, panel_y(3)
set size panel_w, panel_h
set ylabel "Pond depth\nresidual (m)"
set label 1 "(c)" at graph 0.015,0.84 font "Helvetica,22" front
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3 font "Helvetica,18"
set xlabel "Date" font "Helvetica,23" offset 0,-1.5
set format y "%.1e"
plot 0 with lines ls 3 notitle, \
     ta using (($7-25569)*86400):($24-$8) with linespoints ls 1 notitle, \
     tr using (($7-25569)*86400):($24-$8) with lines       ls 2 notitle
set format y "%g"

unset multiplot
unset output
print "Wrote paper_assimilation_residuals.png"
