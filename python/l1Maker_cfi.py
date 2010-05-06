import FWCore.ParameterSet.Config as cms

l1Maker = cms.EDFilter("L1Maker",
	aliasPrefix = cms.untracked.string("l1"),
    fillL1Particles = cms.untracked.bool(True),
    l1ParticlesProcessName = cms.untracked.string("RECO"),
    l1GlobalTriggerReadoutRecordInputTag = cms.InputTag("gtDigis"),
    l1GlobalTriggerRecordInputTag = cms.InputTag("l1GtRecord"),
    l1extraParticlesInputTag             = cms.InputTag("l1extraParticles")                   
)
