#!/bin/bash

for fits in `ls start_time_*_coarse_*_real.fits`
do
   outdir=${fits%%.fits}

   if [[ -d ${outdir} ]]; then
      echo "Directory $outdir already exists -> skipped"
   else
      mkdir -p ${outdir}/   
      echo "~/github/msfitslib/apps/mecube2cubes ${fits} -d ${outdir}/"
      ~/github/msfitslib/apps/mecube2cubes ${fits} -d ${outdir}/
   fi
done


for dirname in `ls -d start_time_*_coarse_*_real`
do
  echo
  cd $dirname
  pwd
  ~/github/msfitslib/apps/diff_channels.sh
  cd ..
done
