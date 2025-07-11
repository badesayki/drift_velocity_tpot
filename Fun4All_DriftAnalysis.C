#include <fun4all/SubsysReco.h>
#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllDstInputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>
#include <fun4all/Fun4AllRunNodeInputManager.h>
#include <fun4all/Fun4AllUtils.h>

#include <phool/PHRandomSeed.h>
#include <phool/recoConsts.h>

// Micromegas
#include <micromegas/MicromegasCombinedDataDecoder.h>
#include <micromegas/MicromegasCombinedDataEvaluation.h>
#include <micromegas/MicromegasClusterizer.h>

// INTT
#include <intt/InttCombinedRawDataDecoder.h>
#include <intt/InttClusterizer.h>

// TPC
#include <tpc/TpcCombinedRawDataUnpacker.h>

#include <tpccalib/PHTpcResiduals.h>

#include <trackingdiagnostics/TrackResiduals.h>

// own modules
#include <g4eval_hp/EventCounter_hp.h>
#include <g4eval_hp/MicromegasEvaluator_hp.h>
#include <g4eval_hp/MicromegasClusterEvaluator_hp.h>
#include <g4eval_hp/MicromegasTrackEvaluator_hp.h>
#include <g4eval_hp/TrackingEvaluator_hp.h>

// local macros
#include "G4Setup_sPHENIX.C"
#include "G4_Global.C"

#include "Trkr_RecoInit.C"
#include "Trkr_TpcReadoutInit.C"

#include "Trkr_Clustering.C"
#include "Trkr_Reco.C"

R__LOAD_LIBRARY(libfun4all.so)
R__LOAD_LIBRARY(libg4eval_hp.so)

R__LOAD_LIBRARY(libmicromegas.so)

#include "make_filelist.C"
#include <algorithm>

std::map<int, float> csv_to_map(std::string filename, int column)
{
  //This is a function that takes 'filename' and pours the 0th column (runNumber) and the 'column'th column into a map of int and float. It's meant to read the csv files we produce.
  std::ifstream file(filename);
  std::map<int, float> data;
  std::string line;

  //skip the first line for header
  std::getline(file, line); 
  while (std::getline(file, line)) 
    {
      std::stringstream ss(line);
      std::string x;
      int runNumber;
      float value;
      int i = 0;

      while (std::getline(ss, x, ','))
	{
	  if (i == 0) runNumber = std::stoi(x); //this is bc i want the type to be int
	  if (i == column) value = std::stof(x); //this is bc i want the type to be float
	  i++;
	}

      data[runNumber] = value;
    }

  file.close();
  return data;
}

std::map<int, std::string> path_map(std::string filename, int column)
{
  std::ifstream file(filename);
  std::map<int, std::string> data;
  std::string line;

  // Skip header
  std::getline(file, line); 
  
  while (std::getline(file, line)) 
    {
      std::stringstream ss(line);
      std::string x;
      int runNumber = -1;
      std::string value;
      int i = 0;

      while (std::getline(ss, x, ','))
	{
	  if (i == 0) runNumber = std::stoi(x);
	  if (i == column) value = x; //i want to keep this a string
	  i++;
	}
      data[runNumber] = value;
    }

  file.close();
  return data;
}

std::vector<int> run_number_vector(const char* run_file)
{
  std::ifstream file(run_file); 
  std::vector<int> runNumbers;
  int runNumber;
  while (file >> runNumber){runNumbers.push_back(runNumber);}
  file.close();
  return runNumbers;
}

std::vector<int> nearest_neighbor(int runnumber, int n)
{
  std::vector<int> neighbors;
  for(int i=1; i<=n; i++)
    {
      neighbors.push_back(runnumber-n);
      neighbors.push_back(runnumber+n);
    }
  return neighbors;
}
//____________________________________________________________________
int Fun4All_DriftAnalysis(
  const int nEvents = 10,
  const int nSkipEvents = 6000,
  const char* tag = "ana478_nocdbtag_v001",
  const int runnumber = 53686,
  const int segment = 49,
  const char* outputFile =  "./dst_eval-00053686-0000_corrected.root",
  const char* tpcResidualsFile = "./PHTpcResiduals-00053686-0000.root"

  )
{
  // print inputs
  std::cout << "Fun4All_CombinedDataReconstruction - nEvents: " << nEvents << std::endl;
  std::cout << "Fun4All_CombinedDataReconstruction - nSkipEvents: " << nSkipEvents << std::endl;
  std::cout << "Fun4All_CombinedDataReconstruction - tag: " << tag << std::endl;
  std::cout << "Fun4All_CombinedDataReconstruction - runnumber: " << runnumber << std::endl;
  std::cout << "Fun4All_CombinedDataReconstruction - segment: " << segment << std::endl;
  std::cout << "Fun4All_CombinedDataReconstruction - outputFile: " << outputFile << std::endl;
  std::cout << "Fun4All_CombinedDataReconstruction - tpcResidualsFile: " << tpcResidualsFile << std::endl;

  TRACKING::pp_mode = true;
  G4TRACKING::SC_CALIBMODE = false;
  G4TRACKING::convert_seeds_to_svtxtracks = false;

  // condition database
  Enable::CDB = true;

  // reco const
  auto rc = recoConsts::instance();
  rc->set_IntFlag("RUNNUMBER", runnumber);
  rc->set_IntFlag("RUNSEGMENT", segment);
  rc->set_StringFlag("CDB_GLOBALTAG", "ProdA_2024");
  rc->set_uint64Flag("TIMESTAMP", runnumber);


  
  // tpc readout initialization
  TpcReadoutInit( runnumber );

  std::string drift_calibration_csv = "collective_calibrations_28052025_dz_constrained.csv";
  std::map<int,float> drift_corrected = csv_to_map(drift_calibration_csv,4);
  std::map<int,float> drift_old = csv_to_map(drift_calibration_csv,3);
  
  int tpcunpacker_t0 = -4;
  G4TPC::tpc_drift_velocity_reco = drift_corrected[runnumber];
  if(drift_corrected[runnumber]>0.00770 || drift_corrected[runnumber]<0.00730)
    {
      //Go back to the previous iteration and maybe try higher statistics?
      G4TPC::tpc_drift_velocity_reco = drift_old[runnumber];
      if(drift_old[runnumber]>0.00770 || drift_old[runnumber]<0.00730)
	{
	  //If it's very weird values, try the usual 0.00747 value.
	  G4TPC::tpc_drift_velocity_reco = 0.00747;
	}
    }
  // reassign to ZCrossing correction
  TpcClusterZCrossingCorrection::_vdrift = G4TPC::tpc_drift_velocity_reco;
  
  // printout
  std::cout<< "Fun4All_CombinedDataReconstruction - samples: " << TRACKING::reco_tpc_maxtime_sample << std::endl;
  std::cout<< "Fun4All_CombinedDataReconstruction - pre: " << TRACKING::reco_tpc_time_presample << std::endl;
  std::cout<< "Fun4All_CombinedDataReconstruction - vdrift: " << G4TPC::tpc_drift_velocity_reco << std::endl;

  // server
  auto se = Fun4AllServer::instance();
  se->Verbosity(0);

  PHRandomSeed::Verbosity(0);

  // event counter
  se->registerSubsystem( new EventCounter_hp( "EventCounter_hp", 1 ) );

  // tracking geometry
  std::string geofile = CDBInterface::instance()->getUrl("Tracking_Geometry");
  std::cout << "Fun4All_CombinedDataReconstruction_hp - geofile: " << geofile << std::endl;

  auto ingeo = new Fun4AllRunNodeInputManager("GeoIn");
  ingeo->AddFile(geofile);
  se->registerInputManager(ingeo);

  // distortion correction
  if(true)
  {
    // module edge corrections
    G4TPC::ENABLE_MODULE_EDGE_CORRECTIONS = true;

    // static distortions
    G4TPC::ENABLE_STATIC_CORRECTIONS = true;
    
    //using the map function to write run numbers and paths.
    std::map<int, std::string> calib_paths = path_map("full_caliblist.txt", 1);
    
    //take the run numbers and pour them into a vector. i'm sure i could just use the map but I'm too tired to figure out how to.
    std::vector<int> available_calib;
    for(auto& [r,p] : calib_paths)
      {
	available_calib.push_back(r);
      }

    // average distortions
   
    if(std::find(available_calib.begin(), available_calib.end(), runnumber) != available_calib.end())
      {
	//if the lamination file exists for the run number, retrieve the default file.
	G4TPC::ENABLE_AVERAGE_CORRECTIONS = true;
	G4TPC::average_correction_filename = CDBInterface::instance()->getUrl("TPC_LAMINATION_FIT_CORRECTION");
      }
    else
      {
	//create a vector of numbers that are +/- n away from the run number. I wrote a function for this up top
	int n = 10;
	std::vector<int> neighbors = nearest_neighbor(runnumber, n);
	//now, look for the smallest distance calibrated run number from the original. The function i wrote automatically goes from smallest distance to the largest allowed.
        int key = -1;
	for (int neighbor : neighbors)
	  {
	    if (std::find(available_calib.begin(), available_calib.end(), neighbor) != available_calib.end())
	      {
		key = neighbor;
		break;
	      }
	  }

	if(key!=-1)
	  {
	    G4TPC::ENABLE_AVERAGE_CORRECTIONS = true;
	    G4TPC::average_correction_filename = calib_paths[key];
	  }
	else
	  {
	    G4TPC::ENABLE_AVERAGE_CORRECTIONS = false;
	  }
      } 
  }

  // tpc zero suppression
  TRACKING::tpc_zero_supp = true;
  Enable::MVTX_APPLYMISALIGNMENT = true;
  ACTSGEOM::mvtx_applymisalignment = Enable::MVTX_APPLYMISALIGNMENT;

  G4MAGNET::magfield_rescale = 1;
  TrackingInit();

  // input managers
  {
    const auto filelist = make_filelist( tag, runnumber, segment );
    for( size_t i = 0; i < filelist.size(); ++i )
    {
      const auto& inputfile = filelist[i];
      auto in = new Fun4AllDstInputManager(Form("DSTin_%zu", i));
      in->fileopen(inputfile);
      se->registerInputManager(in);
    }
  }
  std::cout << "Fun4All_CombinedDataReconstruction_new_hp - done with input managers" << std::endl;

  // hit unpackers
  for(int felix=0; felix < 6; felix++)
  { Mvtx_HitUnpacking(std::to_string(felix)); }

  for(int server = 0; server < 8; server++)
  { Intt_HitUnpacking(std::to_string(server)); }

  {
    // TPC unpacking
    for(int ebdc = 0; ebdc < 24; ebdc++)
    {
      const std::string ebdc_string = (Form( "%02i", ebdc ));

      auto tpcunpacker = new TpcCombinedRawDataUnpacker("TpcCombinedRawDataUnpacker"+ebdc_string);
      tpcunpacker->useRawHitNodeName("TPCRAWHIT_" + ebdc_string);
      tpcunpacker->set_presampleShift(TRACKING::reco_tpc_time_presample);

      if(TRACKING::tpc_zero_supp)
      { tpcunpacker->ReadZeroSuppressedData(); }

      tpcunpacker->doBaselineCorr(TRACKING::tpc_baseline_corr);
      se->registerSubsystem(tpcunpacker);
    }
  }

  std::cout << "Fun4All_CombinedDataReconstruction_new_hp - done with unpackers" << std::endl;

  {
    // micromegas unpacking
    auto tpotunpacker = new MicromegasCombinedDataDecoder;
    const auto calibrationFile = CDBInterface::instance()->getUrl("TPOT_Pedestal");
    tpotunpacker->set_calibration_file(calibrationFile);
    tpotunpacker->set_sample_max(1024);
    se->registerSubsystem(tpotunpacker);
  }
  std::cout << "Done with micromegas unpacking"<<std::endl;
  Mvtx_Clustering();
  std::cout << "Done with MVTX clustering"<<std::endl;
  Intt_Clustering();
  std::cout << "Done with INTT clustering"<<std::endl;

  // tpc clustering
  {
    auto tpcclusterizer = new TpcClusterizer;
    tpcclusterizer->Verbosity(0);
    tpcclusterizer->set_rawdata_reco();
    tpcclusterizer->set_sampa_tbias(0);
    se->registerSubsystem(tpcclusterizer);
  }

  Tpc_LaserEventIdentifying();
  std::cout << "Done with TPC laser event identifying"<<std::endl;
  Micromegas_Clustering();
  std::cout << "Done with micromegas clustering"<<std::endl;
  {
    // silicon seeding
    auto silicon_Seeding = new PHActsSiliconSeeding;
    silicon_Seeding->Verbosity(0);
    silicon_Seeding->setinttRPhiSearchWindow(1.0);
    silicon_Seeding->setinttZSearchWindow(7.0);
    silicon_Seeding->seedAnalysis(false);
    se->registerSubsystem(silicon_Seeding);
  }
  std::cout << "Done with silicon seeding"<<std::endl;
  {
    auto merger = new PHSiliconSeedMerger;
    merger->Verbosity(0);
    se->registerSubsystem(merger);
  }
  std::cout << "Done with Silicon Seed Merger"<<std::endl;
  double fieldstrength = std::numeric_limits<double>::quiet_NaN();
  const bool ConstField = isConstantField(G4MAGNET::magfield_tracking, fieldstrength);

  {
    // TPC seeding
    auto seeder = new PHCASeeding("PHCASeeding");
    if (ConstField)
    {
      seeder->useConstBField(true);
      seeder->constBField(fieldstrength);
    }
    else
    {
      seeder->set_field_dir(-1 * G4MAGNET::magfield_rescale);
      seeder->useConstBField(false);
      seeder->magFieldFile(G4MAGNET::magfield_tracking);  // to get charge sign right
    }
    seeder->Verbosity(0);
    seeder->SetLayerRange(7, 55);
    seeder->SetSearchWindow(2.,0.05); // z-width and phi-width, default in macro at 1.5 and 0.05
    seeder->SetClusAdd_delta_window(3.0,0.06); //  (0.5, 0.005) are default; sdzdr_cutoff, d2/dr2(phi)_cutoff
    seeder->SetMinHitsPerCluster(0);
    seeder->SetMinClustersPerTrack(3);
    seeder->useFixedClusterError(true);
    seeder->set_pp_mode(true);
    seeder->reject_zsize1_clusters(true);
    se->registerSubsystem(seeder);
  }
  std::cout << "Done with TPC seeding"<<std::endl;

  {
    // expand stubs in the TPC using simple kalman filter
    auto cprop = new PHSimpleKFProp("PHSimpleKFProp");
    cprop->set_field_dir(G4MAGNET::magfield_rescale);
    if (ConstField)
    {
      cprop->useConstBField(true);
      cprop->setConstBField(fieldstrength);
    }
    else
    {
      cprop->magFieldFile(G4MAGNET::magfield_tracking);
      cprop->set_field_dir(-1 * G4MAGNET::magfield_rescale);
    }
    cprop->useFixedClusterError(true);
    cprop->set_max_window(5.);
    cprop->Verbosity(0);
    cprop->set_pp_mode(true);
    se->registerSubsystem(cprop);
    std::cout << "Done with Kalman Filter Prop"<<std::endl;
    if (TRACKING::pp_mode)
    {
      // for pp mode, apply preliminary distortion corrections to TPC clusters before crossing is known
      // and refit the trackseeds. Replace KFProp fits with the new fit parameters in the TPC seeds.
      auto prelim_distcorr = new PrelimDistortionCorrection;
      std::cout << "Defined the prelimdistortioncorrection object "<<std::endl;
      prelim_distcorr->set_pp_mode(TRACKING::pp_mode);
      std::cout << "set pp mode for preliminary distortion corrections"<<std::endl;
      prelim_distcorr->Verbosity(0);
      se->registerSubsystem(prelim_distcorr);
      std::cout << "registered the distortion corrections to the server"<<std::endl;
    }

  }
  std::cout << "Done with applying preliminary distortion corrections for pp"<<std::endl;
  {
    // matching to silicons
    auto silicon_match = new PHSiliconTpcTrackMatching;
    silicon_match->Verbosity(0);

    // narrow matching windows
    silicon_match->set_x_search_window(0.36);
    silicon_match->set_y_search_window(0.36);
    silicon_match->set_z_search_window(2.5);
    silicon_match->set_phi_search_window(0.014);
    silicon_match->set_eta_search_window(0.0091);
    silicon_match->set_test_windows_printout(false);

    silicon_match->set_pp_mode(TRACKING::pp_mode);
    se->registerSubsystem(silicon_match);
  }
  std::cout << "Done with silicon track matching"<<std::endl;
  if( true )
  {
    // matching with micromegas
    auto mm_match = new PHMicromegasTpcTrackMatching;
    mm_match->Verbosity(0);
    mm_match->set_pp_mode(TRACKING::pp_mode);

    mm_match->set_rphi_search_window_lyr1(3.0);
    mm_match->set_rphi_search_window_lyr2(15.0);

    mm_match->set_z_search_window_lyr1(30.0);
    mm_match->set_z_search_window_lyr2(3.0);

    mm_match->set_min_tpc_layer(38);             // layer in TPC to start projection fit
    mm_match->set_test_windows_printout(false);  // used for tuning search windows only
    se->registerSubsystem(mm_match);
  }
  std::cout << "Done with micromegas track matching "<<std::endl;
  {
    // track fit
    se->registerSubsystem(new PHTpcDeltaZCorrection);

    // perform final track fit with ACTS
    auto actsFit = new PHActsTrkFitter;
    actsFit->Verbosity(0);
    actsFit->commissioning(G4TRACKING::use_alignment);

    // fit with Micromegas and Silicon ONLY
    actsFit->fitSiliconMMs(G4TRACKING::SC_CALIBMODE);
    actsFit->setUseMicromegas(G4TRACKING::SC_USE_MICROMEGAS);

    actsFit->set_pp_mode(TRACKING::pp_mode);
    actsFit->set_use_clustermover(true);
    actsFit->useActsEvaluator(false);
    actsFit->useOutlierFinder(false);
    actsFit->setFieldMap(G4MAGNET::magfield_tracking);
    se->registerSubsystem(actsFit);
  }

  {
    auto cleaner = new PHTrackCleaner();
    cleaner->Verbosity(0);
    se->registerSubsystem(cleaner);
  }

  {
    // vertex finding
    auto finder = new PHSimpleVertexFinder;
    finder->Verbosity(0);
    finder->setDcaCut(0.5);
    finder->setTrackPtCut(-99999.);
    finder->setBeamLineCut(1);
    finder->setTrackQualityCut(100);
    finder->setNmvtxRequired(3);
    finder->setOutlierPairCut(1);

    se->registerSubsystem(finder);
  }
  std::cout << "Done with vertex finding"<<std::endl;
  if (G4TRACKING::SC_CALIBMODE)
  {
    /*
    * in calibration mode, calculate residuals between TPC and fitted tracks,
    * store in dedicated structure for distortion correction
    */
    auto residuals = new PHTpcResiduals;
    residuals->setOutputfile(tpcResidualsFile);
    residuals->setUseMicromegas(G4TRACKING::SC_USE_MICROMEGAS);

    // matches Tony's analysis
    residuals->setMinPt( 0.2 );

    // reconstructed distortion grid size (phi, r, z)
    residuals->setGridDimensions(36, 48, 80);
    se->registerSubsystem(residuals);
  }

  std::cout << "Done with TPC calib mode"<<std::endl;
  // own evaluator
  if( true )
  {
    auto trackingEvaluator = new TrackingEvaluator_hp;
    trackingEvaluator->set_flags(
      TrackingEvaluator_hp::EvalEvent
      |TrackingEvaluator_hp::EvalTracks
//       |TrackingEvaluator_hp::MicromegasOnly
      );

    if( G4TRACKING::SC_CALIBMODE )
    { trackingEvaluator->set_trackmapname( "SvtxSiliconMMTrackMap" ); }

    se->registerSubsystem(trackingEvaluator);
  }
  std::cout << "Done with Hugo's tracking evaluator 1"<<std::endl;
  // own evaluator
  if( true )
  {

    auto micromegasTrackEvaluator = new MicromegasTrackEvaluator_hp;

    // silicon only extrapolation
    micromegasTrackEvaluator->set_use_silicon(false);

    if( G4TRACKING::SC_CALIBMODE )
    { micromegasTrackEvaluator->set_trackmapname( "SvtxSiliconMMTrackMap" ); }

    se->registerSubsystem(micromegasTrackEvaluator);
  }
  std::cout << "Done with Hugo's tracking evaluator 2"<<std::endl;
  // output manager
  if( true )
  {
    auto out = new Fun4AllDstOutputManager("DSTOUT", outputFile);
    out->AddNode("TrackingEvaluator_hp::Container");
    out->AddNode("MicromegasTrackEvaluator_hp::Container");
    se->registerOutputManager(out);
  }

  std::cout << "Registered MM nodes"<<std::endl;
  // process events
  if( nSkipEvents > 0 ) {
    se->skip(nSkipEvents);
  }
  std::cout << "Running events"<<std::endl;
  se->run(nEvents);
  
  // terminate
  se->End();
  se->PrintTimer();

  std::cout << "All done" << std::endl;
  delete se;
  gSystem->Exit(0);
  return 0;
}

// This function is only used to test if we can load this as root6 macro
// without running into unresolved libraries and include files
void RunFFALoadTest() {}
