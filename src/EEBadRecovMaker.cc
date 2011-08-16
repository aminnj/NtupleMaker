// -*- C++ -*-
// $Id: EEBadRecovMaker.cc,v 1.3 2011/08/10 21:48:22 dbarge Exp $

// C++
#include <memory>

// Header
#include "CMS2/NtupleMaker/interface/EEBadRecovMaker.h"

// user include files
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHit.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"

using namespace std;
using namespace edm;

EEBadRecovMaker::EEBadRecovMaker(const edm::ParameterSet& iConfig) {

  //
  eeRHSrc_      = iConfig.getParameter <edm::InputTag> ("EERecHitSource");
  minRecovE_    = iConfig.getParameter <double>        ("MinRecovE"     );
  maxNrRecHits_ = iConfig.getParameter <unsigned int>  ("MaxNrRecHits"  );

  //
  produces<bool> ("ecalnoiseeeBadRecov") .setBranchAlias("ecalnoise_eeBadRecov" );
  produces<bool> ("ecalnoiseeeRedRecHits").setBranchAlias("ecalnoise_eeRedRecHits");

}


EEBadRecovMaker::~EEBadRecovMaker() {}

void EEBadRecovMaker::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {

  /////////////////////////
  // Get Reduced RecHits // 
  /////////////////////////

  edm::Handle<EcalRecHitCollection> eeRHs;
  iEvent.getByLabel(eeRHSrc_, eeRHs);

  ///////////////////
  // Filter Result //
  ///////////////////

  // # EE Reduced Rec Hits
  bool eeRedRecHits = ( eeRHs->size() < maxNrRecHits_ );

  // EEBadRecov Filter
  double recovE = 0;
  for (EcalRecHitCollection::const_iterator eerh = eeRHs->begin(); eerh != eeRHs->end(); ++eerh) {
    if (eerh->time()!=0 ||
        eerh->energy()<=0 ||
        eerh->checkFlag(EcalRecHit::kPoorReco) ||
        eerh->checkFlag(EcalRecHit::kOutOfTime) ||
        eerh->checkFlag(EcalRecHit::kFaultyHardware) ||
        eerh->checkFlag(EcalRecHit::kNoisy) ||
        eerh->checkFlag(EcalRecHit::kPoorCalib) ||
        eerh->checkFlag(EcalRecHit::kSaturated) ||
        eerh->checkFlag(EcalRecHit::kLeadingEdgeRecovered) ||
        eerh->checkFlag(EcalRecHit::kNeighboursRecovered) ||
        eerh->checkFlag(EcalRecHit::kTowerRecovered) ||
        eerh->checkFlag(EcalRecHit::kDead) ||
        eerh->checkFlag(EcalRecHit::kKilled) ||
        eerh->checkFlag(EcalRecHit::kL1SpikeFlag) ||
        eerh->checkFlag(EcalRecHit::kWeird) ||
        eerh->checkFlag(EcalRecHit::kDiWeird)) continue;
    recovE += eerh->energy();
    if (recovE > minRecovE_) break;
  }
  bool result = (recovE < minRecovE_);

  //std::auto_ptr<bool> pOut(new bool(result) ); 
  //iEvent.put( pOut, "Result" ); 
  //if(taggingMode_) return true; else return result;

  //
  auto_ptr <bool> ecalnoise_eeBadRecov   (new bool (result)       );
  auto_ptr <bool> ecalnoise_eeRedRecHits (new bool (eeRedRecHits) );

  //
  iEvent.put(ecalnoise_eeBadRecov  , "ecalnoiseeeBadRecov"  );
  iEvent.put(ecalnoise_eeRedRecHits, "ecalnoiseeeRedRecHits");

}

//define this as a plug-in
DEFINE_FWK_MODULE(EEBadRecovMaker);