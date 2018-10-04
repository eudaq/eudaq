#!/bin/sh
echo "#on NAF or bird be sure to source first:"
echo "#source /cvmfs/ilc.desy.de/sw/x86_64_gcc49_sl6/v01-19-05/init_ilcsoft.sh"

gnuplotcmd=gnuplot
#gnuplotcmd=/afs/desy.de/user/k/kvas/pool/gnuplot/gnuplot-5.0.7/src/gnuplot
analysiscmd=/home/calice/eudaq/data/noise/AhcalNoiseAnalysis/Release/AhacalNoiseAnalysis
#path=/nfs/dust/ilc/group/flchcal/AHCAL_Commissioning_2017_2018/Cosmic/raw
path=/home/calice/eudaq/data/AhcalRaw
txtout=./txt/
pdfout=./pdf/

for filename in `ls ${path}|grep raw|head -n -1`
do
    #extract the run number from the filename
    run=`echo "$filename"|sed  -n 's/.*Raw[^0-9]*\([0-9]\{4,\}\)[^0-9]*/\1/p'`
    sraw=${path}/${filename}
    #    sraw=`find $path | grep -i -e "ahcalraw.*${run}.raw$"`
    #if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
    #    sraw="$1"
    #fi
    echo -n "#AHCAL spiroc file: ${sraw}"
    if [ ! -f ${txtout}noise${run}.txt ]
    then
        echo ": analyzing raw data"
        $analysiscmd \
            --spiroc_raw_filename "${sraw}" \
            --correlation_shift 2123 \
            --bxid_length 160 \
            --reject_validated \
            --run_number ${run} \
            >${txtout}noise${run}.txt
    else
	echo ": skipping (txt file exists)"
	continue
    fi

    ASIC_START=1; #what is the chipid of the first asic (1 for the typical numbering. 160 planned for the big prototype
    ASIC_PATTERN="10 8 2 0 11 9 3 1 14 12 6 4 15 13 7 5"; #order of asic how they will be plot in multiplot
    #gnuplot script, that makes 1 big pdf
    $gnuplotcmd -e "
           stats '${txtout}noise${run}.txt' u 5 nooutput;
           runlength=STATS_max; print '#duration of the run: ', STATS_max;
           stats '${txtout}noise${run}.txt' u 8 nooutput;
           hitmax=STATS_max; print '#Maximum hits for whole asic: ', STATS_max;
           maxChipFreq=runlength==0?100:1.0*hitmax/runlength;
           stats '${txtout}noise${run}.txt' u 6 nooutput;
           hitmax=STATS_max; print '#Maximum hits for single channel: ', STATS_max;
           maxChannelFreq=runlength==0?100:1.0*hitmax/runlength;
           
           roughxpos = \"18 18 12 12 18 18 12 12 06 06 00 00 06 06 00 00\";
           roughypos = \"18 12 23 17 06 00 11 05 18 12 23 17 06 00 11 05\";
           ydirection = \"1  1 -1 -1  1  1 -1 -1  1  1 -1 -1  1  1 -1 -1\";
           getxpos(asic,channel)=floor(word(roughxpos,1+floor(asic+256)%16))+(floor(channel)/6);
           getypos(asic,channel)=(floor(word(roughypos,1+floor(asic+256)%16))+(floor(channel)%6)*word(ydirection,1+floor(asic+256)%16));
           set style fill solid 1.0;set palette rgbformulae 22,13,-31;
           
           set terminal pdfcairo size 16,9 enhanced background rgb \"white\";
           set output sprintf(\"${pdfout}noise%s.pdf\",\"${run}\");

           print sprintf(\"Printing asic summary frequency maps\");
           do for [ls=0:1] {
             set cbtics font \",8\"; unset xtics; unset ytics; 
             set multiplot layout 6,8 title sprintf(\"Noise frequency in chips [Hz]. Run ${run}. Maximum %f Hz\",maxChipFreq) font \", 20\";
             set boxwidth 1;
             set grid;
             set xrange [-0.5:3.5]; set yrange [-0.5:3.5];
             set lmargin 1;set rmargin 1;set tmargin 1.5;set bmargin 0.2;
             if (ls==0) { set logscale cb; set cbrange [0.1:maxChipFreq]; } else {
             unset logscale cb; set cbrange [0:maxChipFreq];}; 
             do for [p=0:47] {
                 set title sprintf(\"port %d\",p) offset 0,-0.8;
                 plot '${txtout}noise${run}.txt'  u (\$2!=p?NaN:\$4!=0?NaN:floor(getxpos(\$3-1,\$4)/6)):(floor(getypos(\$3-1,\$4)/6)):(0.5):(0.5):(\$8<2?NaN:\$8/\$5) palette w boxxyerrorbars notitle,\
                      '' u (\$2!=p?NaN:\$4!=0?NaN:floor(getxpos(\$3-1,\$4)/6)):(floor(getypos(\$3-1,\$4)/6)):(sprintf(\"%.2f\",\$8/\$5)) w labels font \",8\" notitle;
             };
             unset multiplot; 
           };

           print sprintf(\"Printing channel frequency noise frequency maps\");
           do for [ls=0:1] {
             set cbtics font \",8\";
             set multiplot layout 6,8 title sprintf(\"Noise frequency in channels [Hz]. Run ${run}. Maximum %f Hz\", maxChannelFreq) font \", 20\";
             set boxwidth 1;
             set xrange [-0.5:23.5]; set yrange [-0.5:23.5];
             set xtics format \"\"; set xtics (-0.5,5.5,11.5,17.5); set ytics format \"\"; set ytics (-0.5,5.5,11.5,17.5); 
             set grid nomxtics; set grid front;
             set lmargin 1;set rmargin 1;set tmargin 1.5;set bmargin 0.2;
             if (ls==0) { set logscale cb; set cbrange [0.001:maxChannelFreq];} 
             else { unset logscale cb; set cbrange [0:maxChannelFreq];};
             do for [p=0:47] {
                 set title sprintf(\"port %d\",p) offset 0,-0.8;
                 plot '${txtout}noise${run}.txt'  u (\$2!=p?NaN:getxpos(\$3-1,\$4)):(getypos(\$3-1,\$4)):(0.5):(0.5):(\$6<0?NaN:\$6/\$5) palette w boxxyerrorbars notitle\
             };
             set xtics format \"% g\"; set xtics auto; set ytics format \"% g\"; set ytics auto; 
             set grid back;
             unset multiplot; 
           };
           
           print sprintf(\"Printing layer hitmaps \");
           do for [ls=0:1] {
             do for [p=0:39] {
               unset logscale y ;set yrange [*:*];
               stats '${txtout}noise${run}.txt' u (\$2!=p?0:\$4) nooutput;
               if (STATS_sum != 0) {
               print sprintf(\"Printing map for port %d\",p);
                 set title sprintf(\"Noise frequency in channels [Hz]. Run ${run}. Port %02d\",p) font \",20\";
                 set xrange [-0.5:23.5]; set yrange [-0.5:23.5];
                 set grid; set xtics 1 norotate; set ytics 1;
                 set cblabel \"pixel noise frequency [Hz]\";
                 if (ls==0) { set logscale cb; set cbrange [0.001:maxChannelFreq];} 
                 else { unset logscale cb; set cbrange [0:maxChannelFreq];};
                 set lmargin 40;set rmargin 40;set tmargin 3;set bmargin 3;
                 plot '${txtout}noise${run}.txt'  u (\$2!=p?NaN:getxpos(\$3-1,\$4)):(getypos(\$3-1,\$4)):(0.5):(0.5):(\$6<0?NaN:\$6/\$5) palette w boxxyerrorbars notitle,\
                      '' u (\$2!=p?NaN:getxpos(\$3-1,\$4)):(getypos(\$3-1,\$4)):(sprintf(\"%.2f\",(1.0*\$6)/\$5)) w labels notitle;
               } else { print sprintf(\"Skipping map for port %d\",p);};
             };
             set ytics auto;
           };

           print sprintf(\"Printing asic summaries histogram \");
           do for [ls=0:1] {
             set multiplot layout 6,8 title 'ASIC noise frequency in modules [Hz]. Run ${run}' font \", 20\";
             set boxwidth 1; set style fill solid 0.5; set grid;
             set xtics 1 font \",8\" rotate; set ytics font \",8\";          
             set xrange [-0.5:15.5];
             set lmargin 4;set rmargin 1;set tmargin 1.5;set bmargin 1.5;
             if (ls==0) { set logscale y; set yrange [0.1:1.05*maxChipFreq]; } else {
             unset logscale y; set yrange [0:1.05*maxChipFreq];}; 
             do for [p=0:47] {
                 set title sprintf(\"port %d\",p) offset 0,-0.8 font \",12\";
                 plot '${txtout}noise${run}.txt' u (\$4!=0?NaN:\$2!=p?NaN:floor(\$3+256-${ASIC_START})%16):(\$8/\$5) w boxes notitle;
             };
             unset multiplot; 
           };

           print sprintf(\"Printing channel histogram summaries \");
           unset logscale y; set yrange [0:*]; set xrange [0:*];
           do for [ls=0:1] {
             do for [p=0:39] {
               unset logscale y ;set yrange [*:*];
               stats '${txtout}noise${run}.txt' u (\$2!=p?0:\$4) nooutput;
               if (STATS_sum != 0) {
                 print sprintf(\"Printing histograms for port %d\",p);
                   set multiplot layout 4,4 title sprintf(\"CHannel noise frequency in asics [Hz]. Run ${run}. Port %02d\",p) font \",20\";
                   set xtics 6; set mxtics 6; set grid xtics mxtics lt -1 lt 0;
                   set xrange [-0.5:35.5];
                   if (ls==0) { set logscale y; set yrange [0.001:1.05*maxChannelFreq]; } 
                   else { unset logscale y; set yrange [0:1.05*maxChannelFreq]; };
                   set lmargin 4;set rmargin 1;set tmargin 1.5;set bmargin 1.5;
                   do for [a in \"${ASIC_PATTERN}\"] {
                       set title sprintf(\"asic index %s\",a) offset 0,-0.5;
                       plot '${txtout}noise${run}.txt' u (int(\$3+256-${ASIC_START})%16!=a?NaN:\$2!=p?NaN:\$4):(\$6/\$5) w boxes notitle
                   };
                   unset multiplot;
               } else { print sprintf(\"Skipping histograms for port %d\",p);};
             };
           };
"
done
