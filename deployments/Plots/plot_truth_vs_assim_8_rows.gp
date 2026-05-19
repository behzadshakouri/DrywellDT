#!/usr/bin/env gnuplot
#
# plot_truth_vs_assim.gp
#
# Overlays selected_output.csv from the truth and assimilation deployments
# alongside reanalysis_output.csv (the GA's best-fit trajectory under the
# most recent calibrated parameters across the full assimilation window).
# Eight stacked panels (8 rows × 1 column), one per observation variable,
# sharing the date axis at the bottom.
#
# Run from within deployments/ folder:
#     gnuplot plot_truth_vs_assim.gp
#
# Output: truth_vs_assim.png in the current directory.
#
# CSV layouts:
#
# selected_output.csv (truth and assim — declaration order):
#   col 1: t   col 2: Groundwater Recharge (m3/day)
#   col 3: t   col 4: Infiltration rate (m3/day)
#   col 5: t   col 6: Overflow (m3/day)
#   col 7: t   col 8: Pond water depth (m)
#   col 9: t   col 10: Precipitation (mm/day)
#   col 11: t  col 12: Underdrain flow (m3/day)
#   col 13: t  col 14: inflow to the pond (m3/day)
#   col 15: t  col 16: Soil Moisture
#
# reanalysis_output.csv (alphabetical order — Soil Moisture and
# Underdrain flow swapped relative to selected_output.csv):
#   col 1: t   col 2: Groundwater Recharge (m3/day)
#   col 3: t   col 4: Infiltration rate (m3/day)
#   col 5: t   col 6: Overflow (m3/day)
#   col 7: t   col 8: Pond water depth (m)
#   col 9: t   col 10: Precipitation (mm/day)
#   col 11: t  col 12: Soil Moisture
#   col 13: t  col 14: Underdrain flow (m3/day)
#   col 15: t  col 16: inflow to the pond (m3/day)
#
# t is OHQ day-serial (days since 1899-12-30, the Excel epoch).
# We convert to Unix epoch seconds inline:
#     unix_sec = (serial - 25569) * 86400
# so gnuplot's time-axis machinery can format it as a date.

# Adjust these paths if your folder layout differs:
truth_file      = "Bioretention_truth/outputs/selected_output.csv"
assim_file      = "Bioretention_assimilation/outputs/selected_output.csv"
reanalysis_file = "Bioretention_assimilation/outputs/reanalysis_output.csv"

set datafile separator ","
set datafile commentschars "#t"   # skip header lines starting with 't' or '#'

# Square overall image; 8 stacked wide panels.  Base font size is set
# generously for paper figures; per-element overrides below tune relative
# emphasis (panel titles slightly larger, legend a touch smaller).
set terminal pngcairo size 1600,1600 enhanced font "Helvetica,18"
set output "truth_vs_assim_8_rows.png"

# No overall figure title.
set multiplot layout 8,1

set grid lc rgb "#cccccc"
set key top right font "Helvetica,16"
set tics font "Helvetica,16"

# Wide-and-short panels: keep left/right margins tight so each panel uses
# nearly the full image width.  Top/bottom margins are small; the bottom
# panel gets extra room for the rotated date labels.
set lmargin 14
set rmargin 4
set tmargin 1.6
set bmargin 0.6

# Time axis (Unix epoch seconds; data is converted inline from OHQ day-serial).
set xdata time
set timefmt "%s"

# Hide x tic labels and x label on all panels; the bottom panel will
# re-enable them.
set format x ""
unset xlabel

set style line 1 lc rgb "#1f77b4" lw 2 pt 7 ps 0.4   # truth (blue)
set style line 2 lc rgb "#d62728" lw 2 pt 7 ps 0.4   # assim (red)
set style line 3 lc rgb "#2ca02c" lw 2 pt 7 ps 0.4   # reanalysis (green)

# --- Panel 1: Groundwater Recharge ---------------------------------------
set title "Groundwater Recharge (m^3/day)" font "Helvetica,18"
set ylabel "m^3/day" font "Helvetica,16"
plot truth_file      using (($3-25569)*86400):4  with linespoints ls 1 title "truth", \
     assim_file      using (($3-25569)*86400):4  with linespoints ls 2 title "assim", \
     reanalysis_file using (($3-25569)*86400):4  with linespoints ls 3 title "reanalysis"

# --- Panel 2: Infiltration rate ------------------------------------------
set title "Infiltration rate (m^3/day)" font "Helvetica,18"
set ylabel "m^3/day" font "Helvetica,16"
plot truth_file      using (($5-25569)*86400):6  with linespoints ls 1 title "truth", \
     assim_file      using (($5-25569)*86400):6  with linespoints ls 2 title "assim", \
     reanalysis_file using (($5-25569)*86400):6  with linespoints ls 3 title "reanalysis"

# --- Panel 3: Overflow ---------------------------------------------------
set title "Overflow (m^3/day)" font "Helvetica,18"
set ylabel "m^3/day" font "Helvetica,16"
plot truth_file      using (($9-25569)*86400):10  with linespoints ls 1 title "truth", \
     assim_file      using (($9-25569)*86400):10  with linespoints ls 2 title "assim", \
     reanalysis_file using (($9-25569)*86400):10  with linespoints ls 3 title "reanalysis"

# --- Panel 4: Pond water depth -------------------------------------------
set title "Pond water depth (m)" font "Helvetica,18"
set ylabel "m" font "Helvetica,16"
plot truth_file      using (($11-25569)*86400):12  with linespoints ls 1 title "truth", \
     assim_file      using (($11-25569)*86400):12  with linespoints ls 2 title "assim", \
     reanalysis_file using (($11-25569)*86400):12  with linespoints ls 3 title "reanalysis"

# --- Panel 5: Precipitation ----------------------------------------------
set title "Precipitation (mm/day)" font "Helvetica,18"
set ylabel "mm/day" font "Helvetica,16"
plot truth_file      using (($13-25569)*86400):14 with linespoints ls 1 title "truth", \
     assim_file      using (($13-25569)*86400):14 with linespoints ls 2 title "assim", \
     reanalysis_file using (($13-25569)*86400):14 with linespoints ls 3 title "reanalysis"

# --- Panel 6: Underdrain flow --------------------------------------------
# (reanalysis has Underdrain at cols 13:14 due to alphabetical ordering)
set title "Underdrain flow (m^3/day)" font "Helvetica,18"
set ylabel "m^3/day" font "Helvetica,16"
plot truth_file      using (($17-25569)*86400):18 with linespoints ls 1 title "truth", \
     assim_file      using (($17-25569)*86400):18 with linespoints ls 2 title "assim", \
     reanalysis_file using (($17-25569)*86400):18 with linespoints ls 3 title "reanalysis"

# --- Panel 7: Inflow to the pond -----------------------------------------
# (reanalysis has inflow at cols 15:16 due to alphabetical ordering)
set title "Inflow to the pond (m^3/day)" font "Helvetica,18"
set ylabel "m^3/day" font "Helvetica,16"
plot truth_file      using (($7-25569)*86400):8 with linespoints ls 1 title "truth", \
     assim_file      using (($7-25569)*86400):8 with linespoints ls 2 title "assim", \
     reanalysis_file using (($7-25569)*86400):8 with linespoints ls 3 title "reanalysis"

# --- Panel 8 (bottom): Soil Moisture — show date axis here ---------------
# (reanalysis has Soil Moisture at cols 11:12 due to alphabetical ordering)
set title "Soil Moisture" font "Helvetica,18"
set ylabel "(-)" font "Helvetica,16"
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3
set xlabel "Date" font "Helvetica,18" offset 0,-1.2
set bmargin 5   # extra room for rotated date labels and xlabel
plot truth_file      using (($15-25569)*86400):16 with linespoints ls 1 title "truth", \
     assim_file      using (($15-25569)*86400):16 with linespoints ls 2 title "assim", \
     reanalysis_file using (($15-25569)*86400):16 with linespoints ls 3 title "reanalysis"

unset multiplot
unset output

print "Wrote truth_vs_assim.png"