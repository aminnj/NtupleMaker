#!/bin/bash

##CMSSW Environment
#: ${CMSSW_BASE:?"[ERROR] patchesToSource.sh: \$CMSSW_BASE not set. Perhaps you forgot to set your CMSSW environment."} 
#
##cd
#cd $CMSSW_BASE/src/CMS3/NtupleMaker
#
## #############
## # MET tools #
## #############

pushd $CMSSW_BASE/src
git cms-addpkg PhysicsTools
popd
#mkdir $CMSSW_BASE/previous_code
#mv $CMSSW_BASE/src/* $CMSSW_BASE/previous_code/
#pushd $CMSSW_BASE/src/
#git cms-merge-topic -u cms-met:METCorUnc74X
#mv $CMSSW_BASE/previous_code/* $CMSSW_BASE/src/
#popd
#rm -rf $CMSSW_BASE/previous_code
#mv $CMSSW_BASE/previous_code/RecoEcal/EgammaClusterProducers $CMSSW_BASE/src/RecoEcal/

#for some reason this checks out a BUNCH of shit we don't need.  Delete the unneeded packages here.
#for i in `ls`; do if [ "$i" != "CMGTools" ] && [ "$i" != "CMS3" ] && [ "$i" != "DQMOffline" ] && [ "$i" != "JMEAnalysis" ] && [ "$i" != "JetMETCorrections" ] && [ "$i" != "PhysicsTools" ] && [ "$i" != "RecoEcal" ] && [ "$i" != "RecoMET" ] && [ "$i" != "Validation" ]; then rm -rf $i ; fi; done

##############
## MVA JetId #
##############
 
git clone https://github.com/latinos/UserCode-CMG-CMGTools-External $CMSSW_BASE/src/CMGTools/External
pushd $CMSSW_BASE/src/CMGTools/External
git checkout V00-03-01
rm plugins/PileupJetIdProducer.cc
popd


# #######################
# # LCG dictionaries #
# #######################

git clone https://github.com/cmstas/Dictionaries $CMSSW_BASE/src/CMS3/Dictionaries

# ####################
# # jet tool box     #
# ####################

git clone https://github.com/cms-jet/JetToolbox $CMSSW_BASE/src/JMEAnalysis/JetToolbox -b jetToolbox_74X
pushd $CMSSW_BASE/src/JMEAnalysis/JetToolbox
git checkout a80553163684d718e65e44d9b385a4aa11475659
popd

mkdir $CMSSW_BASE/bullshit  
mv $CMSSW_BASE/src/* $CMSSW_BASE/bullshit/
git cms-addpkg   RecoEcal/EgammaClusterProducers
mv $CMSSW_BASE/bullshit/* $CMSSW_BASE/src/
rmdir $CMSSW_BASE/bullshit

####### line needs to be added ###############
inputfile="$CMSSW_BASE/src/RecoEcal/EgammaClusterProducers/src/PFECALSuperClusterProducer.cc"
grep "desc.setAllowAnything();" $inputfile 2>&1 > /dev/null
doNothing=$?
if [ ! $doNothing = "0" ]; 
then
  echo "line does not exist. Adding now."
  sed -i 's/edm::ParameterSetDescription desc;/edm::ParameterSetDescription desc;\n  desc.setAllowAnything();/' $inputfile
else
  echo "line already exists. File $inputfile will be unchanged."
fi

####################

#######################

# run checkdeps
printf "\nchecking deps:\n"
git cms-checkdeps -a

# compile
#cd $CMSSW_BASE/src
#scram b -j 20
