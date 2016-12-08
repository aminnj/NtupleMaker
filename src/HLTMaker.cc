#include "CMS3/NtupleMaker/interface/HLTMaker.h"
//#include <fstream>
#include "TBits.h"

typedef math::XYZTLorentzVectorF LorentzVector;
using namespace edm;
using namespace reco;
using namespace std;

/*
void PrintTriggerObjectInfo( ofstream& outfile, int id, const ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > p4 ){

  outfile.setf( ios::fixed, ios::floatfield );
  outfile << "\t\t\t\t\t"
          <<  id << "\t"
          << "( "
          << setprecision(2) << setw(4) << setfill(' ') << p4.pt() << ", "
          << setprecision(2) << setw(4) << setfill(' ') << p4.eta() << ", "
          << setprecision(2) << setw(4) << setfill(' ') << p4.phi()
          << " )"
          << endl << endl;
  return;
}
*/



HLTMaker::HLTMaker(const edm::ParameterSet& iConfig) : 
hltConfig_(iConfig, consumesCollector(), *this) {

//HLTPrescaleProvider(iConfig, 
//edm::ConsumesCollector&& iC,
//T& module);

  processName_        = iConfig.getUntrackedParameter<string>         ("processName"       );
  fillTriggerObjects_ = iConfig.getUntrackedParameter<bool>           ("fillTriggerObjects");
  prunedTriggerNames_ = iConfig.getUntrackedParameter<vector<string> >("prunedTriggerNames");
  aliasprefix_        = iConfig.getUntrackedParameter<string>         ("aliasPrefix"       );
  processNamePrefix_  = TString(aliasprefix_); //just easier this way....instead of replace processNamePrefix_ everywhere
  triggerPrescaleToken= consumes<pat::PackedTriggerPrescales>(iConfig.getUntrackedParameter<std::string>("triggerPrescaleInputTag"));
  triggerObjectsToken = consumes<pat::TriggerObjectStandAloneCollection>(iConfig.getUntrackedParameter<string>("triggerObjectsName"));
  triggerResultsToken = consumes<edm::TriggerResults>(edm::InputTag("TriggerResults",       "", processName_));

  produces<TBits>                           (Form("%sbits"        ,processNamePrefix_.Data())).setBranchAlias(Form("%s_bits"       ,processNamePrefix_.Data()));
  produces<vector<TString> >                (Form("%strigNames"   ,processNamePrefix_.Data())).setBranchAlias(Form("%s_trigNames"  ,processNamePrefix_.Data()));
  produces<vector<TString> >                (Form("%sfiltNames"   ,processNamePrefix_.Data())).setBranchAlias(Form("%s_filtNames"  ,processNamePrefix_.Data()));
  produces<vector<unsigned int> >           (Form("%sprescales"   ,processNamePrefix_.Data())).setBranchAlias(Form("%s_prescales"  ,processNamePrefix_.Data()));
  produces<vector<unsigned int> >           (Form("%sl1prescales" ,processNamePrefix_.Data())).setBranchAlias(Form("%s_l1prescales",processNamePrefix_.Data()));
  produces<vector<vector<int> > >           (Form("%strigObjsid"  ,processNamePrefix_.Data())).setBranchAlias(Form("%s_trigObjs_id",processNamePrefix_.Data()));

  produces<vector<vector<LorentzVector> > > (Form("%strigObjsp4"  ,processNamePrefix_.Data())).setBranchAlias(Form("%s_trigObjs_p4",processNamePrefix_.Data()));
  produces<vector<vector<bool> > >          (Form("%strigObjspassLast"  ,processNamePrefix_.Data())).setBranchAlias(Form("%s_trigObjs_passLast",processNamePrefix_.Data()));
  produces<vector<vector<TString> > >       (Form("%strigObjsfilters"  ,processNamePrefix_.Data())).setBranchAlias(Form("%s_trigObjs_filters",processNamePrefix_.Data()));
  produces<vector<vector<vector<int> > > >       (Form("%strigObjsfilterIndices"  ,processNamePrefix_.Data())).setBranchAlias(Form("%s_trigObjs_filterIndices",processNamePrefix_.Data()));
  
  // isData_ = iConfig.getParameter<bool>("isData");
  
}

void HLTMaker::beginRun(const edm::Run& iRun, const edm::EventSetup& iSetup){

  // In the case that we are choosing the process name
  // automatically, i.e. the processName_ parameter is
  // an empty string, we can't init  HLTConfigProvider
  // until after we've determined the process name. So
  // don't init here until after we've set processName_
  // in the produce method and init there once and only
  // once. Sounds scary, it is kinda!
  // HLT config _should no longer_ change within runs :)
  if (processName_ != "") {
	bool changed(true);
	// if (hltConfig_.init(iRun,iSetup,"*",changed)) {
        hltConfigProvider_.init(iRun, iSetup, processName_,changed);
	if (hltConfig_.init(iRun,iSetup,processName_,changed)) {
	}
    else throw cms::Exception("HLTMaker::beginRun: config extraction failure with process name " + processName_);
  }

  // flag to force recreation of list/map of filters for all triggers
  makeFilterMap = true;
}

void HLTMaker::produce(edm::Event& iEvent, const edm::EventSetup& iSetup){
  // If the process name is not specified retrieve the  latest
  // TriggerEvent object and the corresponding TriggerResults.
  // We should only have to do this once though, the next time
  // produce is called processName_ should be set.

  //Now using a single processName_ (set to "HLT" in the configuration file). Is this OK? Do we need the flexibility we had before?
  iEvent.getByToken(triggerResultsToken, triggerResultsH_);
  if (! triggerResultsH_.isValid()) throw cms::Exception("HLTMaker::produce: error getting TriggerResults product from Event!");
  
  triggerNames_ = iEvent.triggerNames(*triggerResultsH_); // Does this have to be done for every event?

  iEvent.getByToken(triggerObjectsToken, triggerObjectStandAlonesH_);
  if (!triggerObjectStandAlonesH_.isValid())
    throw cms::Exception("HLTMaker::produce: error getting TriggerObjectsStandAlone product from Event!");

  edm::Handle<pat::PackedTriggerPrescales> triggerPrescalesH_; 
  iEvent.getByToken( triggerPrescaleToken, triggerPrescalesH_);
  if (!triggerPrescalesH_.isValid())
    throw cms::Exception("HLTMaker::produce: error getting PackedTriggerPrescales product from Event!");

////// Printing miniAOD content...
//  for (unsigned int i=0; i<triggerNames_.size(); i++) {
//    std::cout << "triggerNames= " << triggerNames_.triggerName(i) << std::endl;
//  }
//  if ( triggerObjectStandAlonesH_.isValid()) cout<<"Got triggerObjectStandAlonesHandle with size "<<triggerObjectStandAlonesH_->size()<<endl;
//  else cout<<"Couldn't find triggerObjectStandAlonesH"<<endl;
//  for ( uint i = 0; i < triggerObjectStandAlonesH_->size(); i++ ) {
//    pat::TriggerObjectStandAlone TO = triggerObjectStandAlonesH_->at(i);
//    TO.unpackPathNames( triggerNames_ );
//    cout<<"Trigger Object "<<i<<"has pt, eta, phi, id = "<< TO.pt() <<" "<<TO.eta()<<" "<<TO.phi()<<" "<<TO.pdgId()<<endl;
//    cout<<"Trigger Object has hasPathLastFilterAccepted() "<<TO.hasPathLastFilterAccepted()<<endl;
//    cout<<"Trigger Object "<<i<<" has collection() "<<TO.collection()<<endl;
//    std::vector< std::string > path_names = TO.pathNames(false); //TO associated to path
//    cout<<"Trigger Object "<<i<<" associated to "<< path_names.size() <<" pathNames(false): ";
//    for (uint j  = 0; j < path_names.size(); j++) cout<<path_names[j]<<" ";
//    cout<<endl;
//    std::vector< std::string > path_namesPASS = TO.pathNames(true); //make sure they passed!
//    cout<<"Trigger Object "<<i<<" passed "<< path_namesPASS.size() <<" pathNames(true): ";
//    for (uint j  = 0; j < path_namesPASS.size(); j++) cout<<path_namesPASS[j]<<" ";
//    cout<<endl;
//
//   std::vector< std::string > filter_labels = TO.filterLabels();
//   cout<<"Trigger Object "<<i<<" has "<< filter_labels.size() <<" filter_labels: ";
//   for (uint j  = 0; j < filter_labels.size(); j++) cout<<filter_labels[j]<<" ";
//   cout<<endl;
//
//   std::vector< int > filter_ids = TO.filterIds();
//   cout<<"Trigger Object "<<i<<" has "<< filter_ids.size() <<" filter_ids: ";
//   for (uint j  = 0; j < filter_ids.size(); j++) cout<<filter_ids[j]<<" ";
//   cout<<endl;
//   
//
//
//  }
////// END printing miniAOD content

//  // sanity check
//  assert(triggerResultsH_->size()==hltConfig_.size());
 
  auto_ptr<vector<unsigned int> > prescales   (new vector<unsigned int>);
  auto_ptr<vector<unsigned int> > l1prescales (new vector<unsigned int>);

  unsigned int nTriggers = triggerResultsH_->size();
  //if (nTriggers > 768) throw cms::Exception( Form("HLTMaker::produce: number of HLT trigger variables must be increased! ( %d > 768 )", nTriggers) );

  auto_ptr<TBits>                           bits      (new TBits(nTriggers));
  auto_ptr<vector<TString> >                trigNames (new vector<TString>);
  auto_ptr<vector<TString> >                filtNames (new vector<TString>);
  auto_ptr<vector<vector<int> > >           trigObjsid(new vector<vector<int> >);
  auto_ptr<vector<vector<LorentzVector> > > trigObjsp4(new vector<vector<LorentzVector> >);
  auto_ptr<vector<vector<bool> > >          trigObjspassLast(new vector<vector<bool> >);
  auto_ptr<vector<vector<vector<int> > > >       trigObjsfilterIndices(new vector<vector<vector<int> > >);
  trigNames ->reserve(nTriggers);
  trigObjsid->reserve(nTriggers);
  trigObjsp4->reserve(nTriggers);
  trigObjspassLast->reserve(nTriggers);
  trigObjsfilterIndices->reserve(nTriggers);

  std::vector<bool> doFillTrigger; // map trigger index to decision to fill triggerobjects for it

  if (makeFilterMap) {
      for (unsigned int i = 0; i < nTriggers; ++i){
          const std::vector<std::string>& tagsModules = hltConfigProvider_.saveTagsModules(i);
          for (unsigned int idx = 0; idx < tagsModules.size(); idx++) {
              std::string filtname = tagsModules[idx];
              // Sometimes the filtername is preceded by a "-". Not sure why, but remove it if so
              if (filtname[0] == '-') filtname = filtname.substr(1);
              totalFilterList.push_back(filtname);
          }
      }

      // Sort and remove duplicates
      std::sort(totalFilterList.begin(), totalFilterList.end());
      std::set<std::string> s;
      for (unsigned int i = 0; i < totalFilterList.size(); ++i) s.insert(totalFilterList[i]);
      totalFilterList.assign(s.begin(), s.end());

      for (unsigned int i = 0; i < totalFilterList.size(); ++i) 
          totalFilterMap[totalFilterList[i]] = i;

      makeFilterMap = false;
  }
  
  filtNames ->reserve(totalFilterList.size());
  for (unsigned int i = 0; i < totalFilterList.size(); ++i) filtNames->push_back(totalFilterList[i]);

  for(unsigned int i = 0; i < nTriggers; ++i){

      // What is your name?
      const string& name = triggerNames_.triggerName(i);
      trigNames->push_back(name);

      doFillTrigger.push_back(fillTriggerObjects_ && doPruneTriggerName(name));

      //What is your prescale?
	  //Buggy way in miniAOD
      // prescales->push_back( triggerPrescalesH_.isValid() ? triggerPrescalesH_->getPrescaleForIndex(i) : -1 );
	  bool isdata = iEvent.isRealData();

	  if(isdata){

		//get prescale info from hltConfig_
		std::pair<std::vector<std::pair<std::string,int> >,int> detailedPrescaleInfo = hltConfig_.prescaleValuesInDetail(iEvent, iSetup, name);	 
		prescales->push_back( triggerPrescalesH_.isValid() ? detailedPrescaleInfo.second : -1 );
	  
		// save l1 prescale values in standalone vector
		std::vector <int> l1prescalevals;
		for( size_t varind = 0; varind < detailedPrescaleInfo.first.size(); varind++ ){
		  l1prescalevals.push_back(detailedPrescaleInfo.first.at(varind).second);
		}

		// find and save minimum l1 prescale of any ORed L1 that seeds the HLT
		std::vector<int>::iterator result = std::min_element(std::begin(l1prescalevals), std::end(l1prescalevals));
		size_t minind = std::distance(std::begin(l1prescalevals), result);
		// sometimes there are no L1s associated with a HLT. In that case, this branch stores -1 for the l1prescale
		l1prescales->push_back( minind < l1prescalevals.size() ? l1prescalevals.at(minind) : -1 );
	  }
      else {
	  	prescales   -> push_back( hltConfig_.prescaleValue(iEvent, iSetup, name) );
	  	l1prescales -> push_back( 1 );
	  }
	    

	  // Passed... F+
	  if (triggerResultsH_->accept(i)){
		bits->SetBitNumber(i);
	  }
    }


  for(unsigned int itrig = 0; itrig < nTriggers; ++itrig){
    // first index is [itrig], second is [iobj]
    trigObjsid->push_back(vector<int>());
    trigObjsp4->push_back(vector<LorentzVector>());
    trigObjspassLast->push_back(vector<bool>());
    trigObjsfilterIndices->push_back(vector<vector<int> >());
  }

  pat::TriggerObjectStandAlone TO;
  for ( uint iobj = 0; iobj < triggerObjectStandAlonesH_->size(); iobj++ ) {

    TO = triggerObjectStandAlonesH_->at(iobj);
    TO.unpackPathNames( triggerNames_ );

    for(unsigned int itrig = 0; itrig < nTriggers; ++itrig){
      const string& name = triggerNames_.triggerName(itrig);

      if(!doFillTrigger[itrig]) continue;

      // save all objects associated to path, regardless of final result. And save filter names 
      if ( TO.hasPathName(name, false, false ) ) {  
        int storeID = 0;
        std::vector<int> IDs = TO.filterIds();
        if (IDs.size() == 1) storeID = IDs[0];
        else if (IDs.size() > 1) {
          // Making some arbitrary choices
          if ( IDs[0]==85 || IDs[1]==85 ) storeID = 85; // when in doubt call it jet (and not the bjet, 86)
          if ( IDs[0]==92 || IDs[1]==92 ) storeID = 92; // when in doubt call it cluster (and not Photon, 81, or Electron, 82)
        }

        bool saveFilters = false;
        TString filterslist = "";
        std::vector<int> filter_indices;
        if ( IDs.size() > 0 ) {
          int id = abs(IDs[0]);
          // From: TriggerTypeDefs.h
          //	 TriggerL1Mu           = -81,
          //       TriggerL1NoIsoEG      = -82,
          //       TriggerL1IsoEG        = -83,
          //       TriggerPhoton         = +81,
          //       TriggerElectron       = +82,
          //       TriggerMuon           = +83,
          //       TriggerCluster        = +92,
          if ( id == 81 || id == 82 || id == 83 || IDs[0] == 92) saveFilters = true;
          if ( IDs.size() > 1 ) {
            int id = abs(IDs[1]);
            if ( id == 81 || id == 82 || id == 83 || IDs[1] == 92) saveFilters = true;
          }
        }
        if (saveFilters) {
          std::vector< std::string > filter_labels = TO.filterLabels();
          for (uint j  = 0; j < filter_labels.size(); j++) {
            filterslist += filter_labels[j];
            filterslist += " ";

            // NOTE: are we at the point where it's faster to use O(1) maps rather than O(N) find? The total size is around 2-3k filter names.
            // In principle, should never be -1 if all filter names have been found

            // // slow
            // int idx = std::find(totalFilterList.begin(),totalFilterList.end(),filter_labels[j])-totalFilterList.begin();
            // if (idx == (int)totalFilterList.size()) idx = -1;

            // // faster
            // auto lower = std::lower_bound(totalFilterList.begin(), totalFilterList.end(), filter_labels[j]);
            // int idx = lower - totalFilterList.begin();
            // if (lower == totalFilterList.end() || *lower != filter_labels[j]) idx = -1;

            // faster
            int idx = -1;
            auto t = totalFilterMap.find(filter_labels[j]);
            if (t != totalFilterMap.end()) idx = t->second;

            filter_indices.push_back(idx);

          }
        }


        trigObjsid->at(itrig).push_back(storeID);
        trigObjsp4->at(itrig).push_back(LorentzVector(TO.p4()));
        trigObjspassLast->at(itrig).push_back(TO.hasPathName(name, true));
        trigObjsfilterIndices->at(itrig).push_back(filter_indices);
      } // hasPathName

    } // End of loop over triggers

  } // end of loop over trigger objects
	
  // strip upper zeros
  bits->Compact();
  iEvent.put(bits,       Form("%sbits",       processNamePrefix_.Data() ) );

  iEvent.put(prescales  , Form("%sprescales"   , processNamePrefix_.Data() ) );
  iEvent.put(l1prescales, Form("%sl1prescales" , processNamePrefix_.Data() ) );
  iEvent.put(trigNames  , Form("%strigNames"   , processNamePrefix_.Data() ) );
  iEvent.put(filtNames  , Form("%sfiltNames"   , processNamePrefix_.Data() ) );
  iEvent.put(trigObjsid , Form("%strigObjsid"  , processNamePrefix_.Data() ) );
  iEvent.put(trigObjsp4 , Form("%strigObjsp4"  , processNamePrefix_.Data() ) );
  iEvent.put(trigObjspassLast , Form("%strigObjspassLast"  , processNamePrefix_.Data() ) );
  iEvent.put(trigObjsfilterIndices , Form("%strigObjsfilterIndices"  , processNamePrefix_.Data() ) );
}

bool HLTMaker::doPruneTriggerName(const string& name) const
{
  for(unsigned int i = 0; i < prunedTriggerNames_.size(); ++i) {
    // uses wildcard matching like on the command line, not
    // straight up regexp
    TString pattern(prunedTriggerNames_[i]);
    pattern.ToLower();
    TRegexp reg(Form("%s", pattern.Data()), true);
    TString sname(name);
    sname.ToLower();
    if (sname.Index(reg) >= 0)
      return true;
  }
  return false;
}


/*
void HLTMaker::fillTriggerObjectInfo(unsigned int triggerIndex, vector<int>& idV, vector<LorentzVector>& p4V, vector<bool>& passLastV, vector<TString>& filtersV) const {

  // Triggers from miniAOD. 
  // This seems slow, since it does many full loops over the trigger objects. 
  // Would probably be faster to only loop once, and then do multiple loops for the trigger names

  // 1. Get trigger name
  const string& name = triggerNames_.triggerName(triggerIndex);

  // 2. Loop over StandAloneTriggerObjects 
  pat::TriggerObjectStandAlone TO;
  for ( uint i = 0; i < triggerObjectStandAlonesH_->size(); i++ ) {
    TO = triggerObjectStandAlonesH_->at(i);
    TO.unpackPathNames( triggerNames_ );
    // 3. OLD: Check hasPathName( triggerName, true). 
    // This makes sure that the TriggerObjects belongs to the LAST filter in a path. 
    // With respect to CMS2forAOD, we lose the ability to get objects that don't belong to the last filter. 
    // For example, if a path is (Ecut1, Ecut2, Ecut3, Mcut1, Mcut2, Mcut3) we now only get the muon from 
    // Mcut3 and no electron. Instead, in the old CMS2forAOD we would also get the electron from Ecut3.
    //
    // 3. NEW: save all objects associated to path, regardless of final result. And save filter names 
    if ( TO.hasPathName(name, false, false ) ) {  
      int storeID = 0;
      std::vector<int> IDs = TO.filterIds();
      if (IDs.size() == 1) storeID = IDs[0];
      else if (IDs.size() > 1) {
	// Making some arbitrary choices
	if ( IDs[0]==85 || IDs[1]==85 ) storeID = 85; // when in doubt call it jet (and not the bjet, 86)
	if ( IDs[0]==92 || IDs[1]==92 ) storeID = 92; // when in doubt call it cluster (and not Photon, 81, or Electron, 82)

      }
      
      bool saveFilters = false;
      TString filterslist = "";
      if ( IDs.size() > 0 ) {
      	int id = abs(IDs[0]);
      	// From: TriggerTypeDefs.h
      	//	 TriggerL1Mu           = -81,
      	//       TriggerL1NoIsoEG      = -82,
      	//       TriggerL1IsoEG        = -83,
      	//       TriggerPhoton         = +81,
      	//       TriggerElectron       = +82,
      	//       TriggerMuon           = +83,
      	//       TriggerCluster        = +92,
      	if ( id == 81 || id == 82 || id == 83 || IDs[0] == 92) saveFilters = true;
      	if ( IDs.size() > 1 ) {
      	  int id = abs(IDs[1]);
      	  if ( id == 81 || id == 82 || id == 83 || IDs[1] == 92) saveFilters = true;
      	}
      }
      if (saveFilters) {
      	std::vector< std::string > filter_labels = TO.filterLabels();
      	for (uint j  = 0; j < filter_labels.size(); j++) {
      	  filterslist += filter_labels[j];
      	  filterslist += " ";
      	}
      }

      idV.push_back( storeID );
      p4V.push_back( LorentzVector( TO.p4() ) );
      passLastV.push_back( TO.hasPathName(name, true) );
      filtersV.push_back(filterslist);
    } // hasPathName
  } // End of loop over trigger objects in miniAOD
  


  // End of Triggers from miniAOD

} // end HLTMaker::fillTriggerObjectInfo()
*/

//define this as a plug-in
DEFINE_FWK_MODULE(HLTMaker);
