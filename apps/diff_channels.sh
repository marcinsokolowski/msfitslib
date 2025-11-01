#!/bin/bash

# timestep00015_channel00000.fits

ch=0
while [[ $ch -lt 32 ]];
do
   ch_str=`echo $ch | awk '{printf("%05d",$1);}'`
   
   ls timestep?????_channel${ch_str}.fits > fits_list_channel${ch_str}
   median_channel=median_channel${ch_str}.fits
   
   echo "~/github/msfitslib/apps/median_fits_images fits_list_channel${ch_str} ${median_channel}"
   ~/github/msfitslib/apps/median_fits_images fits_list_channel${ch_str} ${median_channel}
   
   mkdir -p diff/
   echo "~/github/msfitslib/apps/subtract_images fits_list_channel${ch_str} ${median_channel} diff/"
   ~/github/msfitslib/apps/subtract_images fits_list_channel${ch_str} ${median_channel} diff/   
   
   ch=$(($ch+1))
done
