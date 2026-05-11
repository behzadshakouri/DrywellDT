#!/usr/bin/env gnuplot
#
# plot_truth_vs_assim.gp
#
# Paper-figure version: eight stacked time-series panels comparing the truth
# twin, the assimilation twin, and the GA reanalysis trajectory.  Panels are
# placed explicitly (not via multiplot layout) so the bottom panel's plot
# area has the same height as the others.  A single legend appears in a
# dedicated strip above the panels.
#
# Run from within deployments/ folder:
#     gnuplot plot_truth_vs_assim.gp
#
# Output: truth_vs_assim.png

# Adjust paths if needed:
truth_file      = "Bioretention_truth/outputs/selected_output.csv"
assim_file      = "Bioretention_assimilation/outputs/selected_output.csv"
reanalysis_file = "Bioretention_assimilation/outputs/reanalysis_output.csv"

set datafile separator ","
set datafile commentschars "#t"

set terminal pngcairo size 1800,2400 enhanced font "Helvetica,22"
set output "truth_vs_assim.png"

set multiplot

# ---------- styling ----------
set grid lc rgb "#cccccc"
set border lw 1.5

set style line 1 lc rgb "#1f77b4" lw 2.0 pt 7 ps 0.4   # truth (blue)
set style line 2 lc rgb "#d62728" lw 2.0 pt 7 ps 0.4   # assim (red)
set style line 3 lc rgb "#2ca02c" lw 2.0 pt 7 ps 0.4   # reanalysis (green)

# ---------- layout geometry ----------
n_panels = 8
top_y    = 0.94
bottom_y = 0.07
panel_h  = (top_y - bottom_y) / n_panels
panel_w  = 0.85
panel_x  = 0.12

panel_y(i) = top_y - i*panel_h

# =====================================================================
# Legend strip — drawn first, in non-time mode (no xdata time set yet),
# with auto ranges so the data panels below it are unaffected.
# =====================================================================
set origin panel_x, top_y + 0.005
set size   panel_w, 0.04
unset xtics
unset ytics
unset border
unset xlabel
unset ylabel
unset title
set autoscale
set key horizontal center top font "Helvetica,22" samplen 3 spacing 1
plot [0:1][0:1] NaN with linespoints ls 1 title "Truth twin", \
                NaN with linespoints ls 2 title "Assimilation twin", \
                NaN with linespoints ls 3 title "Reanalysis"

# =====================================================================
# Restore everything for the data panels.
# =====================================================================
set border lw 1.5
set xtics
set ytics
set tics font "Helvetica,20" nomirror
unset key                                # only one legend in the figure
set autoscale                            # let each panel pick its own range

# Time axis (Unix epoch from inline transform)
set xdata time
set timefmt "%s"

# Hide x-axis decorations on every panel except the bottom one.
set format x ""
unset xlabel

set ylabel font "Helvetica,22" offset -1,0

# ---------- Panel 1: Groundwater Recharge ----------
set origin panel_x, panel_y(1)
set size   panel_w, panel_h
set ylabel "Groundwater\nRecharge\n(m^3/day)"
plot truth_file      using (($1-25569)*86400):2  with linespoints ls 1 notitle, \
     assim_file      using (($1-25569)*86400):2  with linespoints ls 2 notitle, \
     reanalysis_file using (($1-25569)*86400):2  with linespoints ls 3 notitle

# ---------- Panel 2: Infiltration rate ----------
set origin panel_x, panel_y(2)
set size   panel_w, panel_h
set ylabel "Infiltration\nrate\n(m^3/day)"
plot truth_file      using (($3-25569)*86400):4  with linespoints ls 1 notitle, \
     assim_file      using (($3-25569)*86400):4  with linespoints ls 2 notitle, \
     reanalysis_file using (($3-25569)*86400):4  with linespoints ls 3 notitle

# ---------- Panel 3: Overflow ----------
set origin panel_x, panel_y(3)
set size   panel_w, panel_h
set ylabel "Overflow\n(m^3/day)"
plot truth_file      using (($5-25569)*86400):6  with linespoints ls 1 notitle, \
     assim_file      using (($5-25569)*86400):6  with linespoints ls 2 notitle, \
     reanalysis_file using (($5-25569)*86400):6  with linespoints ls 3 notitle

# ---------- Panel 4: Pond water depth (scientific notation) ----------
set origin panel_x, panel_y(4)
set size   panel_w, panel_h
set ylabel "Pond water\ndepth (m)"
set format y "%.1e"
plot truth_file      using (($7-25569)*86400):8  with linespoints ls 1 notitle, \
     assim_file      using (($7-25569)*86400):8  with linespoints ls 2 notitle, \
     reanalysis_file using (($7-25569)*86400):8  with linespoints ls 3 notitle
set format y "%g"

# ---------- Panel 5: Precipitation ----------
set origin panel_x, panel_y(5)
set size   panel_w, panel_h
set ylabel "Precipitation\n(mm/day)"
plot truth_file      using (($9-25569)*86400):10 with linespoints ls 1 notitle, \
     assim_file      using (($9-25569)*86400):10 with linespoints ls 2 notitle, \
     reanalysis_file using (($9-25569)*86400):10 with linespoints ls 3 notitle

# ---------- Panel 6: Underdrain flow (reanalysis cols 13:14) ----------
set origin panel_x, panel_y(6)
set size   panel_w, panel_h
set ylabel "Underdrain\nflow (m^3/day)"
plot truth_file      using (($11-25569)*86400):12 with linespoints ls 1 notitle, \
     assim_file      using (($11-25569)*86400):12 with linespoints ls 2 notitle, \
     reanalysis_file using (($13-25569)*86400):14 with linespoints ls 3 notitle

# ---------- Panel 7: Inflow to pond (reanalysis cols 15:16) ----------
set origin panel_x, panel_y(7)
set size   panel_w, panel_h
set ylabel "Inflow to\npond (m^3/day)"
plot truth_file      using (($13-25569)*86400):14 with linespoints ls 1 notitle, \
     assim_file      using (($13-25569)*86400):14 with linespoints ls 2 notitle, \
     reanalysis_file using (($15-25569)*86400):16 with linespoints ls 3 notitle

# ---------- Panel 8 (bottom): Soil Moisture (reanalysis cols 11:12) ----------
set origin panel_x, panel_y(8)
set size   panel_w, panel_h
set ylabel "Soil\nmoisture (-)"
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3 font "Helvetica,20"
set xlabel "Date" font "Helvetica,22" offset 0,-1.5
plot truth_file      using (($15-25569)*86400):16 with linespoints ls 1 notitle, \
     assim_file      using (($15-25569)*86400):16 with linespoints ls 2 notitle, \
     reanalysis_file using (($11-25569)*86400):12 with linespoints ls 3 notitle

unset multiplot
unset output

print "Wrote truth_vs_assim.png"
