# PedestalDAlocal
usage
- ./init.sh
- ./copyData.sh
- ./anal.sh runNumber

OR

1. raw data are supposed to be located in data/
    readoutEMCAL_runNumber.raw for EMCAL
    readoutDCAL_runNumber.raw  for DCAL
2. root -b -q AnalData2textFile.C\(runNumber,0\)  -- for EMCAL
3. root -b -q AnalData2textFile.C\(runNumber,1\)  -- for DCAL
4. ./a.out runNumber 0 -- for EMCAL
5. ./a.out runNumber 1 -- for DCAL

----
analPedFile_EMC.C  assumes a merged input from EMCAL/DCAL data and is meant to be used on the DCS LN.

