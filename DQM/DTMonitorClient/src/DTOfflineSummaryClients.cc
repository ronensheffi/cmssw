

/*
 *  See header file for a description of this class.
 *
 *  $Date: 2008/10/22 09:38:05 $
 *  $Revision: 1.1 $
 *  \author G. Mila - INFN Torino
 */


#include <DQM/DTMonitorClient/src/DTOfflineSummaryClients.h>

// Framework
#include <FWCore/Framework/interface/Event.h>
#include <FWCore/Framework/interface/EventSetup.h>
#include <FWCore/ParameterSet/interface/ParameterSet.h>

#include "DQMServices/Core/interface/DQMStore.h"
#include "DQMServices/Core/interface/MonitorElement.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <string>

using namespace edm;
using namespace std;


DTOfflineSummaryClients::DTOfflineSummaryClients(const ParameterSet& ps) : nevents(0) {

  LogVerbatim("DTDQM|DTMonitorClient|DTOfflineSummaryClients") << "[DTOfflineSummaryClients]: Constructor";
  
  
  dbe = Service<DQMStore>().operator->();

}

DTOfflineSummaryClients::~DTOfflineSummaryClients(){
  LogVerbatim ("DTDQM|DTMonitorClient|DTOfflineSummaryClients") << "DTOfflineSummaryClients: analyzed " << nevents << " events";
  
}

void DTOfflineSummaryClients::beginRun(Run const& run, EventSetup const& eSetup) {

  LogVerbatim("DTDQM|DTMonitorClient|DTOfflineSummaryClients") <<"[DTOfflineSummaryClients]: BeginRun"; 

  // book the summary histos
  dbe->setCurrentFolder("DT/EventInfo"); 
  summaryReport = dbe->bookFloat("reportSummary");
  // Initialize to 1 so that no alarms are thrown at the beginning of the run
  summaryReport->Fill(1.);

  summaryReportMap = dbe->book2D("reportSummaryMap","DT Report Summary Map",12,1,13,5,-2,3);
  summaryReportMap->setAxisTitle("sector",1);
  summaryReportMap->setAxisTitle("wheel",2);

  dbe->setCurrentFolder("DT/EventInfo/reportSummaryContents");

  for(int wheel = -2; wheel != 3; ++wheel) {
    stringstream streams;
    streams << "DT_Wheel" << wheel;
    string meName = streams.str();    
    theSummaryContents.push_back(dbe->bookFloat(meName));
    // Initialize to 1 so that no alarms are thrown at the beginning of the run
    theSummaryContents[wheel+2]->Fill(1.);
  }




}


void DTOfflineSummaryClients::endJob(void){
  
  LogVerbatim ("DTDQM|DTMonitorClient|DTOfflineSummaryClients") <<"[DTOfflineSummaryClients]: endJob"; 

}


void DTOfflineSummaryClients::endRun(Run const& run, EventSetup const& eSetup) {
  
  LogVerbatim ("DTDQM|DTMonitorClient|DTOfflineSummaryClients") <<"[DTOfflineSummaryClients]: endRun"; 

}


void DTOfflineSummaryClients::analyze(const Event& event, const EventSetup& context){

   nevents++;
   if(nevents%1000 == 0) {
     LogVerbatim("DTDQM|DTMonitorClient|DTOfflineSummaryClients") << "[DTOfflineSummaryClients] Analyze #Run: " << event.id().run()
					 << " #Event: " << event.id().event()
					 << " LS: " << event.luminosityBlock()	
					 << endl;
   }
}


void DTOfflineSummaryClients::endLuminosityBlock(LuminosityBlock const& lumiSeg, EventSetup const& context) {
  
  LogVerbatim("DTDQM|DTMonitorClient|DTOfflineSummaryClients")
    << "[DTSummaryClients]: End of LS transition, performing the DQM client operation" << endl;

  // reset the monitor elements
  summaryReportMap->Reset();
  summaryReport->Reset();
  for(int ii = 0; ii != 5; ++ii) {
    theSummaryContents[ii]->Reset();
  }

  bool noDTData = false;

  // Check if DT data in each ROS have been read out and set the SummaryContents and the ErrorSummary
  // accordignly
  /*MonitorElement * dataIntegritySummary = dbe->get("DT/00-DataIntegrity/DataIntegritySummary");
  if(dataIntegritySummary != 0) {
  int nDisabledFED = 0;
  for(int wheel = 1; wheel != 6; ++wheel) { // loop over the wheels
    int nDisablesROS = 0;
    for(int sect = 1; sect != 13; ++sect) { // loop over sectors
      if(dataIntegritySummary->getBinContent(sect,wheel) == 1) {
	nDisablesROS++;
      }
    }
    if(nDisablesROS == 12) {
      nDisabledFED++;
      theSummaryContents[wheel-1]->Fill(0);
    }
  }
  
  if(nDisabledFED == 5) {
    noDTData = true;
    summaryReport->Fill(-1);
  }
  
  } else {
    LogError("DTDQM|DTMonitorClient|DTSummaryClients")
      << "Data Integrity Summary not found with name: DT/00-DataIntegrity/DataIntegritySummary" <<endl;
      }*/

  double totalStatus = 0;
  // protection 
  bool efficiencyFound = true;

  // Fill the map using, at the moment, only the information from DT chamber efficiency
  // problems at a granularity smaller than the chamber are ignored
  for(int wheel=-2; wheel<=2; wheel++){ // loop over wheels
    // retrieve the chamber efficiency summary
    stringstream str;
    str << "DT/02-Segments/segmentSummary_W" << wheel;
    MonitorElement * segmentWheelSummary =  dbe->get(str.str());
    if(segmentWheelSummary != 0) {
      int nFailingChambers = 0;
      for(int sector=1; sector<=12; sector++){ // loop over sectors
	for(int station = 1; station != 5; ++station) { // loop over stations
	  double chamberStatus = segmentWheelSummary->getBinContent(sector, station);
	  LogTrace("DTDQM|DTMonitorClient|DTOfflineSummaryClients")
	    << "Wheel: " << wheel << " Stat: " << station << " Sect: " << sector << " status: " << chamberStatus << endl;
	  if(chamberStatus == 0 || chamberStatus == 1) {
	    summaryReportMap->Fill(sector, wheel, 0.25);
	  } else {
	    nFailingChambers++;
	  }
	  LogTrace("DTDQM|DTMonitorClient|DTOfflineSummaryClients") << " sector (" << sector << ") status on the map is: "
							     << summaryReportMap->getBinContent(sector, wheel+3) << endl;
	}

      }
      theSummaryContents[wheel+2]->Fill((48.-nFailingChambers)/48.);
      totalStatus += (48.-nFailingChambers)/48.;
    } else {
      efficiencyFound = false;
      LogWarning("DTDQM|DTMonitorClient|DTOfflineSummaryClients")
	<< " [DTOfflineSummaryClients] Segment Summary not found with name: " << str.str() << endl;
    }
  }


  //if(efficiencyFound && !noDTData)
//   if(efficiencyFound)
//     summaryReport->Fill(totalStatus/5.);

//   cout << "-----------------------------------------------------------------------------" << endl;
//   cout << " In the endLuminosityBlock: " << endl;
//   for(int wheel = -2; wheel != 3; ++wheel) {
//     for(int sector = 1; sector != 13; sector++) {
//       cout << " wheel: " << wheel << " sector: " << sector << " status on the map is: "
// 	   << summaryReportMap->getBinContent(sector, wheel+3) << endl;
//     }
//   }
//   cout << "-----------------------------------------------------------------------------" << endl;


}


