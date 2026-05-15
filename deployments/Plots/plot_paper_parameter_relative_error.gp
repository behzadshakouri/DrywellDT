#!/usr/bin/env gnuplot
#
# plot_paper_parameter_relative_error.gp
#
# Companion to plot_paper_parameter_history.gp.  Shows relative parameter
# error rather than raw parameter values, which makes convergence and
# tracking behavior easier to see over long deployments.
#
# Run from within deployments/ folder:
#     gnuplot plot_paper_parameter_relative_error.gp
#
# Output: paper_parameter_relative_error.png

param_file = "Bioretention_assimilation/outputs/calibration/parameter_history.csv"

# Truth values: keep synchronized with plot_paper_parameter_history.gp.
truth_EngK   = 10.0
truth_NSAlph = 1.0
truth_NSKsat = 0.01

set datafile separator ","
set datafile commentschars "#c"

set terminal pngcairo size 1800,1500 enhanced font "Helvetica,22"
set output "paper_parameter_relative_error.png"

set multiplot

# ---------- styling ----------
set grid lc rgb "#cccccc" lw 0.8
set border lw 1.5
set style line 1 lc rgb "#d62728" lw 2.6 pt 7 ps 1.0
set style line 2 lc rgb "#555555" lw 1.4 dt (4,4)

# ---------- layout ----------
left_x   = 0.16
right_x  = 0.96
panel_top_y    = 0.96
panel_bottom_y = 0.18
gap            = 0.0
n_panels       = 3
total_height   = panel_top_y - panel_bottom_y - (n_panels - 1) * gap
panel_h        = total_height / n_panels
top_a = panel_top_y
bot_a = top_a - panel_h
top_b = bot_a - gap
bot_b = top_b - panel_h
top_c = bot_b - gap
bot_c = top_c - panel_h

set tics font "Helvetica,18" nomirror
unset key
set xdata time
set timefmt "%s"
set format x ""
unset xlabel
set ylabel font "Helvetica,24" offset -1.5,0
set lmargin at screen left_x
set rmargin at screen right_x

# ---------- (a) EngineeredSoilKsat relative error ----------
set tmargin at screen top_a
set bmargin at screen bot_a
set ylabel "Engineered\nK_{sat}\nerror (%)"
set label 1 "(a)" at graph 0.015,0.86 font "Helvetica,22" front
plot 0 with lines ls 2 notitle, \
     param_file using (($3-25569)*86400):(100.0*($4-truth_EngK)/truth_EngK) with linespoints ls 1 notitle

# ---------- (b) NativeSoilAlpha relative error ----------
set tmargin at screen top_b
set bmargin at screen bot_b
set ylabel "Native\nα error (%)"
set label 1 "(b)" at graph 0.015,0.86 font "Helvetica,22" front
plot 0 with lines ls 2 notitle, \
     param_file using (($3-25569)*86400):(100.0*($5-truth_NSAlph)/truth_NSAlph) with linespoints ls 1 notitle

# ---------- (c) NativeSoilKsat relative error ----------
set tmargin at screen top_c
set bmargin at screen bot_c
set ylabel "Native\nK_{sat}\nerror (%)"
set label 1 "(c)" at graph 0.015,0.86 font "Helvetica,22" front
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3 font "Helvetica,18"
set xlabel "Date" font "Helvetica,24" offset 0,-1.5
plot 0 with lines ls 2 notitle, \
     param_file using (($3-25569)*86400):(100.0*($6-truth_NSKsat)/truth_NSKsat) with linespoints ls 1 notitle

unset multiplot
unset output
print "Wrote paper_parameter_relative_error.png"
