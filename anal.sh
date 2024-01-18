#!/bin/bash 

if [ -z "$1" ]
  then
    echo "Run number not specified!"
    echo "usage:"
    echo "   ./anal.sh runNumber"
    exit
fi


runNumber=$1
#root -b -q AnalData2textFile.C\($runNumber,0\)
#root -b -q AnalData2textFile.C\($runNumber,1\)
./a.out $runNumber 0
./a.out $runNumber 1

sleep 5

cd pedestalsRun3
tar -cvf ${runNumber}.tar $runNumber
gzip ${runNumber}.tar

cd ..
echo "pedestalsRun3/${runNumber}.tar.gz produced"

#scp pedestalsRun3/${runNumber}.tar.gz poghos@lxplus.cern.ch:/eos/home-p/poghos/P2_data/PedRun/.
