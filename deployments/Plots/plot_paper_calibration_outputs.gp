#!/usr/bin/env gnuplot
#
# plot_paper_calibration_outputs.gp
#
# Paper figure: three stacked panels comparing the truth twin, the
# assimilation (forward) twin, and the GA reanalysis trajectory for the
# three observation variables most directly informed by the calibrated
# parameters:
#
#   (a) Underdrain flow     (driven by EngineeredSoilKsat, NativeSoilKsat)
#   (b) Soil moisture       (driven by NativeSoilAlpha, EngineeredSoilKsat)
#   (c) Infiltration rate   (driven by EngineeredSoilKsat)
#
# Run from within deployments/ folder:
#     gnuplot plot_paper_calibration_outputs.gp
#
# Output: paper_calibration_outputs.png
#
# Visual conventions:
#   - thin solid blue   : truth twin (noisy reference)
#   - thick solid red   : forward twin (uses calibrated parameters)
#   - thick dashed green: reanalysis (re-solve under most recent calibration)
#   These line styles read in greyscale as well as in colour.
#
# CSV layouts (kept here for reference when editing column indices):
#
# selected_output.csv (truth and assim — declaration order):
#   1: t  2: Groundwater Recharge   3: t  4: Infiltration rate
#   5: t  6: Overflow               7: t  8: Pond water depth
#   9: t  10: Precipitation         11: t  12: Underdrain flow
#   13: t  14: Inflow to the pond   15: t  16: Soil Moisture
#
# reanalysis_output.csv (alphabetical — Soil Moisture / Underdrain swapped):
#   1: t  2: Groundwater Recharge   3: t  4: Infiltration rate
#   5: t  6: Overflow               7: t  8: Pond water depth
#   9: t  10: Precipitation         11: t  12: Soil Moisture
#   13: t  14: Underdrain flow      15: t  16: Inflow to the pond
#
# t is OHQ day-serial (Excel epoch).  Convert to Unix seconds inline:
#     unix_sec = (serial - 25569) * 86400

# ---------- file paths ----------
truth_file      = "Bioretention_truth/outputs/selected_output.csv"
assim_file      = "Bioretention_assimilation/outputs/selected_output.csv"
reanalysis_file = "Bioretention_assimilation/outputs/reanalysis_output.csv"

set datafile separator ","
set datafile commentschars "#t"

# ---------- canvas ----------
set terminal pngcairo size 1800,1500 enhanced font "Helvetica,22"
set output "paper_calibration_outputs.png"

set multiplot

# ---------- styling ----------
set grid lc rgb "#cccccc" lw 0.8
set border lw 1.5

# Three styles distinguishable in greyscale:
#   ls 1: thin solid blue   (truth — let the noise show via line wiggle)
#   ls 2: thick solid red   (forward / assim twin)
#   ls 3: thick dashed green (reanalysis)
set style line 1 lc rgb "#1f77b4" lw 1.5 dt solid
set style line 2 lc rgb "#d62728" lw 2.6 dt solid
set style line 3 lc rgb "#2ca02c" lw 2.6 dt (10,4)

# ---------- layout geometry ----------
n_panels = 3
top_y    = 0.91
bottom_y = 0.13
panel_h  = (top_y - bottom_y) / n_panels
panel_w  = 0.80
panel_x  = 0.14

panel_y(i) = top_y - i*panel_h

# =====================================================================
# Legend strip — drawn first, in non-time mode, with auto ranges so the
# data panels below it are unaffected.
# =====================================================================
set origin panel_x, top_y + 0.012
set size   panel_w, 0.05
unset xtics
unset ytics
unset border
unset xlabel
unset ylabel
unset title
set autoscale
set key horizontal center top font "Helvetica,22" samplen 4 spacing 1.2
plot [0:1][0:1] NaN with lines ls 1 title "Truth", \
                NaN with lines ls 2 title "Forward", \
                NaN with lines ls 3 title "Reanalysis"

# =====================================================================
# Restore for the data panels.
# =====================================================================
set border lw 1.5
set xtics
set ytics
set tics font "Helvetica,18" nomirror
unset key
set autoscale

set xdata time
set timefmt "%s"

set format x ""
unset xlabel

set ylabel font "Helvetica,24" offset -1,0

# Vertical reference line at the end of the GA's most recent calibration
# window (here: serial 43862 = 2020-02-01).  Adjust if your run uses a
# different boundary, or comment out if not applicable.
cal_end = (43862 - 25569) * 86400
set arrow 99 from cal_end, graph 0 to cal_end, graph 1 \
    nohead lc rgb "#888888" lw 1.0 dt (4,4) front

# ---------- Panel (a): Underdrain flow ----------
# (reanalysis cols 13:14)
set origin panel_x, panel_y(1)
set size   panel_w, panel_h
set ylabel "Underdrain\nflow (m^3/day)"
set label 1 "(a)" at graph 0.015, 0.86 font "Helvetica,22" front
plot truth_file      using (($11-25569)*86400):12 with lines ls 1 notitle, \
     assim_file      using (($11-25569)*86400):12 with lines ls 2 notitle, \
     reanalysis_file using (($13-25569)*86400):14 with lines ls 3 notitle

# ---------- Panel (b): Soil moisture ----------
# (reanalysis cols 11:12) — clip y-range to drop the cold-start spike
set origin panel_x, panel_y(2)
set size   panel_w, panel_h
set ylabel "Soil\nmoisture (-)"
set label 1 "(b)" at graph 0.015, 0.86 font "Helvetica,22" front
set yrange [0.25:0.50]
plot truth_file      using (($15-25569)*86400):16 with lines ls 1 notitle, \
     assim_file      using (($15-25569)*86400):16 with lines ls 2 notitle, \
     reanalysis_file using (($11-25569)*86400):12 with lines ls 3 notitle
set yrange [*:*]            # restore autoscale for subsequent panels

# ---------- Panel (c, bottom): Infiltration rate ----------
set origin panel_x, panel_y(3)
set size   panel_w, panel_h
set ylabel "Infiltration\nrate (m^3/day)"
set label 1 "(c)" at graph 0.015, 0.86 font "Helvetica,22" front
set format x "%Y-%m-%d"
set xtics rotate by -30 offset 0,-0.3 font "Helvetica,18"
set xlabel "Date" font "Helvetica,24" offset 0,-1.5
plot truth_file      using (($3-25569)*86400):4  with lines ls 1 notitle, \
     assim_file      using (($3-25569)*86400):4  with lines ls 2 notitle, \
     reanalysis_file using (($3-25569)*86400):4  with lines ls 3 notitle

unset multiplot
unset output

print "Wrote paper_calibration_outputs.png"
