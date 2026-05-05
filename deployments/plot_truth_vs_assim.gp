#!/usr/bin/env gnuplot
#
# plot_truth_vs_assim.gp
#
# Overlays selected_output.csv from the truth and assimilation deployments
# for visual comparison. Eight panels, one per observation variable.
#
# Run from within deployments/ folder:
#     gnuplot plot_truth_vs_assim.gp
#
# Output: truth_vs_assim.png in the current directory.
#
# CSV layout (both files share this structure):
#   col 1: t                              col 2: Groundwater Recharge (m3/day)
#   col 3: t                              col 4: Infiltration rate (m3/day)
#   col 5: t                              col 6: Overflow (m3/day)
#   col 7: t                              col 8: Pond water depth (m)
#   col 9: t                              col 10: Precipitation (mm/day)
#   col 11: t                             col 12: Underdrain flow (m3/day)
#   col 13: t                             col 14: inflow to the pond (m3/day)
#   col 15: t                             col 16: Soil Moisture
#
# t is OHQ day-serial (days since 1899-12-30).

# Adjust these paths if your folder layout differs:
truth_file = "Bioretention_truth/outputs/selected_output.csv"
assim_file = "Bioretention_assimilation/outputs/selected_output.csv"

set datafile separator ","
set datafile commentschars "#t"   # skip header lines starting with 't' or '#'

set terminal pngcairo size 1600,1200 enhanced font "Helvetica,11"
set output "truth_vs_assim.png"

set multiplot layout 4,2 \
    title "Truth Twin vs Assimilation Twin — selected outputs" \
    font "Helvetica,14"

set grid lc rgb "#cccccc"
set key top right font "Helvetica,9"
set xlabel "OHQ day-serial"
set tics font "Helvetica,9"

set lmargin 10
set rmargin 3
set tmargin 2
set bmargin 3

set style line 1 lc rgb "#1f77b4" lw 2 pt 7 ps 0.4   # truth (blue)
set style line 2 lc rgb "#d62728" lw 2 pt 7 ps 0.4   # assim (red)

# --- Panel 1: Groundwater Recharge ---------------------------------------
set title "Groundwater Recharge (m^3/day)"
set ylabel "m^3/day"
plot truth_file using 1:2  with linespoints ls 1 title "truth", \
     assim_file using 1:2  with linespoints ls 2 title "assim"

# --- Panel 2: Infiltration rate ------------------------------------------
set title "Infiltration rate (m^3/day)"
set ylabel "m^3/day"
plot truth_file using 3:4  with linespoints ls 1 title "truth", \
     assim_file using 3:4  with linespoints ls 2 title "assim"

# --- Panel 3: Overflow ---------------------------------------------------
set title "Overflow (m^3/day)"
set ylabel "m^3/day"
plot truth_file using 5:6  with linespoints ls 1 title "truth", \
     assim_file using 5:6  with linespoints ls 2 title "assim"

# --- Panel 4: Pond water depth -------------------------------------------
set title "Pond water depth (m)"
set ylabel "m"
plot truth_file using 7:8  with linespoints ls 1 title "truth", \
     assim_file using 7:8  with linespoints ls 2 title "assim"

# --- Panel 5: Precipitation ----------------------------------------------
set title "Precipitation (mm/day)"
set ylabel "mm/day"
plot truth_file using 9:10 with linespoints ls 1 title "truth", \
     assim_file using 9:10 with linespoints ls 2 title "assim"

# --- Panel 6: Underdrain flow --------------------------------------------
set title "Underdrain flow (m^3/day)"
set ylabel "m^3/day"
plot truth_file using 11:12 with linespoints ls 1 title "truth", \
     assim_file using 11:12 with linespoints ls 2 title "assim"

# --- Panel 7: Inflow to the pond -----------------------------------------
set title "Inflow to the pond (m^3/day)"
set ylabel "m^3/day"
plot truth_file using 13:14 with linespoints ls 1 title "truth", \
     assim_file using 13:14 with linespoints ls 2 title "assim"

# --- Panel 8: Soil Moisture ----------------------------------------------
set title "Soil Moisture"
set ylabel "(-)"
plot truth_file using 15:16 with linespoints ls 1 title "truth", \
     assim_file using 15:16 with linespoints ls 2 title "assim"

unset multiplot
unset output

print "Wrote truth_vs_assim.png"
