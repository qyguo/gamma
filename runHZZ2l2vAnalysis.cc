#include <iostream>
#include <boost/shared_ptr.hpp>

#include "DataFormats/FWLite/interface/Handle.h"
#include "DataFormats/FWLite/interface/Event.h"
//#include "DataFormats/FWLite/interface/ChainEvent.h"
#include "DataFormats/Common/interface/MergeableCounter.h"


 #include "DataFormats/Common/interface/View.h"
 #include "DataFormats/Math/interface/deltaR.h"
 #include "DataFormats/Common/interface/RefToBase.h"
 #include "RecoEgamma/EgammaTools/interface/EcalRegressionData.h"
 #include "DataFormats/Common/interface/Ptr.h"

//Load here all the dataformat that we will need
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/Photon.h"

#include "DataFormats/EgammaCandidates/interface/Photon.h"


#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/PatCandidates/interface/Tau.h"
#include "DataFormats/PatCandidates/interface/PackedTriggerPrescales.h"
#include "DataFormats/PatCandidates/interface/GenericParticle.h"

#include "CondFormats/JetMETObjects/interface/JetResolution.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"

#include "FWCore/FWLite/interface/FWLiteEnabler.h"
#include "FWCore/PythonParameterSet/interface/MakeParameterSets.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "PhysicsTools/Utilities/interface/LumiReWeighting.h"
//#include "TauAnalysis/SVfitStandalone/interface/SVfitStandaloneAlgorithm.h" //for svfit

#include "EgammaAnalysis/ElectronTools/interface/ElectronEnergyCalibratorRun2.h"
#include "EgammaAnalysis/ElectronTools/interface/PhotonEnergyCalibratorRun2.h"
#include "EgammaAnalysis/ElectronTools/interface/EnergyScaleCorrection_class.h"

#include "ZZMatrixElement/MELA/interface/Mela.h"

#include "UserCode/llvv_fwk/interface/MacroUtils.h"
#include "UserCode/llvv_fwk/interface/HiggsUtils.h"
#include "UserCode/llvv_fwk/interface/SmartSelectionMonitor.h"
#include "UserCode/llvv_fwk/interface/TMVAUtils.h"
#include "UserCode/llvv_fwk/interface/LeptonEfficiencySF.h"
#include "UserCode/llvv_fwk/interface/PhotonEfficiencySF.h"
#include "UserCode/llvv_fwk/interface/PDFInfo.h"
#include "UserCode/llvv_fwk/interface/rochcor2015.h"
#include "UserCode/llvv_fwk/interface/rochcor2016.h"
#include "UserCode/llvv_fwk/interface/muresolution_run2.h"
#include "UserCode/llvv_fwk/interface/BTagCalibrationStandalone.h"
#include "UserCode/llvv_fwk/interface/BtagUncertaintyComputer.h"
#include "UserCode/llvv_fwk/interface/GammaWeightsHandler.h"

#include "UserCode/llvv_fwk/interface/PatUtils.h"
#include "UserCode/llvv_fwk/interface/TrigUtils.h"
#include "UserCode/llvv_fwk/interface/EwkCorrections.h"
#include "UserCode/llvv_fwk/interface/ZZatNNLO.h"

#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TEventList.h"
#include "TROOT.h"
#include "TNtuple.h"
#include "TLorentzVector.h"
#include <Math/VectorUtil.h>
#include "TRandom3.h"

#include <time.h>

using namespace std;
using namespace edm;


int main(int argc, char* argv[])
{

  //##############################################
  //########    GLOBAL INITIALIZATION     ########
  //##############################################

  // check arguments
  if(argc<2){ std::cout << "Usage : " << argv[0] << " parameters_cfg.py" << std::endl; exit(0); }

  // load framework libraries
  gSystem->Load( "libFWCoreFWLite" );
//  AutoLibraryLoader::enable();
  FWLiteEnabler::enable();


  // configure the process
  const edm::ParameterSet &runProcess = edm::readPSetsFrom(argv[1])->getParameter<edm::ParameterSet>("runProcess");

  bool isMC = runProcess.getParameter<bool>("isMC");
  double xsec = runProcess.getParameter<double>("xsec");
  double resonance = runProcess.getParameter<double>("resonance");
  int mctruthmode=runProcess.getParameter<int>("mctruthmode");
  bool photonTriggerStudy = runProcess.getParameter<bool>("triggerstudy");
  TString dtag=runProcess.getParameter<std::string>("dtag");

  TString suffix=runProcess.getParameter<std::string>("suffix");
  std::vector<std::string> urls=runProcess.getUntrackedParameter<std::vector<std::string> >("input");
  TString outUrl = runProcess.getParameter<std::string>("outfile");
  TString ReHLT_inMC= urls.at(0);

    

  //good lumi MASK
  lumiUtils::GoodLumiFilter goodLumiFilter(runProcess.getUntrackedParameter<std::vector<edm::LuminosityBlockRange> >("lumisToProcess", std::vector<edm::LuminosityBlockRange>()));

  bool filterOnlyEE(false), filterOnlyMUMU(false), filterOnlyEMU(false), filterOnlyPhoton(false), filterOnlyE(false), filterOnlyMU(false);
  if(!isMC){
      if(dtag.Contains("DoubleEle"))   filterOnlyEE=true;
      if(dtag.Contains("DoubleMu"))    filterOnlyMUMU=true;
      if(dtag.Contains("MuEG"))        filterOnlyEMU=true;
      if(dtag.Contains("SinglePhoton"))filterOnlyPhoton=true;
      if(dtag.Contains("SingleMu"))    filterOnlyMU=true;
      if(dtag.Contains("SingleElectron"))filterOnlyE=true;
  }
  bool isV0JetsMC(false);//isMC && (dtag.Contains("DYJetsToLL_50toInf") || dtag.Contains("_WJets")));  #FIXME should be reactivated as soon as we have exclusive jet samples
  bool isOldMC(isMC && dtag.Contains("old"));
  bool isWGmc(isMC && dtag.Contains("WG"));
  bool isZGmc(isMC && dtag.Contains("ZG"));
  bool isMC_GG  = isMC && ( string(dtag.Data()).find("GG" )  != string::npos);
  bool isMC_VBF = isMC && ( string(dtag.Data()).find("VBF")  != string::npos);
  bool isMC_125OnShell = isMC && (mctruthmode==521);
  if(isMC_125OnShell) mctruthmode=125;
  bool isMC_ZZ  = isMC && ( string(dtag.Data()).find("TeV_ZZ")  != string::npos);
  bool isMC_ZZ2l2nu  = isMC && ( string(dtag.Data()).find("TeV_ZZ2l2nu")  != string::npos);
  bool isMC_ggZZ2mu2nu = isMC && (string(dtag.Data()).find("TeV_ggZZ2mu2nu") != string::npos);
  bool isMC_ggZZ2e2nu = isMC && (string(dtag.Data()).find("TeV_ggZZ2e2nu") != string::npos);
  bool isMC_WZ  = isMC && ( string(dtag.Data()).find("TeV_WZ")  != string::npos);
  bool isMC_WZ3lnu  = isMC && ( string(dtag.Data()).find("TeV_WZ3lnu")  != string::npos);
  bool isMC_Wlnu_inclusive = (isMC && dtag.Contains("_WJets_") && !dtag.Contains("HT"));
  bool isMC_Wlnu_HT100 = (isMC && dtag.Contains("_WJets_HT-") );

  bool isMC_WGToLNuG = (isMC && dtag.Contains("WGToLNuG") );      
  bool isMC_ZNuNuGJets = (isMC && dtag.Contains("ZNuNuGJets")); 
  bool isMC_ZJetsToNuNu = (isMC && dtag.Contains("ZJetsToNuNu")); 

  bool isMC_QCD = (isMC && dtag.Contains("QCD"));
  bool isMC_GJet = (isMC && dtag.Contains("GJet"));
  bool is2015data = (!isMC && dtag.Contains("2015"));
  bool is2015MC = (isMC && dtag.Contains("2015"));
  bool is2016data = (!isMC && dtag.Contains("2016"));
  bool is2016MC = (isMC && dtag.Contains("2016"));
  bool isMC_signal  = isMC && ( (string(dtag.Data()).find("GG" )  != string::npos) ||(string(dtag.Data()).find("VBF")  != string::npos )||dtag.Contains("RsGrav")||dtag.Contains("BulkGrav") || dtag.Contains("Radion") );
  bool isMELA = isMC_signal && ( dtag.Contains("MELA") );
  bool isReHLT = isMC && ( ReHLT_inMC.Contains("reHLT") );

  //MELA reweighting procedure
  if(isMELA) printf("MELA reweighting activated \n");
  printf("MELA: initialization \n");
  Mela mela( 13, 600, TVar::INFO); //TVar::DEBUG
  string MelaMode;

  //Initialize Mela Mode
  if( dtag.Contains("SOnly") ){ MelaMode = "Sigh2"; }
  else if( dtag.Contains("BOnly") ){ MelaMode = "Bckg"; }
  else if( dtag.Contains("SandBandInterf") ){ MelaMode = "All"; }
  else if( dtag.Contains("Sigh1") ){ MelaMode = "Sigh1"; }
  else if( dtag.Contains("ContinuumOnly") ){ MelaMode = "Continuum"; }
  else if( dtag.Contains("Interf") && dtag.Contains("h2") && dtag.Contains("Continuum") ){ MelaMode = "Interf_h2Continuum"; }
  else if( dtag.Contains("Interf") && dtag.Contains("h2") && dtag.Contains("h1") ){ MelaMode = "Interf_h2h1"; }
  else if( dtag.Contains("Interf") && dtag.Contains("Continuum") && dtag.Contains("h1") ){ MelaMode = "Interf_h1Continuum"; }
  else if( dtag.Contains("Interf") && dtag.Contains("Full") ){ MelaMode = "Interf_Full"; }

  //tree info
  TString dirname = runProcess.getParameter<std::string>("dirName");

  //systematics
  bool runSystematics                        = runProcess.getParameter<bool>("runSystematics");
  std::vector<TString> varNames(1,"");

  std::vector<string> jetVarNames = {"", "_scale_jup","_scale_jdown", "_res_jup", "_res_jdown"};

  std::vector<string> eleVarNames = {""};

  if(runSystematics){
	eleVarNames.push_back("_stat_eup");
	eleVarNames.push_back("_stat_edown");
	eleVarNames.push_back("_sys_eup");
	eleVarNames.push_back("_sys_edown");
	eleVarNames.push_back("_GS_eup");
	eleVarNames.push_back("_GS_edown");
	eleVarNames.push_back("_resRho_eup");
	eleVarNames.push_back("_resRho_edown");
	eleVarNames.push_back("_resPhi_edown");
     if(true){
        varNames.push_back("_scale_umetup"); varNames.push_back("_scale_umetdown");    //unclustered met
        varNames.push_back("_res_jup");      varNames.push_back("_res_jdown");    //jet energy resolution
        varNames.push_back("_scale_jup");    varNames.push_back("_scale_jdown");  //jet energy scale
        varNames.push_back("_scale_mup");    varNames.push_back("_scale_mdown");  //muon energy scale
        varNames.push_back("_stat_eup");    varNames.push_back("_stat_edown");  //electron energy scale
        varNames.push_back("_sys_eup");    varNames.push_back("_sys_edown");  //electron energy scale
        varNames.push_back("_GS_eup");    varNames.push_back("_GS_edown");  //electron energy scale
        varNames.push_back("_resRho_eup");    varNames.push_back("_resRho_edown");  //electron energy resolution
        varNames.push_back("_resPhi_edown");   					  //electron energy resolution
        varNames.push_back("_puup");         varNames.push_back("_pudown");      //pileup uncertainty
        varNames.push_back("_eff_bup");      varNames.push_back("_eff_bdown");    //btag veto
        varNames.push_back("_lepveto");                                           //3rd lepton veto
        varNames.push_back("_th_pdf");                                           //pdf
        varNames.push_back("_th_alphas");                                         //alpha_s (QCD)
     }
     /*if(isMC_GG || isMC_VBF){
        varNames.push_back("_signal_lshapeup"); varNames.push_back("_signal_lshapedown"); //signal line shape (NNLO + interf)
        varNames.push_back("_signal_normup"); varNames.push_back("_signal_normdown"); //signal scale      (NNLO + interf)
     }*/
     if(isMC_ZZ || isMC_WZ){
        varNames.push_back("_th_ewkup"); varNames.push_back("_th_ewkdown"); //EWK+QCD corrections
     }
  }
  size_t nvarsToInclude=varNames.size();

  std::vector<std::string> allWeightsURL=runProcess.getParameter<std::vector<std::string> >("weightsFile");
  std::string weightsDir( allWeightsURL.size() ? allWeightsURL[0] : "");

  std::vector<std::string> gammaPtWeightsFiles =  runProcess.getParameter<std::vector<std::string> >("weightsFile");
  GammaWeightsHandler* gammaWgtHandler = (gammaPtWeightsFiles.size()>0 && gammaPtWeightsFiles[0]!="") ? new GammaWeightsHandler(runProcess,"",true) : NULL;

  // Apply pu corrections to photon sample, to match the pu distribution in the dilepton one
  std::vector<std::string> puWeightsFilePath = runProcess.getParameter<std::vector<std::string> >("puWeightsFile");

  bool doPUCorrections=true;
  if ( puWeightsFilePath.size()==0) {doPUCorrections=false; }
  else if ( puWeightsFilePath[0]=="") {doPUCorrections=false; }

  TFile* puWeightsFile=NULL;

  if(gammaWgtHandler)printf("gammaWgtHandler is activated\n");

  //HIGGS weights and uncertainties

  //narrow resonance
  std::vector<std::pair<double, double> > NRparams;
  if(mctruthmode==125){
//    NRparams.push_back(std::make_pair<double,double>(5, -1));  //vary the width
//    NRparams.push_back(std::make_pair<double,double>(8, -1));
//    NRparams.push_back(std::make_pair<double,double>(10,-1));
//    NRparams.push_back(std::make_pair<double,double>(11,-1));
//    NRparams.push_back(std::make_pair<double,double>(12,-1));
//    NRparams.push_back(std::make_pair<double,double>(13,-1));
//    NRparams.push_back(std::make_pair<double,double>(14,-1));
//    NRparams.push_back(std::make_pair<double,double>(15,-1));
//    NRparams.push_back(std::make_pair<double,double>(16,-1));
//    NRparams.push_back(std::make_pair<double,double>(17,-1));
//    NRparams.push_back(std::make_pair<double,double>(18,-1));
//    NRparams.push_back(std::make_pair<double,double>(19,-1));
//    NRparams.push_back(std::make_pair<double,double>(20,-1));
//    NRparams.push_back(std::make_pair<double,double>(22,-1));
//    NRparams.push_back(std::make_pair<double,double>(25,-1));
//    NRparams.push_back(std::make_pair<double,double>(30,-1));
  }else if( (suffix=="" || isMELA)  && (isMC_GG || isMC_VBF)){ //consider the other points only when no suffix is being used

      //Fixed Width
      NRparams.push_back(std::make_pair<double,double>( 100.0, 0.0) );
      NRparams.push_back(std::make_pair<double,double>(  10.0, 0.0) );
      NRparams.push_back(std::make_pair<double,double>(   5.0, 0.0) );

      //Narrow Resonance
      //NRparams.push_back(std::make_pair<double,double>(1.0, 0.0) ); //cp, brnew
      //NRparams.push_back(std::make_pair<double,double>(0.6, 0.0) ); //cp, brnew
      //NRparams.push_back(std::make_pair<double,double>(0.3, 0.0) ); //cp, brnew
      //NRparams.push_back(std::make_pair<double,double>(0.1, 0.0) ); //cp, brnew
  }
  if(NRparams.size()<=0)NRparams.push_back(std::make_pair<double,double>(-1.0, -1.0)); //no reweighting

  std::vector<TString>    NRsuffix;
  std::vector<double[6] > lShapeWeights(NRparams.size());   //WEIGHT for LineShape (NNLO kFactors + Interf), split in shape and scale unc with the following format:  scaleNominal, shapeNominal, scaleDown, shapeDown, scaleUp, shapeUp
  std::vector< std::map<TString,double> > lMelaShapeWeights(NRparams.size());
  std::map< TString, double> MelaWeigthsMap;

  for(unsigned int nri=0;nri<NRparams.size();nri++){
     char tmp[255];
     if(      NRparams[nri].first<0 && NRparams[nri].second<0){   sprintf(tmp,"%s", "");
     }else if(NRparams[nri].first>0 && NRparams[nri].second<0){   sprintf(tmp,"_width%6.2f"      , NRparams[nri].first);
     }else{                                                       sprintf(tmp,"_cp%3.2f_brn%3.2f",NRparams[nri].first, NRparams[nri].second);
     }
     NRsuffix.push_back(TString(tmp));
  }


  //STANDARD MODEL
  double HiggsMass=0; string VBFString = ""; string GGString("");
  TF1 *decayProbPdf=new TF1("relbw","(2*sqrt(2)*[0]*[1]*sqrt(pow([0],2)*(pow([0],2)+pow([1],2)))/(TMath::Pi()*sqrt(pow([0],2)+sqrt(pow([0],2)*(pow([0],2)+pow([1],2))))))/(pow(pow(x,2)-pow([0],2),2)+pow([0]*[1],2))",0,2000);
  if(isMC_GG){
    size_t GGStringpos =  string(dtag.Data()).find("GG");
    string StringMass = string(dtag.Data()).substr(GGStringpos+5,4);  sscanf(StringMass.c_str(),"%lf",&HiggsMass);
    GGString = string(dtag.Data()).substr(GGStringpos);
  }else if(isMC_VBF){
    size_t VBFStringpos =  string(dtag.Data()).find("VBF");
    string StringMass = string(dtag.Data()).substr(VBFStringpos+6,4);  sscanf(StringMass.c_str(),"%lf",&HiggsMass);
    VBFString = string(dtag.Data()).substr(VBFStringpos);
  }
  if(mctruthmode==125) HiggsMass=124;


  //ELECTROWEAK CORRECTION WEIGHTS
  std::vector<std::vector<float>> ewkTable, ZZ_NNLOTable;
  if(isMC_ZZ2l2nu){
  	ewkTable = EwkCorrections::readFile_and_loadEwkTable(dtag);
		ZZ_NNLOTable = ZZatNNLO::readFile_and_loadTable(dtag);
	}
  if(isMC_WZ3lnu){
  	ewkTable = EwkCorrections::readFile_and_loadEwkTable(dtag);
	}

  //NNLO Correction for ggHZZ and ggZZ processes. They are computed as a function of the invariant mass of the final state using HNNLO.
  double nnlo_ggH_kFact=1.0;
  TGraph* HNNLO_Wgt_Gr = higgs::utils::Get_NNLO_kFactors();


  //#######################################
  //####      LINE SHAPE WEIGHTS       ####
  //#######################################
  std::map<std::pair<double,double>, std::vector<std::pair<double, TGraph *> > > hLineShapeGrVec;
  if(isMC_GG || isMC_VBF){
      TH1D* hGen=new TH1D("hGen", "hGen", 2000, 0, 8000);
      utils::getHiggsLineshapeFromMiniAOD(urls, hGen);
      printf("hGen integral = %f\n", hGen->Integral());

      TGraph* hLineShapeNominal= new TGraph(hGen);
      TFile* nrLineShapesFile=NULL;
      if(mctruthmode==125){
	TString nrLineShapesFileUrl(weightsDir+"/higgs_width_zz2l2nu.root");
	gSystem->ExpandPathName(nrLineShapesFileUrl);
	nrLineShapesFile=TFile::Open(nrLineShapesFileUrl);
      } else if( isMC_GG ){
	//TString nrLineShapesFileUrl(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/NR_weightsRun2.root");
	TString nrLineShapesFileUrl(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/Weights_EWS_GGH_21June2016_AllInterferences.root");
	//Weights_EWS_GGH_21June2016_AllInterferences.root");
	gSystem->ExpandPathName(nrLineShapesFileUrl);
	nrLineShapesFile=TFile::Open(nrLineShapesFileUrl);
      } else if( isMC_VBF ){
        TString nrLineShapesFileUrl(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/Weights_EWS_VBF_21June2016_AllInterferences.root");
	//Weights_EWS_VBF_21June2016_AllInterferences.root");
        gSystem->ExpandPathName(nrLineShapesFileUrl);
        nrLineShapesFile=TFile::Open(nrLineShapesFileUrl);
      }

      bool isGGZZContinuum = (dtag.Contains("ggZZ"));

      //loop over possible scenarios: SM or BSM
      for(size_t nri=0; nri<NRparams.size(); nri++){
         //recompute weights depending on the scenario (SM or BSM)
         TGraph* shapeWgtsGr      = new TGraph; shapeWgtsGr->SetName("shapeWgts_"+ NRsuffix[nri]);          float shapeNorm(0);     double signalNorm(1.0);
         TGraph* shapeWgts_upGr   = new TGraph; shapeWgts_upGr->SetName("shapeWgtsUp_"+ NRsuffix[nri]);     float shapeUpNorm(0);   double signalUpNorm(1.0);
         TGraph* shapeWgts_downGr = new TGraph; shapeWgts_downGr->SetName("shapeWgtsDown_"+ NRsuffix[nri]); float shapeDownNorm(0); double signalDownNorm(1.0);

         TGraph* nrWgtGr=NULL; TGraph* nrWgtUpGr=NULL; TGraph* nrWgtDownGr=NULL;
         if(isGGZZContinuum){
            nrWgtGr     = higgs::utils::weightGGZZContinuum(nrLineShapesFile,signalNorm    ,""  );
            nrWgtUpGr   = higgs::utils::weightGGZZContinuum(nrLineShapesFile,signalUpNorm  ,"Up");
            nrWgtDownGr = higgs::utils::weightGGZZContinuum(nrLineShapesFile,signalDownNorm,"Dn");
         }else if(mctruthmode!=125 && NRparams[nri].first>=0){
            nrWgtGr     = higgs::utils::weightNarrowResonnance(isMC_VBF,HiggsMass, NRparams[nri].first, NRparams[nri].second, nrLineShapesFile,signalNorm    ,""     );
            nrWgtUpGr   = higgs::utils::weightNarrowResonnance(isMC_VBF,HiggsMass, NRparams[nri].first, NRparams[nri].second, nrLineShapesFile,signalUpNorm  ,"_up"  );
            nrWgtDownGr = higgs::utils::weightNarrowResonnance(isMC_VBF,HiggsMass, NRparams[nri].first, NRparams[nri].second, nrLineShapesFile,signalDownNorm,"_down");
         }
         if(!nrWgtUpGr){nrWgtUpGr=nrWgtGr;  signalUpNorm=signalNorm;}
         if(!nrWgtDownGr){nrWgtDownGr=nrWgtGr;  signalDownNorm=signalNorm;}

         double hySum=0;
         for(int ip=1; ip<=hGen->GetXaxis()->GetNbins(); ip++){
    	     Double_t hmass    = hGen->GetBinCenter(ip);
	     Double_t hy       = hGen->GetBinContent(ip);
             hySum            += hy;

	     Double_t shapeWgt(1.0),shapeWgtUp(1.0),shapeWgtDown(1.0);
             if(mctruthmode==125){  //reweighting to SM higgs width various width
                TString var("");
   	        if(dtag.Contains("ScaleUp"))   var="up";
	        if(dtag.Contains("ScaleDown")) var="down";
  	        shapeWgt       = higgs::utils::weightToH125Interference(hmass,NRparams[nri].first,nrLineShapesFile,var);
	        shapeWgtUp     = shapeWgt;
	        shapeWgtDown   = shapeWgt;
 	     }else if(NRparams[nri].first>=0 || isGGZZContinuum){ //reweighting to Narrow reasonnance or EWS model, or to ggZZ continuum
           //     shapeWgt       = nrWgtGr    ->Eval(hmass);
           //     shapeWgtUp     = nrWgtUpGr  ->Eval(hmass);
           //     shapeWgtDown   = nrWgtDownGr->Eval(hmass);
                shapeWgt     = 1.0;
                shapeWgtUp   = 1.0;
                shapeWgtDown = 1.0;
             }else{ //unknown case, do not reweight  (SM-Like)
     	        shapeWgt     = 1.0;
	        shapeWgtUp   = 1.0;
 	        shapeWgtDown = 1.0;
  	     }

             //if(ip==150)printf("weight for mZZ %f = %f x %f\n", hmass, shapeWgt, hy);
	     shapeWgtsGr     ->SetPoint(shapeWgtsGr     ->GetN(), hmass, shapeWgt);       shapeNorm     += shapeWgt*hy;
	     shapeWgts_upGr  ->SetPoint(shapeWgts_upGr  ->GetN(), hmass, shapeWgtUp);     shapeUpNorm   += shapeWgtUp*hy;
	     shapeWgts_downGr->SetPoint(shapeWgts_downGr->GetN(), hmass, shapeWgtDown);   shapeDownNorm += shapeWgtDown*hy;
         }

         if(mctruthmode!=125){
	    if(hySum>0){
	       shapeNorm     /= hySum;
	       shapeUpNorm   /= hySum;
	       shapeDownNorm /= hySum;
     	    }

            //fix possible normalization issues
            //printf("C'=%6.2f  BRNew=%6.2f shapeNorm = %f Up=%f Down=%f  signalNorm=%f Up=%f Down=%f\n", NRparams[nri].first, NRparams[nri].second, shapeNorm, shapeUpNorm, shapeDownNorm, signalNorm, signalUpNorm, signalDownNorm);
 	    for(Int_t ip=0; ip<shapeWgtsGr->GetN(); ip++){
  	         Double_t x,y;
	         shapeWgtsGr->GetPoint(ip,x,y);
	         shapeWgtsGr->SetPoint(ip,x,y/shapeNorm);
 	         shapeWgts_upGr->GetPoint(ip,x,y);
	         shapeWgts_upGr->SetPoint(ip,x,y/shapeUpNorm);
  	         shapeWgts_downGr->GetPoint(ip,x,y);
	         shapeWgts_downGr->SetPoint(ip,x,y/shapeDownNorm);
	    }
         }

         //all done here...
	 std::vector<std::pair<double, TGraph*> > inrWgts = {std::make_pair(signalNorm/signalNorm, shapeWgtsGr), std::make_pair(signalUpNorm/signalNorm,shapeWgts_upGr), std::make_pair(signalDownNorm/signalNorm,shapeWgts_downGr)};
	 hLineShapeGrVec[ NRparams[nri] ] = inrWgts;
      }
      if(nrLineShapesFile){nrLineShapesFile->Close(); delete nrLineShapesFile;}
  }


  //##############################################
  //########    INITIATING HISTOGRAMS     ########
  //##############################################
  SmartSelectionMonitor mon;
  printf("Definition of plots");

  //generator level control : add an underflow entry to make sure the histo is kept
  ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_raw",      ";Higgs Mass [GeV];Events", 2000, 0, 5000) ))->Fill(-1.0,0.0001);
  ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_nnlo_raw", ";Higgs Mass [GeV];Events", 2000, 0, 5000) ))->Fill(-1.0,0.0001);
  for(unsigned int nri=0;nri<NRparams.size();nri++){
    ((TH1F*)mon.addHistogram( new TH1F( "MELA_weights"+NRsuffix[nri] , ";Wgt; Events", 10000, 0, 100)))->Fill(-1.0,0.0001);
    ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_shape"+NRsuffix[nri] , ";Higgs Mass;Events [GeV]", 2000, 0, 5000) ))->Fill(-1.0,0.0001);
    ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_shape&scale"+NRsuffix[nri] , ";Higgs Mass;Events [GeV]", 2000, 0, 5000) ))->Fill(-1.0,0.0001);
  }

  //mon.addHistogram( new TH1F( "wdecays",     ";W decay channel",5,-0.5,4.5) );
  //mon.addHistogram( new TH1F( "zdecays",     ";Z decay channel",6,-0.5,5.5) );
  TH1F *hb=(TH1F*) mon.addHistogram( new TH1F( "wdecays",     ";W decay channel",5,0,5) );
  hb->GetXaxis()->SetBinLabel(1,"None");
  hb->GetXaxis()->SetBinLabel(2,"e+#nu");
  hb->GetXaxis()->SetBinLabel(3,"#mu+#nu");
  hb->GetXaxis()->SetBinLabel(4,"#tau+#nu");
  hb->GetXaxis()->SetBinLabel(5,"Njets #\geq 2");
 
 

    
    
    
  hb=(TH1F*) mon.addHistogram( new TH1F( "zdecays",     ";Z decay channel",6,0,6) );
  hb->GetXaxis()->SetBinLabel(1,"None");
  hb->GetXaxis()->SetBinLabel(2,"e+e");
  hb->GetXaxis()->SetBinLabel(3,"#mu+#mu");
  hb->GetXaxis()->SetBinLabel(4,"#tau+#tau");
  hb->GetXaxis()->SetBinLabel(5,"Njets #\geq 2");
  hb->GetXaxis()->SetBinLabel(6,"#nu+#nu");
    

    
    
  //  mon.addHistogram( new TH1F( "metFilter_eventflow",     ";metEventflow",20,0,20) );
  TH1F *hm=(TH1F*) mon.addHistogram( new TH1F( "metFilter_eventflow",     ";metEventflow",20,0,20) );
  hm->GetXaxis()->SetBinLabel(1,"raw");
  hm->GetXaxis()->SetBinLabel(2,"globalTightHalo2016Filter");
  hm->GetXaxis()->SetBinLabel(3,"goodVertices");
  hm->GetXaxis()->SetBinLabel(4,"eeBadScFilter");
  hm->GetXaxis()->SetBinLabel(5,"EcalDeadCellTriggerPrimitiveFilter");
  hm->GetXaxis()->SetBinLabel(6,"HBHENoiseFilter");
  hm->GetXaxis()->SetBinLabel(7,"HBHENoiseIsoFilter");
  hm->GetXaxis()->SetBinLabel(8,"BadPFMuonFilter");
  hm->GetXaxis()->SetBinLabel(9,"BadChargedCandidateFilte");
  hm->GetXaxis()->SetBinLabel(10,"badMuonHIPFilter");
  hm->GetXaxis()->SetBinLabel(11,"duplicateMuonHIPFilter");

  //event selection
  TH1F *h=(TH1F*) mon.addHistogram( new TH1F ("eventflow", ";;Events", 10,0,10) );
  h->GetXaxis()->SetBinLabel(1,"raw");
  h->GetXaxis()->SetBinLabel(2,"#geq 2 iso leptons");
  h->GetXaxis()->SetBinLabel(3,"|M-91|<15");
  h->GetXaxis()->SetBinLabel(4,"p_{T}>55");
  h->GetXaxis()->SetBinLabel(5,"3^{rd}-lepton veto");
  h->GetXaxis()->SetBinLabel(6,"b-veto");
  h->GetXaxis()->SetBinLabel(7,"#Delta #phi(jet,E_{T}^{miss})>0.5");
  h->GetXaxis()->SetBinLabel(8,"#Delta #phi(Z/#gamma,E_{T}^{miss})>0.5");
  h->GetXaxis()->SetBinLabel(9,"E_{T}^{miss}>80");
  h->GetXaxis()->SetBinLabel(10,"E_{T}^{miss}>125");


  h=(TH1F*) mon.addHistogram( new TH1F ("trigger", ";;Events", 10,0,10) );
  h->GetXaxis()->SetBinLabel(1,"#mu#mu");
  h->GetXaxis()->SetBinLabel(2,"#mu");
  h->GetXaxis()->SetBinLabel(3,"ee");
  h->GetXaxis()->SetBinLabel(4,"e");
  h->GetXaxis()->SetBinLabel(5,"e#mu");
  h->GetXaxis()->SetBinLabel(6,"#gamma");

  //pu control
  mon.addHistogram( new TH1F( "nvtx",";Vertices;Events",101, -0.5, 100.5)); //50,0,50) );
  mon.addHistogram( new TH1F( "nvtxraw",";Vertices;Events",101, -0.5, 100.5 )); //50,0,50) );
  mon.addHistogram( new TH1F( "rho",";#rho;Events",100,0,50) );

  Double_t ZPtBins[] = {0,30,36,50,75,90,120,165,3000};
  Int_t NZPtBins = sizeof(ZPtBins)/sizeof(ZPtBins[0]) - 1;
  Double_t RhoBins[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100};

  Int_t NRhoBins = sizeof(RhoBins)/sizeof(RhoBins[0]) - 1;

  TH2F *hnvtx=(TH2F *) mon.addHistogram( new TH2F ("zpt_vs_nvtx",";zpt;#vertices",NZPtBins, ZPtBins, NRhoBins, RhoBins) );
  TH2F *hrho=(TH2F *) mon.addHistogram( new TH2F ("zpt_vs_rho",";zpt;rho", NZPtBins, ZPtBins, NRhoBins, RhoBins) );

    
    TH1F *ha=(TH1F*) mon.addHistogram( new TH1F ("check_pass_condition", ";;Events", 10,0,10) );
    ha->GetXaxis()->SetBinLabel(1,"#gamma#gamma");
    ha->GetXaxis()->SetBinLabel(2,"#mu#mu");
    ha->GetXaxis()->SetBinLabel(3,"ee");
    //ha->GetXaxis()->SetBinLabel(4,"e");
    //ha->GetXaxis()->SetBinLabel(5,"e#mu");
    //ha->GetXaxis()->SetBinLabel(6,"#gamma");
  
    
    
  // photon control
  mon.addHistogram(new TH1F("npho",   ";Number of Photons;Events", 20, 0, 20) );
  mon.addHistogram(new TH1F("npho55", ";Number of Photons;Events", 20, 0, 20) );
  mon.addHistogram(new TH1F("npho100",";Number of Photons;Events", 20, 0, 20) );
  mon.addHistogram(new TH1F("photonpt", ";Photon pT [GeV];Events", 500, 0, 1000) );
  mon.addHistogram(new TH1F("phopt", ";Photon pT [GeV];Events", 500, 0, 1000) );
  mon.addHistogram(new TH1F("phoeta", ";Photon pseudo-rapidity;Events", 50, 0, 5) );
  mon.addHistogram(new TH1F("bosonnvtx", ";Photon #vertices;Events", 101, -0.5, 100.5) );
  mon.addHistogram(new TH1F("bosonrho", ";Photon rho;Events",100,0,50) );

    
  mon.addHistogram(new TH1F("bosoneta", ";Photon #eta;Events", 100, -5, 5) );
  mon.addHistogram(new TH1F("bosonphi", ";Photon #phi;Events", 80, -4, 4) );
  mon.addHistogram(new TH1F("bosonphiHG", ";Photon #phi;Events", 800, -4, 4) );
  mon.addHistogram(new TH1F("metphi", ";MET #phi;Events", 80, -4, 4) );
  mon.addHistogram(new TH1F("metphiUnCor", ";MET #phi;Events", 80, -4, 4) );
  mon.addHistogram(new TH1F("dphi_boson_met", ";#Delta #phi(Z/#gamma,MET);Events", 40, 0, 4) );
  mon.addHistogram(new TH1F("dphi_boson_met125", ";#Delta #phi(Z/#gamma,MET);Events", 40, 0, 4) );
   
/*
// PhotonID Histograms
  mon.addHistogram(new TH1F("photonEcalIso",          "Ecal Rec Hit Isolation", 300, 0, 300));
  mon.addHistogram(new TH1F("photonHcalIso",          "Hcal Rec Hit Isolation", 300, 0, 300));
  mon.addHistogram(new TH1F("photonTrackSolidIso",    "Sum of track pT in a cone of #DeltaR" , 300, 0, 300));
  mon.addHistogram(new TH1F("photonTrackHollowIso",   "Sum of track pT in a hollow cone" ,     300, 0, 300));
  mon.addHistogram(new TH1F("photonTrackCountSolid",  "Number of tracks in a cone of #DeltaR", 100, 0, 100));
  mon.addHistogram(new TH1F("photonTrackCountHollow", "Number of tracks in a hollow cone",     100, 0, 100));
  mon.addHistogram(new TH1F("photonInEBEtagap",          "Ecal Barrel eta gap flag",  2, -0.5, 1.5));
  mon.addHistogram(new TH1F("photonInEBPhigap",          "Ecal Barrel phi gap flag",  2, -0.5, 1.5));
  mon.addHistogram(new TH1F("photonInEERinggap",          "Ecal Endcap ring gap flag",  2, -0.5, 1.5));
  mon.addHistogram(new TH1F("photonInEEDeegap",          "Ecal Endcap dee gap flag",  2, -0.5, 1.5));
  mon.addHistogram(new TH1F("photonInEEgap",          "Ecal Barrel/Endcap gap flag",  2, -0.5, 1.5));
  mon.addHistogram(new TH1F("photonR9",               "R9 = E(3x3) / E(SuperCluster)", 300, 0, 3));
                                           
// Photon Histograms
  mon.addHistogram(new TH1F("photonEt",     "Photon E_{T}",  200,  0, 200));
  mon.addHistogram(new TH1F("photonEta",    "Photon #eta",   800, -4,   4));
  mon.addHistogram(new TH1F("photonPhi",    "Photon #phi",   628, -1.*TMath::Pi(), TMath::Pi()));
  mon.addHistogram(new TH1F("photonHoverE", "Hadronic over EM", 200, 0, 1));
    
// Photon's SuperCluster Histograms
  mon.addHistogram(new TH1F("photonScEt",  "Photon SuperCluster E_{T}", 200,  0, 200));
  mon.addHistogram(new TH1F("photonScEta", "Photon #eta",               800, -4,   4));
  mon.addHistogram(new TH1F("photonScPhi", "Photon #phi",628, -1.*TMath::Pi(), TMath::Pi()));
  mon.addHistogram(new TH1F("photonScEtaWidth","#eta-width",            100,  0,  .1));
    
    // Composite or Other Histograms
    //  mon.addHistogram(new TH1F("photonInAnyGap",     "Photon in any gap flag",  2, -0.5, 1.5);
    //  mon.addHistogram(new TH1F("photonLoosePhoton", "Total number photons (0=NotPassing, 1=Passing)", 2, -0.5, 1.5));
    //  mon.addHistogram(new TH1F("photonLooseEM", "Total number photons (0=NotPassing, 1=Passing)", 2, -0.5, 1.5));
    //  mon.addHistogram(new TH1F("photonCount",        "Number of photons passing cuts in event",  10,  0,  10));
    
*/
    //mon.addHistogram( new TH1F( "wdecays",     ";W decay channel",5,0,5) );

    
    mon.addHistogram(new TH1F("check_gen_size",";;Events",201,-0.5,200.5));
    mon.addHistogram(new TH1F("Status_WZH",   "; ; Events", 11 ,-0.5,10.5));
    mon.addHistogram(new TH1F("DeltaEta_VH",      "; #Delta #eta [GeV]; Events", 50, -1.*TMath::Pi(), TMath::Pi()) );
    mon.addHistogram(new TH1F("PT_VH",        "; Pt of VH [GeV]; Events", 50, 0, 500) );
    mon.addHistogram(new TH1F("Mass_VH",      "; Mass of VH [GeV]; Events", 50, 0, 500) );
    mon.addHistogram(new TH1F("Et_VH",        "; E_{T} of VH [GeV]; Events", 50, 0, 500) );
    mon.addHistogram(new TH1F("Eta_VH",       "; #eta of VH [GeV]; Events", 50, -5, 5) );
    mon.addHistogram(new TH1F("Rapidity_VH",      "; Rapidity of VH [GeV]; Events", 50, -5, 5) );
    mon.addHistogram(new TH1F("ColinearRapidity_VH",      "; ColinearRapidity of VH [GeV]; Events", 50, -5, 5) );
    
    mon.addHistogram(new TH1F("CosTheta_VH",      "Cos(#Theta); [GeV]; Events", 20, -1, 1) );
    mon.addHistogram(new TH1F("DeltaR_VH",      "#Delta R; [GeV]; Events", 50, 0, 10) );
    mon.addHistogram(new TH1F("DeltaPhi_VH",      "#Delta #phi; [GeV]; Events",  50, -1.*TMath::Pi(), TMath::Pi()) );
    
    
    

    mon.addHistogram(new TH1F("Pass_deltaR",      ";Whether pass #DeltaR with photon [GeV]; Events", 2, -0.5, 1.5) );
    mon.addHistogram(new TH1F("Lepton_pT",      ";pT of Lepton [GeV]; Events", 30, 0, 300) );
    mon.addHistogram(new TH1F("Num_Vector_preshowerClusters", ";Number of Vector_preshowerClusters;Events",10,-1,9));
    mon.addHistogram(new TH1F("pass_Preseletion", ";pass_Preseletion; Events",  2,-0.5,1.5));
    mon.addHistogram(new TH1F("wrong_pt_order", ";wrong_pt_order; Events",  2,-0.5,1.5));
    mon.addHistogram(new TH1F("PHOTON_pT",      ";pT of Photon_one [GeV]; Events", 30, 0, 300) );
    mon.addHistogram(new TH1F("PHOTON_Eta",     ";Photon E_{T}; Events",           80, -4,   4));
    
    mon.addHistogram(new TH1F("numberofphoton",   ";Number of Photons;Events", 20, 0, 20) );
    mon.addHistogram(new TH1F("numberofmatchphoton",   ";Number of matching Photons;Events", 20, 0, 20) );

    // PhotonID Histograms
    mon.addHistogram(new TH1F("photonEcalIso",          ";Ecal Rec Hit Isolation;events", 30, 0, 30));
    mon.addHistogram(new TH1F("photonHcalIso",          ";Hcal Rec Hit Isolation;events", 30, 0, 30));
    mon.addHistogram(new TH1F("photonTrackSolidIso",    ";Sum of track pT in a cone of #DeltaR;events" , 30, 0, 30));
    mon.addHistogram(new TH1F("photonTrackHollowIso",   ";Sum of track pT in a hollow cone;events" ,     30, 0, 30));
    mon.addHistogram(new TH1F("photonTrackCountSolid",  ";Number of tracks in a cone of #DeltaR;events", 30, 0, 30));
    mon.addHistogram(new TH1F("photonTrackCountHollow", ";Number of tracks in a hollow cone;events",     30, 0, 30));
    mon.addHistogram(new TH1F("photonInEBEtagap",          ";Ecal Barrel eta gap flag;events",  2, -0.5, 1.5));
    mon.addHistogram(new TH1F("photonInEBPhigap",          ";Ecal Barrel phi gap flag;events",  2, -0.5, 1.5));
    mon.addHistogram(new TH1F("photonInEERinggap",          ";Ecal Endcap ring gap flag;events",  2, -0.5, 1.5));
    mon.addHistogram(new TH1F("photonInEEDeegap",          ";Ecal Endcap dee gap flag;events",  2, -0.5, 1.5));
    mon.addHistogram(new TH1F("photonInEEgap",          ";Ecal Barrel/Endcap gap flag;events",  2, -0.5, 1.5));
    mon.addHistogram(new TH1F("photonR9",               ";R9 = E(3x3) / E(SuperCluster);events", 120, 0, 1.2));
    
    // Photon Histograms
    mon.addHistogram(new TH1F("photonEt",     ";Photon E_{T};events",  20,  0, 200));
    mon.addHistogram(new TH1F("photonEta",    ";Photon #eta;events",   80, -4,   4));
    mon.addHistogram(new TH1F("photonPhi",    ";Photon #phi;events",   50, -1.*TMath::Pi(), TMath::Pi()));
    mon.addHistogram(new TH1F("photonHoverE", ";Hadronic over EM;events", 20, 0, 1));
    mon.addHistogram(new TH1F("photonPt", ";Photon pT [GeV];Events", 30, 0, 300) );
    
    
    // Photon's SuperCluster Histograms
    mon.addHistogram(new TH1F("photonScEt",  ";Photon SuperCluster E_{T};events", 20,  0, 200));
    mon.addHistogram(new TH1F("photonScEta", ";Photon #eta;events",               80, -4,   4));
    mon.addHistogram(new TH1F("photonScPhi", ";Photon #phi;events",50, -1.*TMath::Pi(), TMath::Pi()));
    mon.addHistogram(new TH1F("photonScEtaWidth",";#eta-width;events",            100,  0,  .1));
    
    // Composite or Other Histograms
    //  mon.addHistogram(new TH1F("photonInAnyGap",     "Photon in any gap flag",  2, -0.5, 1.5);
    //  mon.addHistogram(new TH1F("photonLoosePhoton", "Total number photons (0=NotPassing, 1=Passing)", 2, -0.5, 1.5));
    //  mon.addHistogram(new TH1F("photonLooseEM", "Total number photons (0=NotPassing, 1=Passing)", 2, -0.5, 1.5));
    //  mon.addHistogram(new TH1F("photonCount",        "Number of photons passing cuts in event",  10,  0,  10));
    
    mon.addHistogram(new TH1F("photonScPhiWidth",  ";#phi-width;events",         100,  0,  .1));
    mon.addHistogram(new TH1F("photonScRawEnergy",  ";Photon SuperCluster RawEnergy;events", 800,  0, 800));
    mon.addHistogram(new TH1F("photonScEnergy",  ";Photon SuperCluster Energy;events", 800,  0, 800));
    mon.addHistogram(new TH1F("photonrho",      ";#rho;events",               100, 0,   50));
    mon.addHistogram(new TH1F("photone2x2",  ";E_{2x2};events", 50,  0, 500));
    mon.addHistogram(new TH1F("photone5x5",  ";E_{5x5};events", 50,  0, 500));
    mon.addHistogram(new TH1F("photoneffSigmaRR",     ";ES #sigma_{effRR};events",  40,  0, 20));
    mon.addHistogram(new TH1F("photonsigmaIetaIeta",  ";#sigma_{i#etai#eta};events",    100,  0, 0.1));
    mon.addHistogram(new TH1F("photonsigmaEtaEta",  ";#sigma_{#eta#eta};events",    100,  0, 0.1));
    mon.addHistogram(new TH1F("photonsigmaIphiIphi",  ";#sigma_{i#phii#phi};events",    100,  0, 0.1));
    mon.addHistogram(new TH1F("photonsigmaIetaIphi",  ";#sigma_{i#etai#phi};events",    100,  -0.001, 0.001));
    mon.addHistogram(new TH1F("photonfull5x5_r9",     ";full5x5_r9;events",        120, 0, 1.2));
    
    mon.addHistogram(new TH1F("photonr2x5",     ";r2x5;events",        120, 0, 1.2));
    
    mon.addHistogram(new TH1F("photonE2x2overE5x5",  ";E_{2x2}/E_{5x5};   events",    100,  0, 2));
    
    mon.addHistogram(new TH1F("photonIso",          ";Photon Isolation;events", 30, 0, 30));
    mon.addHistogram(new TH1F("photonIsosumChargedParticlePt",          ";sumChargedParticlePt;events", 20, 0, 10));
    mon.addHistogram(new TH1F("photonchargedHadronIso",          ";chargedHadronIso;events", 20, 0, 10));
    mon.addHistogram(new TH1F("photonchargedHadronIsoWrongVtx",          ";chargedHadronIsoWrongVtx;events", 20, 0, 10));

    mon.addHistogram(new TH1F("photonPreshowerEOverScRawEnergy", ";PreshowerEOverScRawEnergy;events", 20, 0, 0.2));
    
    mon.addHistogram(new TH1F("muonIso",          ";Muon Isolation;events", 30, 0, 10));

    
    TFile *ofile=TFile::Open("out.root", "recreate");
    
    //TTree* tree_nvtx=new TTree("tree_nvtx","tree_nvtx");
    //TTree* treeMM=new TTree("treeMM","treeMM");
    //TTree* treegammagamma=new TTree("treegammagamma","treegammagamma");
    TTree* tree1=new TTree("tree1","tree1");
    TTree* treeEE=new TTree("treeEE","treeEE");
    TTree* treeEB=new TTree("treeEB","treeEB");
    TTree* treeVH=new TTree("treeVH","treeVH");
    TTree* treeVH_leading=new TTree("treeVH_leading","treeVH_leading");
    TTree* treeVH_subleading=new TTree("treeVH_subleading","treeVH_subleading");

   // TTree* tree2=new TTree("tree2","tree2");
    float V_MASS_LepLep;
    float HIGGS_MASS_gammagamma;
    float number_of_vertex;
    float pho_numphotons;
    bool pho_EE(false);
    bool pho_EB(false);
//    float HIGGS_MASS_A;
    //bool two_sel_Photons(false);
    
    float pho_ecalRecHitSumEtConeDR04;
    float pho_hcalTowerSumEtConeDR04;
    float pho_trkSumPtSolidConeDR04;
    float pho_trkSumPtHollowConeDR04;
    float pho_nTrkSolidConeDR04;
    float pho_nTrkHollowConeDR04;
    float pho_r9;
    
    float pho_Et;
    float pho_Eta;
    float pho_Phi;
    float pho_HOverE;
    float pho_Pt;
    
    float pho_ScEt;
    float pho_ScEta;
    float pho_ScPhi;
    float pho_ScEtaWidth;
    float pho_ScPhiWidth;
    float pho_ScRawEnergy;
    float pho_ScEnergy;
    
    float pho_rho;
    float pho_E2x2;
    float pho_E5x5;
    float pho_EffSigmaRR;
    float pho_SigmaIetaIeta;
    float pho_SigmaEtaEta;
    float pho_SigmaIphiIphi;
    float pho_SigmaIetaIphi;
    
    float pho_Full5x5_R9;
    float pho_R2x5;
    float pho_E2x2overE5x5;
    float pho_PhotonIso;
    float pho_SumChargedParticlePt;
    float pho_ChargedHadronIso;
    float pho_ChargedHadronIsoWrongVtx;
    float pho_PreshowerEOverScRawEnergy;
    
    //variables of VH
    float VH_pt;
    float VH_mass;
    float VH_et;
    float VH_eta;
    float VH_rapidity;
    float VH_colinearrapidity;
    float VH_deltaphi;
    float VH_deltaeta;
    float VH_costhera;
    float VH_r;
    //float VH_;
    float VH_leading_pho_pt;
    float VH_subleading_pho_pt;
    float VH_leadingPhoPtOverHiggsMass;
    float VH_SubleadingPhoPtOverHiggsMass;
    float leading_Photon_Pt;
    float leading_Photon_Pt_OverHiggsMass;

    
    /*
    double pho__ecalRecHitSumEtConeDR04;
    double pho__hcalTowerSumEtConeDR04;
    double pho__trkSumPtSolidConeDR04;
    double pho__trkSumPtHollowConeDR04;
    double pho__nTrkSolidConeDR04;
    double pho__nTrkHollowConeDR04;
    double pho__r9;*/
    
  //  tree1->Branch("recPhoton", &pho_ecalRecHitSumEtConeDR04, "pho_ecalRecHitSumEtConeDR04/F:pho_hcalTowerSumEtConeDR04:pho_trkSumPtSolidConeDR04:pho_trkSumPtHollowConeDR04:pho_nTrkSolidConeDR04:pho_nTrkHollowConeDR04:pho_r9");*/
    
    tree1->Branch("Number_of_Vertex", &number_of_vertex);
//    tree1->Branch("Mass_of_gen_higgs", &HIGGS_MASS_A);
    tree1->Branch("number_Of_Photons", &pho_numphotons);
    tree1->Branch("EE", &pho_EE);
    tree1->Branch("EB", &pho_EB);
    
    tree1->Branch("ecalRecHitSumEtConeDR04", &pho_ecalRecHitSumEtConeDR04);
    tree1->Branch("hcalTowerSumEtConeDR04", &pho_hcalTowerSumEtConeDR04);
    tree1->Branch("trkSumPtSolidConeDR04", &pho_trkSumPtSolidConeDR04);
    tree1->Branch("trkSumPtHollowConeDR04", &pho_trkSumPtHollowConeDR04);
    tree1->Branch("nTrkSolidConeDR04", &pho_nTrkSolidConeDR04);
    tree1->Branch("nTrkHollowConeDR04", &pho_nTrkHollowConeDR04);
    tree1->Branch("r9", &pho_r9);
    
    tree1->Branch("pho_Et", &pho_Et);
    tree1->Branch("pho_Eta", &pho_Eta);
    tree1->Branch("pho_Phi", &pho_Phi);
    tree1->Branch("pho_HOverE", &pho_HOverE);
    tree1->Branch("pho_Pt", &pho_Pt);
    
    tree1->Branch("pho_ScEt", &pho_ScEt);
    tree1->Branch("pho_ScEta", &pho_ScEta);
    tree1->Branch("pho_ScPhi", &pho_ScPhi);
    tree1->Branch("pho_ScEtaWidth", &pho_ScEtaWidth);
    tree1->Branch("pho_ScPhiWidth", &pho_ScPhiWidth);
    tree1->Branch("pho_ScRawEnergy", &pho_ScRawEnergy);
    tree1->Branch("pho_ScEnergy", &pho_ScEnergy);
    
    tree1->Branch("pho_rho", &pho_rho);
    tree1->Branch("pho_E2x2", &pho_E2x2);
    tree1->Branch("pho_E5x5", &pho_E5x5);
    tree1->Branch("pho_EffSigmaRR", &pho_EffSigmaRR);
    tree1->Branch("pho_SigmaIetaIeta", &pho_SigmaIetaIeta);
    tree1->Branch("pho_SigmaEtaEta", &pho_SigmaEtaEta);
    tree1->Branch("pho_SigmaIphiIphi", &pho_SigmaIphiIphi);
    tree1->Branch("pho_SigmaIetaIphi", &pho_SigmaIetaIphi);
    
    tree1->Branch("pho_Full5x5_R9", &pho_Full5x5_R9);
    tree1->Branch("pho_R2x5", &pho_R2x5);
    tree1->Branch("pho_E2x2overE5x5", &pho_E2x2overE5x5);
    tree1->Branch("pho_PhotonIso", &pho_PhotonIso);
    tree1->Branch("pho_SumChargedParticlePt", &pho_SumChargedParticlePt);
    tree1->Branch("pho_ChargedHadronIso", &pho_ChargedHadronIso);
    tree1->Branch("pho_ChargedHadronIsoWrongVtx", &pho_ChargedHadronIsoWrongVtx);
    
    tree1->Branch("pho_PreshowerEOverScRawEnergy", &pho_PreshowerEOverScRawEnergy);
  
    
    treeEE->Branch("ecalRecHitSumEtConeDR04", &pho_ecalRecHitSumEtConeDR04);
    treeEE->Branch("hcalTowerSumEtConeDR04", &pho_hcalTowerSumEtConeDR04);
    treeEE->Branch("trkSumPtSolidConeDR04", &pho_trkSumPtSolidConeDR04);
    treeEE->Branch("trkSumPtHollowConeDR04", &pho_trkSumPtHollowConeDR04);
    treeEE->Branch("nTrkSolidConeDR04", &pho_nTrkSolidConeDR04);
    treeEE->Branch("nTrkHollowConeDR04", &pho_nTrkHollowConeDR04);
    treeEE->Branch("r9", &pho_r9);
    
    treeEE->Branch("pho_Et", &pho_Et);
    treeEE->Branch("pho_Eta", &pho_Eta);
    treeEE->Branch("pho_Phi", &pho_Phi);
    treeEE->Branch("pho_HOverE", &pho_HOverE);
    treeEE->Branch("pho_Pt", &pho_Pt);
    
    treeEE->Branch("pho_ScEt", &pho_ScEt);
    treeEE->Branch("pho_ScEta", &pho_ScEta);
    treeEE->Branch("pho_ScPhi", &pho_ScPhi);
    treeEE->Branch("pho_ScEtaWidth", &pho_ScEtaWidth);
    treeEE->Branch("pho_ScPhiWidth", &pho_ScPhiWidth);
    treeEE->Branch("pho_ScRawEnergy", &pho_ScRawEnergy);
    treeEE->Branch("pho_ScEnergy", &pho_ScEnergy);
    
    treeEE->Branch("pho_rho", &pho_rho);
    treeEE->Branch("pho_E2x2", &pho_E2x2);
    treeEE->Branch("pho_E5x5", &pho_E5x5);
    treeEE->Branch("pho_EffSigmaRR", &pho_EffSigmaRR);
    treeEE->Branch("pho_SigmaIetaIeta", &pho_SigmaIetaIeta);
    treeEE->Branch("pho_SigmaEtaEta", &pho_SigmaEtaEta);
    treeEE->Branch("pho_SigmaIphiIphi", &pho_SigmaIphiIphi);
    treeEE->Branch("pho_SigmaIetaIphi", &pho_SigmaIetaIphi);
    
    treeEE->Branch("pho_Full5x5_R9", &pho_Full5x5_R9);
    treeEE->Branch("pho_R2x5", &pho_R2x5);
    treeEE->Branch("pho_E2x2overE5x5", &pho_E2x2overE5x5);
    treeEE->Branch("pho_PhotonIso", &pho_PhotonIso);
    treeEE->Branch("pho_SumChargedParticlePt", &pho_SumChargedParticlePt);
    treeEE->Branch("pho_ChargedHadronIso", &pho_ChargedHadronIso);
    treeEE->Branch("pho_ChargedHadronIsoWrongVtx", &pho_ChargedHadronIsoWrongVtx);
    treeEE->Branch("pho_PreshowerEOverScRawEnergy", &pho_PreshowerEOverScRawEnergy);

    treeEB->Branch("ecalRecHitSumEtConeDR04", &pho_ecalRecHitSumEtConeDR04);
    treeEB->Branch("hcalTowerSumEtConeDR04", &pho_hcalTowerSumEtConeDR04);
    treeEB->Branch("trkSumPtSolidConeDR04", &pho_trkSumPtSolidConeDR04);
    treeEB->Branch("trkSumPtHollowConeDR04", &pho_trkSumPtHollowConeDR04);
    treeEB->Branch("nTrkSolidConeDR04", &pho_nTrkSolidConeDR04);
    treeEB->Branch("nTrkHollowConeDR04", &pho_nTrkHollowConeDR04);
    treeEB->Branch("r9", &pho_r9);
    
    treeEB->Branch("pho_Et", &pho_Et);
    treeEB->Branch("pho_Eta", &pho_Eta);
    treeEB->Branch("pho_Phi", &pho_Phi);
    treeEB->Branch("pho_HOverE", &pho_HOverE);
    treeEB->Branch("pho_Pt", &pho_Pt);
    
    treeEB->Branch("pho_ScEt", &pho_ScEt);
    treeEB->Branch("pho_ScEta", &pho_ScEta);
    treeEB->Branch("pho_ScPhi", &pho_ScPhi);
    treeEB->Branch("pho_ScEtaWidth", &pho_ScEtaWidth);
    treeEB->Branch("pho_ScPhiWidth", &pho_ScPhiWidth);
    treeEB->Branch("pho_ScRawEnergy", &pho_ScRawEnergy);
    treeEB->Branch("pho_ScEnergy", &pho_ScEnergy);
    
    treeEB->Branch("pho_rho", &pho_rho);
    treeEB->Branch("pho_E2x2", &pho_E2x2);
    treeEB->Branch("pho_E5x5", &pho_E5x5);
    treeEB->Branch("pho_EffSigmaRR", &pho_EffSigmaRR);
    treeEB->Branch("pho_SigmaIetaIeta", &pho_SigmaIetaIeta);
    treeEB->Branch("pho_SigmaEtaEta", &pho_SigmaEtaEta);
    treeEB->Branch("pho_SigmaIphiIphi", &pho_SigmaIphiIphi);
    treeEB->Branch("pho_SigmaIetaIphi", &pho_SigmaIetaIphi);
    
    treeEB->Branch("pho_Full5x5_R9", &pho_Full5x5_R9);
    treeEB->Branch("pho_R2x5", &pho_R2x5);
    treeEB->Branch("pho_E2x2overE5x5", &pho_E2x2overE5x5);
    treeEB->Branch("pho_PhotonIso", &pho_PhotonIso);
    treeEB->Branch("pho_SumChargedParticlePt", &pho_SumChargedParticlePt);
    treeEB->Branch("pho_ChargedHadronIso", &pho_ChargedHadronIso);
    treeEB->Branch("pho_ChargedHadronIsoWrongVtx", &pho_ChargedHadronIsoWrongVtx);
    treeEB->Branch("pho_PreshowerEOverScRawEnergy", &pho_PreshowerEOverScRawEnergy);
    
    treeVH->Branch("VH_pt",   &VH_pt);
    treeVH->Branch("VH_mass", &VH_mass);
    treeVH->Branch("VH_et",   &VH_et);
    treeVH->Branch("VH_eta",  &VH_eta);
    treeVH->Branch("VH_rapidity", &VH_rapidity);
    treeVH->Branch("VH_colinearrapidity", &VH_colinearrapidity);
    treeVH->Branch("VH_deltaphi", &VH_deltaphi);
    treeVH->Branch("VH_deltaeta", &VH_deltaeta);
    treeVH->Branch("VH_costhera", &VH_costhera);
    treeVH->Branch("VH_r",        &VH_r);
    treeVH->Branch("HIGGS_MASS_gammagamma", &HIGGS_MASS_gammagamma);
    treeVH->Branch("V_MASS_LepLep", &V_MASS_LepLep);
    treeVH->Branch("leading_Photon_Pt", &leading_Photon_Pt);
    treeVH->Branch("leading_Photon_Pt_OverHiggsMass", &leading_Photon_Pt_OverHiggsMass);

    
    
    treeVH_leading->Branch("VH_leading_pho_pt",                  &VH_leading_pho_pt);
    treeVH_subleading->Branch("VH_subleading_pho_pt",               &VH_subleading_pho_pt);
    treeVH_leading->Branch("VH_leadingPhoPtOverHiggsMass",       &VH_leadingPhoPtOverHiggsMass);
    treeVH_subleading->Branch("VH_SubleadingPhoPtOverHiggsMass",    &VH_SubleadingPhoPtOverHiggsMass);

    //float pho_ecalRecHitSumEtConeDR04;
    //tree1->Branch("recPhoton", &pho_ecalRecHitSumEtConeDR04);
                  //, "pho_ecalRecHitSumEtConeDR04/F:pho_hcalTowerSumEtConeDR04:pho_trkSumPtSolidConeDR04:pho_trkSumPtHollowConeDR04:pho_nTrkSolidConeDR04:pho_nTrkHollowConeDR04:pho_r9");
    
    
  //lepton control
  //mon.addHistogram( new TH1F( "nleptons",   ";Nleptons;Events",10,0,10) );
  //mon.addHistogram( new TH1F( "leadpt",     ";Transverse momentum [GeV];Events", 50,0,500) );
  //mon.addHistogram( new TH1F( "leadeta",    ";Pseudo-rapidity;Events", 50,0,2.6) );
  //mon.addHistogram( new TH1F( "trailerpt",  ";Transverse momentum [GeV];Events", 50,0,500) );
  //mon.addHistogram( new TH1F( "trailereta", ";Pseudo-rapidity;Events", 50,0,2.6) );
  //mon.addHistogram( new TH1F( "zy",         ";Rapidity;Events", 50,0,3) );
 // mon.addHistogram( new TH1F( "zmass",      ";Mass [GeV];Events / 2 GeV", 100,40,240) );
 // mon.addHistogram( new TH1F( "zmass_btag50", ";Mass [GeV];Events / 2 GeV", 100,40,200) );
 // mon.addHistogram( new TH1F( "zmass_bveto50",";Mass [GeV];Events / 2 GeV", 100,40,200) );
 // mon.addHistogram( new TH1F( "zmass_btag80", ";Mass [GeV];Events / 2 GeV", 100,40,200) );
 // mon.addHistogram( new TH1F( "zmass_bveto80",";Mass [GeV];Events / 2 GeV", 100,40,200) );
 // mon.addHistogram( new TH1F( "zmass_btag125", ";Mass [GeV];Events / 2 GeV", 100,40,200) );
 // mon.addHistogram( new TH1F( "zmass_bveto125",";Mass [GeV];Events / 2 GeV", 100,40,200) );
 // mon.addHistogram( new TH1F( "zpt",        ";Transverse momentum [GeV];Events",150,0,1500));
  Double_t zptaxis[]= {0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345,360,375,390,405,435,465,495,525,555,585,615,675,735,795,855,975,1500};
  Int_t nzptAxis=sizeof(zptaxis)/sizeof(Double_t);
  //mon.addHistogram( new TH1F( "zpt_rebin",  ";Transverse momentum [GeV];Events / GeV", nzptAxis-1,zptaxis));
  //mon.addHistogram( new TH1F( "zptMet125",        ";Transverse momentum [GeV];Events",100,0,1500));
  //mon.addHistogram( new TH1F( "qmass",      ";Mass [GeV];Events / (1 GeV)",100,76,106));
 // mon.addHistogram( new TH1F( "qt",         ";Transverse momentum [GeV];Events / GeV",3000,0,3000));
 // mon.addHistogram( new TH1F( "qtraw",      ";Transverse momentum [GeV];Events / GeV",3000,0,3000));

  //extra leptons in the event
 // mon.addHistogram( new TH1F( "nextraleptons", ";Extra leptons;Events",4,0,4) );
 // mon.addHistogram( new TH1F( "thirdleptonpt", ";Transverse momentum;Events", 50,0,500) );
 // mon.addHistogram( new TH1F( "thirdleptoneta", ";Pseudo-rapidity;Events", 50,0,2.6) );
 // mon.addHistogram( new TH1F( "thirdleptonmt", ";Transverse mass(3^{rd} lepton,E_{T}^{miss}) [GeV];Events", 50,0,500) );


//  mon.addHistogram( new TH1F("csv",      ";Combined Secondary Vertex;Jets",50,0.,1.) );
//  mon.addHistogram( new TH1F("csvb",     ";Combined Secondary Vertex;Jets",50,0.,1.) );
//  mon.addHistogram( new TH1F("csvc",     ";Combined Secondary Vertex;Jets",50,0.,1.) );
//  mon.addHistogram( new TH1F("csvothers",";Combined Secondary Vertex;Jets",50,0.,1.) );
//  mon.addHistogram( new TH1F("leadjetpt",    ";Transverse momentum [GeV];Events",50,0,1000) );

//  mon.addHistogram( new TH1F("leadjet_pt",    ";Transverse momentum [GeV];Events",50,0,1000) );
//  mon.addHistogram( new TH1F("leadjet_eta",    ";Pseudo-rapidity;Events",25,0,5) );
//  mon.addHistogram( new TH1F("leadjet_phi",    ";leadjet #phi;Events",80, -4, 4) );

//  mon.addHistogram( new TH1F("trailerjetpt", ";Transverse momentum [GeV];Events",50,0,1000) );
//  mon.addHistogram( new TH1F("vbfjeteta",    ";Pseudo-rapidity;Events",25,0,5) );
//  mon.addHistogram( new TH1F("fwdjeteta",    ";Pseudo-rapidity;Events",25,0,5) );
//  mon.addHistogram( new TH1F("cenjeteta",       ";Pseudo-rapidity;Events",25,0,5) );
//  mon.addHistogram( new TH1F("jetId",       ";Pseudo-rapidity;Events",25,0,5) );
  Double_t mjjaxis[37];
  mjjaxis[0]=0.01;
  for(size_t i=1; i<20; i++)  mjjaxis[i]   =50*i;        //0-1000
  for(size_t i=0; i<5; i++)   mjjaxis[20+i]=1000+100*i; //1000-1500
  for(size_t i=0; i<=10; i++)   mjjaxis[25+i]=1500+300*i; //1500-5000
  mjjaxis[36]=9000;
//  mon.addHistogram( new TH1F("vbfmjj"       , ";Dijet invariant mass [GeV];Events / GeV",31,mjjaxis) );
//  mon.addHistogram( new TH1F("vbfdphijj"    , ";Azimuthal angle difference;Events",20,0,3.5) );
//  mon.addHistogram( new TH1F("vbfdetajj"    , ";Pseudo-rapidity span;Events",20,0,10) );
//  mon.addHistogram( new TH1F("vbfcjv"       , ";Central jet multiplicity;Events",5,0,5) );
//  TH1F* hjetsfinal   = (TH1F*) mon.addHistogram( new TH1F("njetsfinal",   ";Jet multiplicity;Events",5,0,5) );
//  TH1F* hjets   = (TH1F*) mon.addHistogram( new TH1F("njets",   ";Jet multiplicity;Events",5,0,5) );
//  TH1F* hbtags  = (TH1F*) mon.addHistogram( new TH1F("nbtags",  ";b-tag multiplicity;Events",5,0,5) );
//  for(int ibin=1; ibin<=hjets->GetXaxis()->GetNbins(); ibin++){
//      TString label("");
//     if(ibin==h->GetXaxis()->GetNbins()) label +="#geq";
//      label += (ibin-1);
//      hjets   ->GetXaxis()->SetBinLabel(ibin,label);
//      hjetsfinal->GetXaxis()->SetBinLabel(ibin,label);
//      hbtags  ->GetXaxis()->SetBinLabel(ibin,label);
//  }

//  mon.addHistogram( new TH1F( "mindphijmet",  ";min #Delta#phi(jet,E_{T}^{miss});Events",20,0,4) );
//  mon.addHistogram( new TH1F( "mindphijmet25",  ";min #Delta#phi(jet,E_{T}^{miss});Events",20,0,4) );
//  mon.addHistogram( new TH1F( "mindphijmet50",  ";min #Delta#phi(jet,E_{T}^{miss});Events",20,0,4) );
//  mon.addHistogram( new TH1F( "mindphijmetNM1",  ";min #Delta#phi(jet,E_{T}^{miss});Events",20,0,4) );
//  mon.addHistogram( new TH1D( "balance",      ";E_{T}^{miss}/q_{T};Events", 25,0,2.5) );
//  mon.addHistogram( new TH1D( "balanceNM1",   ";E_{T}^{miss}/q_{T};Events", 25,0,2.5) );
//  mon.addHistogram( new TH1F( "axialmet",     ";Axial missing transvere energy [GeV];Events", 50,-100,400) );
//  mon.addHistogram( new TH1F( "axialmetNM1",   ";Axial missing transvere energy [GeV];Events", 50,-100,400) );
  Double_t metaxis[]={0,5,10,15,20,25,30,35,40,45,50,55,60,70,80,90,100,125,150,175,200,250,300,400,500,600,700,800,900,1000};
  Int_t nmetAxis=sizeof(metaxis)/sizeof(Double_t);
//  mon.addHistogram( new TH1F( "metpuppi",          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
  mon.addHistogram( new TH1F( "met",          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
//  mon.addHistogram( new TH1F( "metNM1",        ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
  Double_t mtaxis[]={100,120,140,160,180,200,220,240,260,280,300,325,350,375,400,450,500,600,700,800,900,1000,1500,2000};
  Int_t nmtAxis=sizeof(mtaxis)/sizeof(Double_t);
  mon.addHistogram( new TH1F( "mt"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mtNM1"  ,       ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mtresponse",   ";Transverse mass response [GeV];Events / GeV", 100,0,2) );
//  mon.addHistogram( new TH1F( "mtcheckpoint"  ,         ";Transverse mass [GeV];Events / GeV",160,150,1750) );
//  mon.addHistogram( new TH1F( "metcheckpoint" ,         ";Missing transverse energy [GeV];Events / GeV",100,0,500) );

//  mon.addHistogram( new TH1F( "met_Inbtag",          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
//  mon.addHistogram( new TH1F( "met_Inbveto",          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
//  mon.addHistogram( new TH1F( "met_Outbtag",          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
//  mon.addHistogram( new TH1F( "met_Outbveto",          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );

/*  for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
     mon.addHistogram( new TH1F( (TString("metSyst")+varNames[ivar]),          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
     mon.addHistogram( new TH1F( (TString("mtSyst")+varNames[ivar]),         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
  }*/


//  mon.addHistogram( new TH1F( "mt_Inbtag50"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Inbveto50"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Inbtag80"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Inbveto80"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Inbtag125"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Inbveto125"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Outbtag50"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Outbveto50"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Outbtag80"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Outbveto80"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Outbtag125"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "mt_Outbveto125"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );

//  mon.addHistogram( new TH1F( "mtfinal"  ,         ";Transverse mass [GeV];Events / GeV",nmtAxis-1,mtaxis) );
//  mon.addHistogram( new TH1F( "metfinal",          ";Missing transverse energy [GeV];Events / GeV",nmetAxis-1,metaxis) ); //50,0,1000) );
//  mon.addHistogram( new TH1F( "mindphijmetfinal",  ";min #Delta#phi(jet,E_{T}^{miss});Events",20,0,4) );
//  mon.addHistogram( new TH1F("vbfmjjfinal"       , ";Dijet invariant mass [GeV];Events / GeV",31,mjjaxis) );
//  mon.addHistogram( new TH1F("vbfdetajjfinal"    , ";Pseudo-rapidity span;Events",20,0,10) );

//  TH2F *hmet2D=(TH2F *) mon.addHistogram( new TH2F ("met_vs_dphi",";Missing transverse energy [GeV];#Delta #phi(Z/#gamma,MET)",nmetAxis-1,metaxis,40, 0, 4) );
//  TH2F *hmt2D=(TH2F *) mon.addHistogram( new TH2F ("mt_vs_dphi",";Transverse mass [GeV];#Delta #phi(Z/#gamma,MET)",nmtAxis-1,mtaxis,40, 0, 4) );

  mon.addHistogram( new TH1F( "mzz",   ";M_{ZZ} [GeV];Events / GeV", 150, 0, 1500) ); //The binning is the same than the one for the corrections.


  //
  // HISTOGRAMS FOR OPTIMIZATION and STATISTICAL ANALYSIS
  //
  //

  //NEED FOR ALL OPTIMIZATION
/*  TH1F* Hoptim_systs     =  (TH1F*) mon.addHistogram( new TH1F ("optim_systs"    , ";syst;;", nvarsToInclude,0,nvarsToInclude) ) ;
  for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
      Hoptim_systs->GetXaxis()->SetBinLabel(ivar+1, varNames[ivar]);
  }*/

  std::vector<double> optim_Cuts1_met;
  optim_Cuts1_met.push_back(0); //add a bin in the shapes with a MET cut of 0
  for(double met=50;met<140;met+=5) {  optim_Cuts1_met    .push_back(met);  }
  TH2F* Hoptim_cuts  =(TH2F*)mon.addHistogram(new TProfile2D("optim_cut",      ";cut index;variable",       optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 1, 0, 1)) ;
  Hoptim_cuts->GetYaxis()->SetBinLabel(1, "met>");
  for(unsigned int index=0;index<optim_Cuts1_met.size();index++){ Hoptim_cuts    ->Fill(index, 0.0, optim_Cuts1_met[index]);  }
  for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
      for(unsigned int nri=0;nri<NRparams.size();nri++){
	mon.addHistogram( new TH2F (TString("mt_shapes")+NRsuffix[nri]+varNames[ivar],";cut index;Transverse mass [GeV];Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 285,150,3000) );
	mon.addHistogram( new TH2F (TString("met_shapes")+NRsuffix[nri]+varNames[ivar],";cut index;Missing transverse energy [GeV];Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(),300 ,0,1500) );
	TH2F *h=(TH2F *) mon.addHistogram( new TH2F ("mt_shapes_NRBctrl"+NRsuffix[nri]+varNames[ivar],";cut index;Selection region;Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(),6,0,6) );

	h->GetYaxis()->SetBinLabel(1,"M_{in}^{ll}/=0 b-tags");
	h->GetYaxis()->SetBinLabel(2,"M_{out}^{ll}/=0 b-tags");
	h->GetYaxis()->SetBinLabel(3,"M_{out+}^{ll}/=0 b-tags");
	h->GetYaxis()->SetBinLabel(4,"M_{in}^{ll}/#geq 1 b-tag");
	h->GetYaxis()->SetBinLabel(5,"M_{out}^{ll}/#geq 1 b-tag");
	h->GetYaxis()->SetBinLabel(6,"M_{out+}^{ll}/#geq 1 b-tag");
      }
    }

  std::vector<std::vector<double>> optim_Cuts_VBF;
  for(double jet2Pt=20    ;jet2Pt<50;jet2Pt+=5) {
     for(double jet1Pt=jet2Pt;jet1Pt<50;jet1Pt+=5) {
        for(double deta=2.0     ;deta<5.0;deta+=0.25) {
           for(double mjj=100.0    ;mjj<1000;mjj+=50) {
              optim_Cuts_VBF.push_back( std::vector<double>{jet1Pt, jet2Pt, deta, mjj} );
  }}}}

  TH2F* Hoptim_cuts_VBF  =(TH2F*)mon.addHistogram(new TProfile2D("optim_cut_VBF",      ";cut index;variable",       optim_Cuts_VBF.size(),0,optim_Cuts_VBF.size(), 4, 0, 4)) ;
  Hoptim_cuts_VBF->GetYaxis()->SetBinLabel(1, "jet1 p_{T}>");
  Hoptim_cuts_VBF->GetYaxis()->SetBinLabel(2, "jet2 p_{T}>");
  Hoptim_cuts_VBF->GetYaxis()->SetBinLabel(3, "d#eta>");
  Hoptim_cuts_VBF->GetYaxis()->SetBinLabel(4, "M_{jj}>");
  for(unsigned int index=0;index<optim_Cuts_VBF.size();index++){ for(unsigned int cut=0;cut<4;cut++){ Hoptim_cuts_VBF    ->Fill(index, float(cut), optim_Cuts_VBF[index][cut]); }  }
  for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
     if(ivar>0)continue;// do not fill for systematics in order to save time
     mon.addHistogram( new TH2F (TString("vbf_shapes")+varNames[ivar],";cut index;Transverse mass [GeV];Events",optim_Cuts_VBF.size(),0,optim_Cuts_VBF.size(), 1, 0, 2500) );
  }


  //##############################################
  //######## GET READY FOR THE EVENT LOOP ########
  //##############################################



  //MET CORRection level
  pat::MET::METCorrectionLevel metcor = pat::MET::METCorrectionLevel::Type1XY;
  //jet energy scale and uncertainties (2015 vs 2016 depends on the path "jecDir" )
  TString jecDir = runProcess.getParameter<std::string>("jecDir");
  //if (is2016MC || is2016data) { jecDir+="80X/"; }
  if (is2016data) {
      if(dtag.Contains("2016B") || dtag.Contains("2016C") ||dtag.Contains("2016D")) jecDir+="Summer16_80X/Summer16_23Sep2016BCDV4_DATA/";
      else if(dtag.Contains("2016E") || dtag.Contains("2016F")) jecDir+="Summer16_80X/Summer16_23Sep2016EFV4_DATA/";
      else if(dtag.Contains("2016G")) jecDir+="Summer16_80X/Summer16_23Sep2016GV4_DATA/";
      else if(dtag.Contains("2016H")) jecDir+="Summer16_80X/Summer16_23Sep2016HV4_DATA/";
  }
  if(is2016MC) {jecDir+="Summer16_80X/Summer16_23Sep2016V4_MC/";}


  gSystem->ExpandPathName(jecDir);

  FactorizedJetCorrector *jesCor = NULL;
  jesCor = utils::cmssw::getJetCorrector(jecDir,isMC);

  TString pf(isMC ? "MC" : "DATA");
  JetCorrectionUncertainty *totalJESUnc = NULL;
  //if(isMC || is2015data)
  totalJESUnc = new JetCorrectionUncertainty((jecDir+"/"+pf+"_Uncertainty_AK4PFchs.txt").Data());

  //muon energy scale and uncertainties
  TString muscleDir = runProcess.getParameter<std::string>("muscleDir");
  gSystem->ExpandPathName(muscleDir);
  rochcor2015* muCor2015 = NULL; //need to be updated for 2016
  if(is2015MC || is2015data) muCor2015 = new rochcor2015();  //replace the MuScleFitCorrector we used at run1
  rochcor2016* muCor2016 = NULL; //need to be updated for 2016
  if(is2016MC || is2016data) muCor2016 = new rochcor2016();  //replace the MuScleFitCorrector we used at run1

  //photon and electron enerhy scale based on https://twiki.cern.ch/twiki/bin/viewauth/CMS/EGMSmearer    (adapted to the miniAOD/FWLite framework)

  EnergyScaleCorrection_class eScaler("EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_74x_pho");
  eScaler.doScale=true;
  eScaler.doSmearings=true;

  std::string bit_string_stat = "001";
  std::string bit_string_syst = "010";
  std::string bit_string_gain = "100";
  std::bitset<6> bit_stat(bit_string_stat);
  std::bitset<6> bit_syst(bit_string_syst);
  std::bitset<6> bit_gain(bit_string_gain);

  EnergyScaleCorrection_class phScaler("EgammaAnalysis/ElectronTools/data/ScalesSmearings/Moriond17_74x_pho");
  phScaler.doScale=true;
  phScaler.doSmearings=true;

  //lepton efficiencies
  LeptonEfficiencySF lepEff;

  //photon efficiencies
  PhotonEfficiencySF phoEff;

  //b-tagging: beff and leff must be derived from the MC sample using the discriminator vs flavor
  //the scale factors are taken as average numbers from the pT dependent curves see:
  //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagPOG#2012_Data_and_MC_EPS13_prescript
  BTagSFUtil btsfutil;
  float beff(0.68), sfb(0.99), sfbunc(0.015);
  float leff(0.13), sfl(1.05), sflunc(0.12);

  //double btagLoose = 0.605; //https://twiki.cern.ch/twiki/bin/viewauth/CMS/BtagRecommendation74X FIXME, I sent an email to Petra to know more (Hugo)
  //double btagLoose = 0.460;  //80X recommendation Loose
  double btagLoose = 0.5426;  //Moriond17 recommendation Loose

  // setup calibration readers 74X
  //BTagCalibration btagCalib("CSVv2", string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/btagSF_CSVv2.csv");
  //BTagCalibrationReader btagCal   (&btagCalib, BTagEntry::OP_LOOSE, "mujets", "central");  // calibration instance, operating point, measurement type, systematics type
  //BTagCalibrationReader btagCalUp (&btagCalib, BTagEntry::OP_LOOSE, "mujets", "up"     );  // sys up
  //BTagCalibrationReader btagCalDn (&btagCalib, BTagEntry::OP_LOOSE, "mujets", "down"   );  // sys down
  //BTagCalibrationReader btagCalL  (&btagCalib, BTagEntry::OP_LOOSE, "comb", "central");  // calibration instance, operating point, measurement type, systematics type
  //BTagCalibrationReader btagCalLUp(&btagCalib, BTagEntry::OP_LOOSE, "comb", "up"     );  // sys up
  //BTagCalibrationReader btagCalLDn(&btagCalib, BTagEntry::OP_LOOSE, "comb", "down"   );  // sys down

  // setup calibration readers 80X
  BTagCalibration btagCalib("CSVv2", string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/weights/CSVv2_Moriond17_B_H.csv");

  BTagCalibrationReader80X btagCal80X   (BTagEntry::OP_LOOSE, "central", {"up", "down"});
  btagCal80X.load(btagCalib, BTagEntry::FLAV_B, "comb");
  btagCal80X.load(btagCalib, BTagEntry::FLAV_C, "comb");
  btagCal80X.load(btagCalib, BTagEntry::FLAV_UDSG, "incl");


  // from Btag SF and eff from https://indico.cern.ch/event/437675/#preview:1629681
  // beff = 0.747; sfb = 0.899; //for Loose WP  //sfb is not actually used as it's taken from btagCal
  //
  //beff = 0.836; sfb = 0.920; //for Loose WP  //sfb is from page 7 https://indico.cern.ch/event/557018/contributions/2246312/attachments/1310986/1961665/csvSF_rwt_July18th_2016.pdf
  //leff = 0.139;
  beff = 0.827; sfb = 0.980; //for Loose WP  //sfb is from page 7 https://indico.cern.ch/event/557018/contributions/2246312/attachments/1310986/1961665/csvSF_rwt_July18th_2016.pdf
  leff = 0.132;

  //pileup weighting
  edm::LumiReWeighting* LumiWeights = NULL;
  utils::cmssw::PuShifter_t PuShifters;
  double PUNorm[] = {1,1,1};

  //MC normalization (to 1/pb)
  double xsecWeight = 1.0;
  if(isMC){
          std::vector<double> dataPileupDistributionDouble = runProcess.getParameter< std::vector<double> >("datapileup");
          std::vector<float> dataPileupDistribution; for(unsigned int i=0;i<dataPileupDistributionDouble.size();i++){dataPileupDistribution.push_back(dataPileupDistributionDouble[i]);}
          std::vector<float> mcPileupDistribution;

          double totalNumEvent = utils::getMCPileupDistributionAndTotalEventFromMiniAOD(urls,dataPileupDistribution.size(), mcPileupDistribution);
          xsecWeight=xsec/totalNumEvent;

	  //utils::getMCPileupDistributionFromMiniAOD(urls,dataPileupDistribution.size(), mcPileupDistribution);
          while(mcPileupDistribution.size()<dataPileupDistribution.size())  mcPileupDistribution.push_back(0.0);
          while(mcPileupDistribution.size()>dataPileupDistribution.size())dataPileupDistribution.push_back(0.0);
          gROOT->cd();  //THIS LINE IS NEEDED TO MAKE SURE THAT HISTOGRAM INTERNALLY PRODUCED IN LumiReWeighting ARE NOT DESTROYED WHEN CLOSING THE FILE
          LumiWeights = new edm::LumiReWeighting(mcPileupDistribution,dataPileupDistribution);
          PuShifters=utils::cmssw::getPUshifters(dataPileupDistribution,0.05);
          utils::getPileupNormalization(mcPileupDistribution, PUNorm, LumiWeights, PuShifters);
  }

  gROOT->cd();  //THIS LINE IS NEEDED TO MAKE SURE THAT HISTOGRAM INTERNALLY PRODUCED IN LumiReWeighting ARE NOT DESTROYED WHEN CLOSING THE FILE


  higgs::utils::EventCategory eventCategoryInst(higgs::utils::EventCategory::EXCLUSIVE2JETSVBF); //jet(0,>=1)+vbf binning
  patUtils::MetFilter metFilter;
  if(!isMC){
    if(is2015data){
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleEG_RunD/DoubleEG_csc2015.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleEG_RunD/DoubleEG_ecalscn1043093.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleMuon_RunD/DoubleMuon_csc2015.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/DoubleMuon_RunD/DoubleMuon_ecalscn1043093.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/MuonEG_RunD/MuonEG_csc2015.txt");
      metFilter.FillBadEvents(string(std::getenv("CMSSW_BASE"))+"/src/UserCode/llvv_fwk/data/MetFilter/MuonEG_RunD/MuonEG_ecalscn1043093.txt");
    }
    else if(is2016data){

    }
        //FIXME, we need to add here the single mu, single el, and gamma path
  }
  string debugText = "";

  //##############################################
  //########           EVENT LOOP         ########
  //##############################################
  //loop on all the events
  //DuplicatesChecker duplicatesChecker;
  //int nDuplicates(0)

  printf("Progressing Bar           :0%%       20%%       40%%       60%%       80%%       100%%\n");
  for(unsigned int f=0;f<urls.size();f++){
     TFile* file = TFile::Open(urls[f].c_str() );
     fwlite::Event ev(file);
     cout<<"Number of enerty: "<<ev.size()<<endl;
     printf("Scanning the ntuple %2i/%2i :", (int)f+1, (int)urls.size());
     int iev=0;
     int treeStep(ev.size()/50);
     if(treeStep==0){ treeStep = 1;}
     for(ev.toBegin(); !ev.atEnd(); ++ev){ iev++;
         if(iev%treeStep==0){printf(".");fflush(stdout);}
         float weight = xsecWeight;
         double puWeightUp = 1.0;
         double puWeightDown = 1.0;
         float puWeight(1.0);

         //##############################################   EVENT LOOP STARTS   ##############################################
         //if(!isMC && duplicatesChecker.isDuplicate( ev.run, ev.lumi, ev.event) ) { nDuplicates++; continue; }

         //Skip bad lumi
         if(!isMC && !goodLumiFilter.isGoodLumi(ev.eventAuxiliary().run(),ev.eventAuxiliary().luminosityBlock()))continue;

         reco::GenParticleCollection gen;
         GenEventInfoProduct eventInfo;
          if(isMC){
            fwlite::Handle< reco::GenParticleCollection > genHandle;
            genHandle.getByLabel(ev, "prunedGenParticles");
            if(genHandle.isValid()){ gen = *genHandle;}

            fwlite::Handle< GenEventInfoProduct > genEventInfoHandle;
            genEventInfoHandle.getByLabel(ev, "generator");
            if(genEventInfoHandle.isValid()){ eventInfo = *genEventInfoHandle;}

            //WEIGHT for NLO negative interference
            //totalNumEvent+=eventInfo.weight();
            weight *= eventInfo.weight();


            //WEIGHT for Pileup
	    int ngenITpu = 0;
	    fwlite::Handle< std::vector<PileupSummaryInfo> > puInfoH;
            puInfoH.getByLabel(ev, "slimmedAddPileupInfo");
            for(std::vector<PileupSummaryInfo>::const_iterator it = puInfoH->begin(); it != puInfoH->end(); it++){
               if(it->getBunchCrossing()==0)      { ngenITpu += it->getTrueNumInteractions(); } //getPU_NumInteractions();
            }
            puWeight          = LumiWeights->weight(ngenITpu) * PUNorm[0];
            puWeightUp  = PuShifters[utils::cmssw::PUUP  ]->Eval(ngenITpu) * (PUNorm[2]/PUNorm[0]);
            puWeightDown = PuShifters[utils::cmssw::PUDOWN]->Eval(ngenITpu) * (PUNorm[1]/PUNorm[0]);
            weight *= puWeight;

            if(isMC && (mctruthmode==15 || mctruthmode==1113)){// && (string(dtag.Data()).find("Z#rightarrow")==0 || isMC_ZZ2l2nu))
                int prodId = 1;
                for( unsigned int k=0; k<gen.size(); ++k){
                        if( gen[k].isHardProcess() && ( abs( gen[k].pdgId() ) == 11 || abs( gen[k].pdgId() ) == 13 || abs( gen[k].pdgId() )==15 ) ) prodId*=gen[k].pdgId();
                }
                if(mctruthmode==15   && abs(prodId)!=225)continue; //skip not tautau
                if(mctruthmode==1113 && abs(prodId)==225)continue; //skip tautau
            }

            if(isMC_VBF || isMC_GG || mctruthmode==125 || isMC_ggZZ2mu2nu || isMC_ggZZ2e2nu ){
               LorentzVector higgs(0,0,0,0);
  	       for(unsigned int igen=0; igen<gen.size(); igen++){
                  if(!gen[igen].isHardProcess()) continue;
	          if(abs(gen[igen].pdgId())>=11 && abs(gen[igen].pdgId())<=16){ higgs += gen[igen].p4(); }
	       }
	       if(mctruthmode==125) {
	         if( isMC_125OnShell && higgs.mass()> 180) continue;
	         if(!isMC_125OnShell && higgs.mass()<=180) continue;
	       }

               //Line shape weights
               if(isMC_VBF || isMC_GG || isMC_ggZZ2mu2nu || isMC_ggZZ2e2nu ){

               	  mon.fillHisto("higgsMass_raw",    "all", higgs.mass(), weight);

		  //Apply NNLO kFactors to ggZZ2l2v or ggHZZ2l2v
                  if(isMC_GG || isMC_ggZZ2mu2nu || isMC_ggZZ2e2nu){ nnlo_ggH_kFact=HNNLO_Wgt_Gr->Eval(higgs.mass());
		  }else{ nnlo_ggH_kFact=1.0;}

                  weight *= nnlo_ggH_kFact;
		  mon.fillHisto("higgsMass_nnlo_raw",  "all", higgs.mass(), weight);

                  //WEIGHT for LineShape (NNLO kFactors + Interf), split in shape and scale unc with the following format:  scaleNominal, shapeNominal, scaleDown, shapeDown, scaleUp, shapeUp
                  //compute weight correction for all NR shapes
                  for(unsigned int nri=0;nri<NRparams.size();nri++){
                     std::vector<std::pair<double, TGraph*> > shapeWgtGr = hLineShapeGrVec[NRparams[nri] ];
                     if(!isMELA){
			for(size_t iwgt=0; iwgt<shapeWgtGr.size(); iwgt++){
                        	lShapeWeights[nri][iwgt*2+0]=shapeWgtGr[iwgt].first;
                        	lShapeWeights[nri][iwgt*2+1]=shapeWgtGr[iwgt].second?shapeWgtGr[iwgt].second->Eval(higgs.mass()):1.0;
                    	 }
		     }else if(isMELA){
			//printf("MelaMode: %s CPrime: %5.3f MPole: %5f \n", MelaMode.c_str(), NRparams[nri].first, resonance);
                        lMelaShapeWeights[nri][MelaMode] = higgs::utils::weightNarrowResonnance_MELA( mela, isMC_VBF, MelaMode, NRparams[nri].first, resonance, ev);
		     }

		     double shape_SF =0; double shapescale_SF = 0;
		     if( !isMELA ){ shape_SF = lShapeWeights[nri][1]; shapescale_SF = lShapeWeights[nri][0]; }
		     else if( isMELA ){ shape_SF = lMelaShapeWeights[nri][MelaMode]; shapescale_SF = lMelaShapeWeights[nri][MelaMode]; }

		     mon.fillHisto(TString("MELA_weights"         )+NRsuffix[nri], "all",            1, shape_SF);
                     mon.fillHisto(TString("higgsMass_shape"      )+NRsuffix[nri], "all", higgs.mass(), weight*shape_SF );
                     mon.fillHisto(TString("higgsMass_shape&scale")+NRsuffix[nri], "all", higgs.mass(), weight*shapescale_SF );

                     //scale Up/Down by Nominal
                     if(lShapeWeights[nri][0]>0){lShapeWeights[nri][2]/=lShapeWeights[nri][0];  lShapeWeights[nri][4]/=lShapeWeights[nri][0];}
                     if(lShapeWeights[nri][1]>0){lShapeWeights[nri][3]/=lShapeWeights[nri][1];  lShapeWeights[nri][5]/=lShapeWeights[nri][1];}

                     //printf("NRI AW=%i --> %6.2e %6.2e %6.2e %6.2e %6.2e %6.2e\n", nri, lShapeWeights[nri][0], lShapeWeights[nri][1], lShapeWeights[nri][2], lShapeWeights[nri][3], lShapeWeights[nri][4], lShapeWeights[nri][5]);
                  }
                  if(!isMELA){ weight *= lShapeWeights[0][0]*lShapeWeights[0][1]; }
		  else if(isMELA){ weight *= lMelaShapeWeights[0][MelaMode]; }
	      }
            }
          }

	  //Weight in case the code run over Bckg in Mela Framework
          if( !isMC_VBF && !isMC_GG && isMELA){
             for(unsigned int nri=0; nri<NRsuffix.size(); nri++){
                lMelaShapeWeights[nri][MelaMode] = 1.0;
             }
          }


          //apply trigger and require compatibilitiy of the event with the PD
          edm::TriggerResultsByName tr(nullptr,nullptr);
          if(isReHLT) { tr = ev.triggerResultsByName("HLT2");}
          else { tr = ev.triggerResultsByName("HLT");}

          //if(!tr.isValid())return false;
          if(!tr.isValid() && (!isMC_signal && is2016MC)  )return false;

          float triggerPrescale(1.0),triggerThreshold(0), triggerThresholdHigh(99999);
          char photonTriggerTreshName[255];

	  bool mumuTrigger(true); bool muTrigger(true);	bool eeTrigger(true); bool eTrigger(true); bool emuTrigger(true);
	  bool photonTrigger(true); bool highPTeeTrigger(true);

          int metFilterValue = 0;

	  bool filterbadPFMuon = true;
	  bool filterbadChCandidate = true;
	  bool filterbadMuonHIP = true;
	  bool filterduplicateMuonHIP = true;
	  std::unique_ptr<std::vector<reco::Muon*>> outbadMuon(new std::vector<reco::Muon*>());
	  std::unique_ptr<std::vector<reco::Muon*>> outduplicateMuon(new std::vector<reco::Muon*>());
          if (is2016data || is2016MC) {

	    mumuTrigger        = utils::passTriggerPatterns(tr, "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_v*", "HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_v*", "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v*" , "HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v*");
	    muTrigger          = utils::passTriggerPatterns(tr, "HLT_IsoMu22_v*","HLT_IsoTkMu22_v*", "HLT_IsoMu24_v*", "HLT_IsoTkMu24_v*");
	    eeTrigger          = utils::passTriggerPatterns(tr, "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*","HLT_Ele17_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*","HLT_DoubleEle33_CaloIdL_v*");
	    highPTeeTrigger    = utils::passTriggerPatterns(tr, "HLT_ECALHT800_v*");
	    eTrigger           = utils::passTriggerPatterns(tr, "HLT_Ele27_eta2p1_WPLoose_Gsf_v*","HLT_Ele27_WPTight_Gsf_v*") ;
	    emuTrigger         = utils::passTriggerPatterns(tr, "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v*","HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v*","HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v*" , "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*") || utils::passTriggerPatterns(tr,"HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v*","HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v*");
	    photonTrigger      = ( patUtils::passPhotonTrigger(ev, triggerThreshold, triggerPrescale, triggerThresholdHigh) );
	      //				     patUtils::passVBFPhotonTrigger(ev, triggerThreshold, triggerPrescale, triggerThresholdHigh) );

	    metFilterValue = metFilter.passMetFilterInt( ev, is2016data );

	    // Apply Bad Charged Hadron and Bad Muon Filters from MiniAOD (for Run II 2016 only )
	    filterbadChCandidate = metFilter.passBadChargedCandidateFilter(ev); if (!filterbadChCandidate) {  metFilterValue=9; }
	    filterbadPFMuon = metFilter.passBadPFMuonFilter(ev); if (!filterbadPFMuon) { metFilterValue=8; }
	    filterbadMuonHIP = metFilter.BadGlobalMuonTaggerFilter(ev,outbadMuon,false); if (!filterbadMuonHIP) { metFilterValue=10; }
	    filterduplicateMuonHIP = metFilter.BadGlobalMuonTaggerFilter(ev,outduplicateMuon,true); if (!filterduplicateMuonHIP) { metFilterValue=11; }

          } else {

            mumuTrigger        = utils::passTriggerPatterns(tr, "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v*", "HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v*");
            muTrigger          = utils::passTriggerPatterns(tr, "HLT_IsoMu20_v*", "HLT_IsoTkMu20_v*", "HLT_IsoMu27_v*");
            eeTrigger          = utils::passTriggerPatterns(tr, "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*","HLT_Ele17_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*");
            eTrigger           = utils::passTriggerPatterns(tr, "HLT_Ele23_WPLoose_Gsf_v*", "HLT_Ele22_eta2p1_WP75_Gsf_v*", "HLT_Ele22_eta2p1_WPLoose_Gsf_v*");
            emuTrigger         = utils::passTriggerPatterns(tr, "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v*",
                                                            "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v*");

            photonTrigger      = patUtils::passPhotonTrigger(ev, triggerThreshold, triggerPrescale, triggerThresholdHigh);
            metFilterValue = metFilter.passMetFilterInt( ev );
          }

          mon.fillHisto("metFilter_eventflow", "", metFilterValue, weight);

          bool passTrigger        = mumuTrigger||muTrigger||eeTrigger||highPTeeTrigger||eTrigger||emuTrigger||photonTrigger;

          if(  mumuTrigger)mon.fillHisto("trigger", "raw", 0 , weight);
          if(    muTrigger)mon.fillHisto("trigger", "raw", 1 , weight);
          if(    eeTrigger)mon.fillHisto("trigger", "raw", 2 , weight);
          if(     eTrigger)mon.fillHisto("trigger", "raw", 3 , weight);
          if(   emuTrigger)mon.fillHisto("trigger", "raw", 4 , weight);
          if(photonTrigger)mon.fillHisto("trigger", "raw", 5 , weight);

          if(!isMC && passTrigger){ //avoid double counting of events from different PD
             if(filterOnlyMUMU)     { passTrigger = mumuTrigger;}
             if(filterOnlyMU)       { passTrigger = muTrigger     && !mumuTrigger;}
             if(filterOnlyEE)       { passTrigger = (eeTrigger || highPTeeTrigger)   && !muTrigger  && !mumuTrigger;}
             if(filterOnlyE)        { passTrigger = eTrigger  && !highPTeeTrigger   && !eeTrigger  && !muTrigger && !mumuTrigger; }
             if(filterOnlyEMU)      { passTrigger = emuTrigger    && !eTrigger   && !eeTrigger && !muTrigger && !mumuTrigger; }
             if(filterOnlyPhoton)   { passTrigger = photonTrigger && !emuTrigger && !eTrigger  && !eeTrigger && !muTrigger && !mumuTrigger;}
          }

          if(passTrigger){
             if(  mumuTrigger)mon.fillHisto("trigger", "cleaned", 0 , weight);
             if(    muTrigger)mon.fillHisto("trigger", "cleaned", 1 , weight);
             if(    eeTrigger)mon.fillHisto("trigger", "cleaned", 2 , weight);
             if(     eTrigger)mon.fillHisto("trigger", "cleaned", 3 , weight);
             if(   emuTrigger)mon.fillHisto("trigger", "cleaned", 4 , weight);
             if(photonTrigger)mon.fillHisto("trigger", "cleaned", 5 , weight);
          }

          if(photonTrigger){sprintf(photonTriggerTreshName, "PhoTrg%i", int(triggerThreshold));}


          //ONLY RUN ON THE EVENTS THAT PASS OUR TRIGGERS
	  if(!passTrigger && !photonTriggerStudy)continue;
         //##############################################   EVENT PASSED THE TRIGGER   ######################################
	  if (metFilterValue==10 || metFilterValue==11) { metFilterValue=0; }
          if( metFilterValue!=0 ) continue;	 //Note this must also be applied on MC

	  // Apply Bad Charged Hadron and Bad Muon Filters from MiniAOD (for Run II 2016 only )
	  //	  if (!filterbadPFMuon || !filterbadChCandidate) continue;
          //##############################################   EVENT PASSED MET FILTER   #######################################


          //load all the objects we will need to access
          reco::VertexCollection vtx;
          fwlite::Handle< reco::VertexCollection > vtxHandle;
          vtxHandle.getByLabel(ev, "offlineSlimmedPrimaryVertices");
          if(vtxHandle.isValid()){ vtx = *vtxHandle;}

          double rho = 0;
          fwlite::Handle< double > rhoHandle;
          rhoHandle.getByLabel(ev, "fixedGridRhoFastjetAll");
          if(rhoHandle.isValid()){ rho = *rhoHandle;}

          pat::MuonCollection muons;
          fwlite::Handle< pat::MuonCollection > muonsHandle;
          muonsHandle.getByLabel(ev, "slimmedMuons");
          if(muonsHandle.isValid()){ muons = *muonsHandle;}

          pat::ElectronCollection electrons;
          fwlite::Handle< pat::ElectronCollection > electronsHandle;
          electronsHandle.getByLabel(ev, "slimmedElectrons");
          if(electronsHandle.isValid()){ electrons = *electronsHandle;}

          // to find Gain Seed 
          fwlite::Handle<EcalRecHitCollection> recHitCollectionEBHandle;
          fwlite::Handle<EcalRecHitCollection> recHitCollectionEEHandle;
          recHitCollectionEBHandle.getByLabel(ev, "reducedEgamma","reducedEBRecHits");
          recHitCollectionEEHandle.getByLabel(ev, "reducedEgamma","reducedEERecHits");

          pat::JetCollection jets;
          fwlite::Handle< pat::JetCollection > jetsHandle;
          jetsHandle.getByLabel(ev, "slimmedJets");
          if(jetsHandle.isValid()){ jets = *jetsHandle;}

          pat::PhotonCollection photons;
          fwlite::Handle< pat::PhotonCollection > photonsHandle;
          photonsHandle.getByLabel(ev, "slimmedPhotons");
          if(photonsHandle.isValid()){ photons = *photonsHandle;}
         
         //edm::Ptr<pat::Photon> pho;
         

        // const edm::Ptr<reco::Candidate>& particle;
        // const edm::Ptr<reco::Photon> phoRecoPtr = ( edm::Ptr<reco::Photon> )particle;
         
         //edm::Ptr<reco::Photon> photonPtr =
         
         
         
 /*        reco::PhotonCollection photons;
         fwlite::Handle< reco::PhotonCollection > photonsHandle;
         photonsHandle.getByLabel(ev, "slimmedPhotons");
         if(photonsHandle.isValid()){ photons = *photonsHandle;}
*/
          pat::METCollection mets;
          fwlite::Handle< pat::METCollection > metsHandle;
          if(!isMC)metsHandle.getByLabel(ev, "slimmedMETsMuEGClean");
          if(isMC)metsHandle.getByLabel(ev, "slimmedMETs");
          if(metsHandle.isValid()){ mets = *metsHandle;}
          pat::MET met = mets[0];

          pat::METCollection puppimets;
          fwlite::Handle< pat::METCollection > puppimetsHandle;
          puppimetsHandle.getByLabel(ev, "slimmedMETsPuppi");
          if(puppimetsHandle.isValid()){ puppimets = *puppimetsHandle;}
          LorentzVector puppimet = puppimets[0].p4();

          fwlite::Handle<pat::PackedCandidateCollection> PFparticles;
          PFparticles.getByLabel(ev, "packedPFCandidates");

    			//GenJets
			    reco::GenJetCollection genJets;
    			fwlite::Handle< reco::GenJetCollection > genJetsHandle;
    			genJetsHandle.getByLabel(ev, "slimmedGenJets");
    			if(genJetsHandle.isValid()){ genJets = *genJetsHandle;}

         if(isV0JetsMC){
            fwlite::Handle< LHEEventProduct > lheEPHandle;
            lheEPHandle.getByLabel(ev, "externalLHEProducer");
            if(lheEPHandle.isValid()){
               mon.fillHisto("nup","",lheEPHandle->hepeup().NUP,1);
               if(lheEPHandle->hepeup().NUP>5) continue;
               mon.fillHisto("nupfilt","",lheEPHandle->hepeup().NUP,1);
            }else{
               printf("Handle to externalLHEProducer is invalid --> Can not ignore V0+Jet events from inclusive samples\n");
            }
         }


         //MC crap for photon studies
         if(photonTrigger ){
         //if(photonTrigger && (isWGmc || isZGmc)){
           int nge(0), ngm(0), ngt(0), ngj(0), ngnu(0);
           bool zFound(false), wFound(false);
             
           mon.fillHisto("check_gen_size","",gen.size(),1);
           
            for(size_t ig=0; ig<gen.size(); ig++){
               //mon.fillHisto("check_gen_size","",gen[ig].size(),1);
               mon.fillHisto("Status_WZH","",gen[ig].status(),1);
               
               //if(gen[ig].status()!=3) continue;
             int id(abs(gen[ig].pdgId()));
             if(id==23) zFound=true;
             if(id==24) wFound=true;
             if(id==11) nge++;
             if(id==13) ngm++;
             if(id==15) ngt++;
             if(id==12 || id==14 || id==16) ngnu++;
             if((wFound || zFound) && id<6) ngj++;
           }
           if(zFound){
             int decBin=0;
             if(nge==2) decBin=1;
             if(ngm==2) decBin=2;
             if(ngt==2) decBin=3;
             if(ngj>=2) decBin=4;
             if(ngnu==2) decBin=5;
             mon.fillHisto("zdecays","",decBin,1);
           }
           if(wFound){
             int decBin=0;
             if(nge==1 && ngnu==1) decBin=1;
             if(ngm==1 && ngnu==1) decBin=2;
             if(ngt==1 && ngnu==1) decBin=3;
             if(ngj>=2) decBin=4;
             mon.fillHisto("wdecays","",decBin,1);
           }
         }

         //Resolve G+jet/QCD mixing (avoid double counting of photons)
         if (isMC_GJet || isMC_QCD ||
	     isMC_Wlnu_inclusive || isMC_Wlnu_HT100 || isMC_WGToLNuG || 
             isMC_ZNuNuGJets || isMC_ZJetsToNuNu ) {
           // iF GJet sample; accept only event with prompt photons
           // if QCD sample; reject events with prompt photons in final state
             bool gPromptFound=false;
             for(size_t ig=0; ig<gen.size(); ig++){
               if((abs(gen[ig].pdgId())==22) && gen[ig].isPromptFinalState())  gPromptFound=true;
             }
             if ( (isMC_GJet) && (!gPromptFound) ) continue; //reject event
             if ( (isMC_QCD) && gPromptFound ) continue; //reject event
	     if ( ( isMC_Wlnu_inclusive || isMC_Wlnu_HT100) && gPromptFound ) continue;
	     //             if ( (isMC_WGToLNuG) && (!gPromptFound) ) continue; 
	     // if ( (isMC_ZNuNuGJets) && (!gPromptFound) ) continue; 
             if ( (isMC_ZJetsToNuNu) && gPromptFound ) continue;
         }

  	 //Electroweak corrections to ZZ and WZ simulations
     	 double ewkCorrectionsWeight = 1.;
     	 double ewkCorrections_error = 0.;
     	 if(isMC_ZZ2l2nu || isMC_WZ3lnu) ewkCorrectionsWeight = EwkCorrections::getEwkCorrections(dtag, gen, ewkTable, eventInfo, ewkCorrections_error);
     	 double ewkCorrections_up = (ewkCorrectionsWeight + ewkCorrections_error)/ewkCorrectionsWeight;
         double ewkCorrections_down = (ewkCorrectionsWeight - ewkCorrections_error)/ewkCorrectionsWeight;

       	 //final event weight
       	 weight *= ewkCorrectionsWeight;

				 //LO to NLO k-factor for ZNuNuGamma (ref: fig 16 (bottom right) of http://link.springer.com/article/10.1007%2FJHEP02%282016%29057)
				 //FIXME Careful, this has to be removed once me move to a NLO version of this sample
				 double kFactor_ZNuNuGWeight = 1.;
				 if(isMC_ZNuNuGJets){
				 	 //reconstruct the gen transverse energy
				 	 reco::GenParticleCollection genNeutrinosFromZ;
					 for (unsigned int i =0; i < gen.size(); i++){
      		   reco::GenParticle genParticle = gen[i];
             if(fabs(genParticle.pdgId())==12 || fabs(genParticle.pdgId())==14 || fabs(genParticle.pdgId())==16){
               if(fabs(genParticle.mother()->pdgId())==23 && genParticle.mother()->status()==62) genNeutrinosFromZ.push_back(genParticle); //neutrino originating directly from Z boson
             }
           }
           std::sort(genNeutrinosFromZ.begin(), genNeutrinosFromZ.end(), utils::sort_CandidatesByPt);
           if(genNeutrinosFromZ.size() < 2) continue;
           LorentzVector genZnunuBoson;
           genZnunuBoson = genNeutrinosFromZ[0].p4() + genNeutrinosFromZ[1].p4(); //Z from neutrinos at gen lvl
				 	
				 	 //Apply LO to NLO k-factor for ZNuNuGamma (ref: fig 16 (bottom right) of http://link.springer.com/article/10.1007%2FJHEP02%282016%29057)
				 	 if(      genZnunuBoson.Pt() > 960 ) kFactor_ZNuNuGWeight = 2.05;
				 	 else if( genZnunuBoson.Pt() > 920 ) kFactor_ZNuNuGWeight = 2.10;
				 	 else if( genZnunuBoson.Pt() > 880 ) kFactor_ZNuNuGWeight = 2.13;
				 	 else if( genZnunuBoson.Pt() > 800 ) kFactor_ZNuNuGWeight = 2.16;
				 	 else if( genZnunuBoson.Pt() > 440 ) kFactor_ZNuNuGWeight = 2.20;
				 	 else if( genZnunuBoson.Pt() > 400 ) kFactor_ZNuNuGWeight = 2.16;
				 	 else if( genZnunuBoson.Pt() > 360 ) kFactor_ZNuNuGWeight = 2.13;
				 	 else if( genZnunuBoson.Pt() > 320 ) kFactor_ZNuNuGWeight = 2.07;
				 	 else if( genZnunuBoson.Pt() > 280 ) kFactor_ZNuNuGWeight = 2.03;
				 	 else if( genZnunuBoson.Pt() > 240 ) kFactor_ZNuNuGWeight = 1.96;
				 	 else if( genZnunuBoson.Pt() > 200 ) kFactor_ZNuNuGWeight = 1.90;
				 	 else if( genZnunuBoson.Pt() > 160 ) kFactor_ZNuNuGWeight = 1.75;
				 	 else if( genZnunuBoson.Pt() > 120 ) kFactor_ZNuNuGWeight = 1.50;
				 	 else if( genZnunuBoson.Pt() > 100 ) kFactor_ZNuNuGWeight = 1.32;
				   else kFactor_ZNuNuGWeight = 1.;
				 }
				 weight *= kFactor_ZNuNuGWeight;

    	 //NNLO corrections on ZZ2l2nu
    	 double ZZ_NNLOcorrectionsWeight =1.;
	 double mzz = - 404; // will be filled by getNNLOCorrections
	 if(isMC_ZZ2l2nu) ZZ_NNLOcorrectionsWeight = ZZatNNLO::getNNLOCorrections(dtag, gen, ZZ_NNLOTable, mzz);

	 //final event weight
	 if(isMC_ZZ2l2nu) mon.fillHisto("mzz", "qqZZ_atNLO", mzz, weight);
	 weight *= ZZ_NNLOcorrectionsWeight;
	 if(isMC_ZZ2l2nu) mon.fillHisto("mzz", "qqZZ_atNNLO", mzz, weight);

         //
         //
         // BELOW FOLLOWS THE ANALYSIS OF THE MAIN SELECTION WITH N-1 PLOTS
         //
         //

         ///////////////////////////////////////////////////////////////////////////////////////////////////////////
         ///////////////////////////////////////////                       /////////////////////////////////////////
         ///////////////////////////////////////////   LEPTON ANALYSIS     /////////////////////////////////////////
         ///////////////////////////////////////////                       /////////////////////////////////////////
         ///////////////////////////////////////////////////////////////////////////////////////////////////////////

         //start by merging electrons and muons
         std::vector<patUtils::GenericLepton> leptons;
         for(size_t l=0;l<electrons.size();l++){leptons.push_back(patUtils::GenericLepton(electrons[l]));}
         for(size_t l=0;l<muons    .size();l++){leptons.push_back(patUtils::GenericLepton(muons    [l]));}
         std::sort(leptons.begin(),   leptons.end(), utils::sort_CandidatesByPt);

//         std::vector<patUtils::GenericLepton> selLeptons, extraLeptons;
         std::map<string, std::vector<patUtils::GenericLepton> > selLeptonsVar;
         std::map<string, std::vector<patUtils::GenericLepton> > extraLeptonsVar;
         std::vector<patUtils::GenericLepton> self_Leptons;
         
         LorentzVector muDiff(0,0,0,0);
         LorentzVector elDiff(0,0,0,0);
	 LorentzVector elDiff_forMET(0,0,0,0);
         for(size_t ilep=0; ilep<leptons.size(); ilep++){
             bool passKin(true),passId(true),passIso(true),pass_Muon_Iso(true);
             bool passLooseLepton(true), passSoftMuon(true), passSoftElectron(true), passVetoElectron(true);
             int lid=leptons[ilep].pdgId();

             //no need for charge info any longer
             lid=abs(lid);
             TString lepStr( lid==13 ? "mu" : "e");

             reco::Vertex& vertex = vtx[0];
             std::vector<pat::PackedCandidate> PFpartVect = *PFparticles;

             //Cut based identification
             if(is2016MC || is2016data){
                 passId = lid==11?patUtils::passId(leptons[ilep].el, vtx[0], patUtils::llvvElecId::Tight, patUtils::CutVersion::ICHEP16Cut) : patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::tkHighPT, patUtils::CutVersion::ICHEP16Cut);
                 passLooseLepton &= lid==11?patUtils::passId(leptons[ilep].el, vtx[0], patUtils::llvvElecId::Loose, patUtils::CutVersion::ICHEP16Cut) : patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Loose, patUtils::CutVersion::ICHEP16Cut);
                 passSoftMuon &= lid==11? false : patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Soft, patUtils::CutVersion::ICHEP16Cut);

                 //isolation
                 //passIso = lid==11?patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::Tight, patUtils::CutVersion::ICHEP16Cut, 0) : patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::Tight, patUtils::CutVersion::ICHEP16Cut);
                 passIso = lid==11?patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::Tight, patUtils::CutVersion::ICHEP16Cut, rho) :   patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::TightAndTkRelatBoosted, patUtils::CutVersion::Moriond17Cut, PFpartVect, muons, vertex);
                 passLooseLepton &= lid==11?patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::Loose, patUtils::CutVersion::ICHEP16Cut, 0) : patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::Loose, patUtils::CutVersion::ICHEP16Cut);
	     } else {
                 passId = lid==11?patUtils::passId(leptons[ilep].el, vtx[0], patUtils::llvvElecId::Tight, patUtils::CutVersion::Spring15Cut25ns) : patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Tight, patUtils::CutVersion::Spring15Cut25ns);
                 passLooseLepton &= lid==11?patUtils::passId(leptons[ilep].el, vtx[0], patUtils::llvvElecId::Loose, patUtils::CutVersion::Spring15Cut25ns) : patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Loose, patUtils::CutVersion::Spring15Cut25ns);
                 passSoftMuon &= lid==11? false : patUtils::passId(leptons[ilep].mu, vtx[0], patUtils::llvvMuonId::Soft, patUtils::CutVersion::Spring15Cut25ns);

                 //isolation
                 passIso = lid==11?patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::Tight, patUtils::CutVersion::Spring15Cut25ns, 0) : patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::Tight, patUtils::CutVersion::Spring15Cut25ns);
                 passLooseLepton &= lid==11?patUtils::passIso(leptons[ilep].el,  patUtils::llvvElecIso::Loose, patUtils::CutVersion::Spring15Cut25ns, 0) : patUtils::passIso(leptons[ilep].mu,  patUtils::llvvMuonIso::Loose, patUtils::CutVersion::Spring15Cut25ns);

	     }


             //apply muon corrections
             if(abs(lid)==13 && passIso && passId){
                 passSoftMuon=false;
                 if(is2015MC || is2015data){
                   if(muCor2015){
                     float qter;
                     TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                     if(is2015MC){muCor2015->momcor_mc  (p4, lid<0 ? -1 :1, 0, qter);
                     }else if (is2015data){   muCor2015->momcor_data(p4, lid<0 ? -1 :1, 0, qter);
                     }


                     if(ev.eventAuxiliary().event()==869607902 || ev.eventAuxiliary().event()==471854508){
                        printf("\nevent = %lli %i lepton pt, eta, phi %f %f %f --> %f %f %f\n", ev.eventAuxiliary().event(), int(ilep), leptons[ilep].pt(),leptons[ilep].eta(),leptons[ilep].phi(), p4.Pt(),p4.Eta(),p4.Phi() );
                     }

                     muDiff -= leptons[ilep].p4();
                     leptons[ilep].setP4(LorentzVector(p4.Px(),p4.Py(),p4.Pz(),p4.E() ) );
                     muDiff += leptons[ilep].p4();
                   }
                 }
                 if(is2016MC || is2016data){
                   if(muCor2016){
                     float qter =1.0;
                     TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                     int ntrk = leptons[ilep].mu.innerTrack()->hitPattern().trackerLayersWithMeasurement();
                     if(is2016MC){muCor2016->momcor_mc  (p4, lid<0 ? -1 :1, ntrk, qter);
                     }else if (is2016data){   muCor2016->momcor_data(p4, lid<0 ? -1 :1, 0, qter);
                     }


                     muDiff -= leptons[ilep].p4();
                     leptons[ilep].setP4(LorentzVector(p4.Px(),p4.Py(),p4.Pz(),p4.E() ) );
                     muDiff += leptons[ilep].p4();
                   }
                 }
               }

             //apply electron corrections
             if(abs(lid)==11  && passIso && passId){
                elDiff -= leptons[ilep].p4();
                if(fabs(leptons[ilep].el.superCluster()->eta()) < 1.479)elDiff_forMET -= leptons[ilep].p4()*0.006;
                else elDiff_forMET -= leptons[ilep].p4()*0.015;
                //if(!isMC){
                //	utils::cmssw::SlewRateCorrection(ev,leptons[ilep].el);
               	 // leptons[ilep] = patUtils::GenericLepton(leptons[ilep].el); //recreate the generic lepton to be sure that the p4 is ok
               // }
                if (isMC || is2015data || is2016data){
                const EcalRecHitCollection* recHits = (leptons[ilep].el.isEB()) ? recHitCollectionEBHandle.product() : recHitCollectionEEHandle.product();
                unsigned int gainSeed = patUtils::GainSeed(leptons[ilep].el,recHits);

                 if(!isMC){

                 double scale_corr=eScaler.ScaleCorrection(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(), leptons[ilep].el.et(),gainSeed);
                  //At this point, the new data energy will be:
                 // E_new=E_old*(scale_corr);
                  TLorentzVector p4(leptons[ilep].el.px(),leptons[ilep].el.py(),leptons[ilep].el.pz(),leptons[ilep].el.energy());
                  leptons[ilep].el.setP4(LorentzVector(p4.Px()*scale_corr,p4.Py()*scale_corr,p4.Pz()*scale_corr,p4.E()*scale_corr ) ); 
                  leptons[ilep] = patUtils::GenericLepton(leptons[ilep].el); //recreate the generic lepton to be sure that the p4 is ok
                 }
             if(isMC){
                 double sigma=eScaler.getSmearingSigma(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(), leptons[ilep].el.et(),gainSeed,0,0);
                //Put the last two inputs at 0,0 for the nominal value of sigma
                //Now smear the MC energy
                  TRandom3 *rgen_ = new TRandom3(0);
                  double smearValue = rgen_->Gaus(1, sigma) ;
                  TLorentzVector p4(leptons[ilep].el.px(),leptons[ilep].el.py(),leptons[ilep].el.pz(),leptons[ilep].el.energy());
                  leptons[ilep].el.setP4(LorentzVector(p4.Px()*smearValue,p4.Py()*smearValue,p4.Pz()*smearValue,p4.E()*smearValue ) ); 
                  leptons[ilep] = patUtils::GenericLepton(leptons[ilep].el); //recreate the generic lepton to be sure that the p4 is ok
             }


                	//ElectronEnCorrector.calibrate(leptons[ilep].el, ev.eventAuxiliary().run(), edm::StreamID::invalidStreamID());
                	//leptons[ilep] = patUtils::GenericLepton(leptons[ilep].el); //recreate the generic lepton to be sure that the p4 is ok
                }
                elDiff += leptons[ilep].p4();
                if(fabs(leptons[ilep].el.superCluster()->eta()) < 1.479)elDiff_forMET += leptons[ilep].p4()*0.006;
                else elDiff_forMET += leptons[ilep].p4()*0.015;
             }

              //kinematics
             float leta = fabs(lid==11 ?  leptons[ilep].el.superCluster()->eta() : leptons[ilep].eta());
             if(leta> (lid==11 ? 2.5 : 2.4) )            passKin=false;
             if(lid==11 && (leta>1.4442 && leta<1.5660)) passKin=false;
             passLooseLepton &= passKin;
             passSoftMuon    &= passKin;
             if(lid==13){
               if(leptons[ilep].pt()<20) passLooseLepton=false;
               if(leptons[ilep].pt()<20)  passSoftMuon=false;
             }else if(lid==11){
               if(leptons[ilep].pt()<20) passLooseLepton=false;
             }
             if(leptons[ilep].pt()<20) passKin=false;

             
             if(lid==13)//muon iso selction
             {
                 //if(leptons[ilep].trackIso()>0.25) pass_Muon_Iso = false;
                 if(!pass_Muon_Iso) continue;
                 //cout<<"hello"<<endl;
                 mon.fillHisto("muonIso",  "Hellohello"  , leptons[ilep].mu.trackIso(),weight);
             }
             if(lid==13)//muon iso selction
             {
                 if(leptons[ilep].mu.trackIso()>=0.25) pass_Muon_Iso = false;
                 if(!pass_Muon_Iso) continue;
                 mon.fillHisto("muonIso",  "Hellohello_after_cut"  , leptons[ilep].mu.trackIso(),weight);
             }
             
             patUtils::GenericLepton varLep_my = leptons[ilep];
             if (passId && passIso){
                 if(leptons[ilep].pt()<20) passKin = false;
                 if(passKin){self_Leptons.push_back(varLep_my);}
             }
             
             
          for(unsigned int ivar=0;ivar<eleVarNames.size();ivar++){
	        if (abs(lid)==11) { //if electron
                const EcalRecHitCollection* recHits = (leptons[ilep].el.isEB()) ? recHitCollectionEBHandle.product() : recHitCollectionEEHandle.product();
                unsigned int gainSeed = patUtils::GainSeed(leptons[ilep].el,recHits);
                  patUtils::GenericLepton varLep = leptons[ilep];
                  if(ivar==1) { //stat electron up
                      double error_scale=0.0;
	              if(leptons[ilep].pt()>5.0)error_scale = eScaler.ScaleCorrectionUncertainty(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(),leptons[ilep].el.et(),gainSeed,bit_stat);
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*(1+error_scale),p4.Py()*(1+error_scale),p4.Pz()*(1+error_scale),p4.E()*(1+error_scale) ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==2) { //stat electron down
                      double error_scale=0.0;
	              if(leptons[ilep].pt()>5.0)error_scale = eScaler.ScaleCorrectionUncertainty(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(),leptons[ilep].el.et(),gainSeed,bit_stat);
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*(1-error_scale),p4.Py()*(1-error_scale),p4.Pz()*(1-error_scale),p4.E()*(1-error_scale) ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==3) { //systematic electron up
                      double error_scale=0.0;
	              if(leptons[ilep].pt()>5.0)error_scale = eScaler.ScaleCorrectionUncertainty(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(),leptons[ilep].el.et(),gainSeed,bit_syst);
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*(1+error_scale),p4.Py()*(1+error_scale),p4.Pz()*(1+error_scale),p4.E()*(1+error_scale) ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==4) { //systematic electron down
                      double error_scale=0.0;
	              if(leptons[ilep].pt()>5.0)error_scale = eScaler.ScaleCorrectionUncertainty(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(),leptons[ilep].el.et(),gainSeed,bit_syst);
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*(1-error_scale),p4.Py()*(1-error_scale),p4.Pz()*(1-error_scale),p4.E()*(1-error_scale) ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==5) { //gain switch electron up
                      double error_scale=0.0;
	              if(leptons[ilep].pt()>5.0)error_scale = eScaler.ScaleCorrectionUncertainty(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(),leptons[ilep].el.et(),gainSeed,bit_gain);
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*(1+error_scale),p4.Py()*(1+error_scale),p4.Pz()*(1+error_scale),p4.E()*(1+error_scale) ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==6) { //gain switch electron down
                      double error_scale=0.0;
	              if(leptons[ilep].pt()>5.0)error_scale = eScaler.ScaleCorrectionUncertainty(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(),leptons[ilep].el.et(),gainSeed,bit_gain);
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*(1-error_scale),p4.Py()*(1-error_scale),p4.Pz()*(1-error_scale),p4.E()*(1-error_scale) ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==7) { //rho resolution Electron up
		     double smearValue = 1.0;
                     if(leptons[ilep].pt()>5.0) {
			double sigma=eScaler.getSmearingSigma(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(), leptons[ilep].el.et(),gainSeed,1,0);
                        TRandom3 *rgen_ = new TRandom3(0);
                        smearValue = rgen_->Gaus(1, sigma) ;
                       }
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*smearValue,p4.Py()*smearValue,p4.Pz()*smearValue,p4.E()*smearValue ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==8) { //rho resolution Electron down
		     double smearValue = 1.0;
                     if(leptons[ilep].pt()>5.0) {
			double sigma=eScaler.getSmearingSigma(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(), leptons[ilep].el.et(),gainSeed,-1,0);
                        TRandom3 *rgen_ = new TRandom3(0);
                        smearValue = rgen_->Gaus(1, sigma) ;
                       }
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*smearValue,p4.Py()*smearValue,p4.Pz()*smearValue,p4.E()*smearValue ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==9) { //phi resolution Electron down
		     double smearValue = 1.0;
                     if(leptons[ilep].pt()>5.0) {
			double sigma=eScaler.getSmearingSigma(ev.eventAuxiliary().run(),leptons[ilep].el.isEB(),leptons[ilep].el.r9(), leptons[ilep].el.superCluster()->eta(), leptons[ilep].el.et(),gainSeed,0,-1);
                        TRandom3 *rgen_ = new TRandom3(0);
                        smearValue = rgen_->Gaus(1, sigma) ;
                       }
                      TLorentzVector p4(leptons[ilep].px(),leptons[ilep].py(),leptons[ilep].pz(),leptons[ilep].energy());
                      varLep.setP4(LorentzVector(p4.Px()*smearValue,p4.Py()*smearValue,p4.Pz()*smearValue,p4.E()*smearValue ));
	              if (passId && passIso){
                           if(varLep.pt()<25) passKin = false;
                           if(passKin){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
                      } else {
                               if(varLep.pt()<10) passLooseLepton = false;
                               if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep); 
	              }
                  }if(ivar==0){ // nominal
	              if(passKin && passId && passIso){selLeptonsVar[eleVarNames[ivar]].push_back(varLep);}
	              else {
	                 if(passLooseLepton) extraLeptonsVar[eleVarNames[ivar]].push_back(varLep);
	              }
	         }

              }if(abs(lid)==13){ //if muon
		   if(passKin && passId && passIso)selLeptonsVar[eleVarNames[ivar]].push_back(leptons[ilep]);
		   else if (passLooseLepton || passSoftMuon) extraLeptonsVar[eleVarNames[ivar]].push_back(leptons[ilep]);
      	      }     
            }
	}
         

         /*LorentzVector Higgsgammagamma(0,0,0,0);
         for(unsigned int iigen=0; iigen<matchingphoton.size(); iigen++){
             Higgsgammagamma += matchingphoton[iigen].p4();
         }
         HIGGS_MASS_gammagamma = Higgsgammagamma.mass();
         pho_numphotons = matchingphoton.size();*/
         
         std::vector<patUtils::GenericLepton>  selLeptons      = selLeptonsVar[""]; 
         std::vector<patUtils::GenericLepton>  extraLeptons      = extraLeptonsVar[""];
         std::vector<patUtils::GenericLepton>  sel_Leptons;
         
         //v->ll
         LorentzVector Vmass(0,0,0,0);
         float Z_mass(0);
         int id_first(0);
         int id_second(1);
         bool lepton_ID_match(false);
         bool Two_Lepton (false);
         bool Z_mass_match(false);
         if(self_Leptons.size()==2) Two_Lepton = true;

         if(!Two_Lepton) continue;
         if (Two_Lepton)
         {
             for (int ii =0;ii<self_Leptons.size();ii++)
             {
                 if(ii==0) id_first  = abs(self_Leptons[ii].pdgId());
                 
                 if(ii==1) id_second = abs(self_Leptons[ii].pdgId());
             }
             
             if(id_first == id_second) lepton_ID_match = true;
             
             if (lepton_ID_match)
             {
                 for (int jj =0;jj<self_Leptons.size();jj++)
                 {
                     Vmass += self_Leptons[jj].p4();
                 }
                 Z_mass=Vmass.mass();
                 if (Z_mass>70&&Z_mass<110) Z_mass_match = true;
             }
        }

         if (!(Two_Lepton && lepton_ID_match && Z_mass_match )) continue;
         
         for (int iijj =0;iijj<self_Leptons.size();iijj++)
         {
             patUtils::GenericLepton varLep_match = self_Leptons[iijj];
             sel_Leptons.push_back(varLep_match);
             
         }

          std::sort(sel_Leptons.begin(),   sel_Leptons.end(), utils::sort_CandidatesByPt);
         
         for (int i =0;i<sel_Leptons.size();i++)
         {
             if (sel_Leptons[i].pdgId()==13) mon.fillHisto("Lepton_pT",     "Muon", sel_Leptons[i].pt(),  weight);
             if (sel_Leptons[i].pdgId()==11) mon.fillHisto("Lepton_pT",     "Electon", sel_Leptons[i].pt(),  weight);
             
         }
         
         
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         /////////////////      PHOTON ANALYSIS   (photons is defines before before)         //////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         //////////////////////////////////////////////////////////////////////////////////////////////////////////
         

         
         //start by merging photon
      //   std::vector<pat::Photon> *PATPhotons = new std::vector<pat::Photon>();
      //   for(size_t l=0;l<photons.size();l++){PATPhotons->push_back(photons[l]);}
      //   std::sort(PATPhotons->begin(),   PATPhotons->end(), utils::sort_CandidatesByPt);
        
         pat::PhotonCollection PATPhotons;
         for(size_t l=0;l<photons.size();l++){PATPhotons.push_back(photons[l]);}
         std::sort(PATPhotons.begin(),   PATPhotons.end(), utils::sort_CandidatesByPt);
         
         ////reco::GenParticleCollection sel_Photons, extra_Photons;
         
         
         
         
         
         pat::PhotonCollection sel_Photons,   extra_Photons ;
         //PRE-SELECTION based to pt value
         for(unsigned int j=0; j<PATPhotons.size(); j++)
         {
             if(PATPhotons[j].pt() > 10) sel_Photons.push_back(PATPhotons[j]);
         }
         
         std::sort(sel_Photons.begin(),   sel_Photons.end(), utils::sort_CandidatesByPt);
         //if(sel_Photons.size() != 2) continue;
         //if(sel_Photons.size() < 2) continue;
         
         bool two_sel_Photons(false);
         if(sel_Photons.size() == 2) two_sel_Photons = true;
         
         
         
         
         
         
         /* std::vector<patUtils::GenericLepton> leptons;
         for(size_t l=0;l<electrons.size();l++){leptons.push_back(patUtils::GenericLepton(electrons[l]));}
         for(size_t l=0;l<muons    .size();l++){leptons.push_back(patUtils::GenericLepton(muons    [l]));}
         std::sort(leptons.begin(),   leptons.end(), utils::sort_CandidatesByPt);
         
         //         std::vector<patUtils::GenericLepton> selLeptons, extraLeptons;
         std::map<string, std::vector<patUtils::GenericLepton> > selLeptonsVar;
         std::map<string, std::vector<patUtils::GenericLepton> > extraLeptonsVar;
         LorentzVector muDiff(0,0,0,0);
         LorentzVector elDiff(0,0,0,0);
         LorentzVector elDiff_forMET(0,0,0,0);

         */
       /*  reco::PhotonCollection selPhotons;
         int nPho55=0; int nPho100=0;
         for(size_t ipho=0; ipho<photons.size(); ipho++){
            reco::Photon photon = photons[ipho];
            mon.fillHisto("phopt", "trg", photon.pt(), weight);
            mon.fillHisto("phoeta", "trg", photon.eta(), weight);    */
         
         
         
         
      
         double dR = 9999999.;
         float minDr=10000.;
         int iMatch=-1;
         bool matched=false;
         double nmatch = 0;
         
         
         
         /*
         //reconstruct the gen transverse energy
         reco::GenParticleCollection genPhotonFromHiggs;
         for (unsigned int i =0; i < gen.size(); i++){
             reco::GenParticle genParticlePho = gen[i];
             if(fabs(genParticlePho.pdgId())==22 && fabs(genParticlePho.status())==1){
                 if(fabs(genParticlePho.mother()->pdgId())==25 && genParticlePho.mother()->status()==1)
                     genPhotonFromHiggs.push_back(genParticlePho); //photon originating directly from higgs boson
             }
         }
         std::sort(genPhotonFromHiggs.begin(), genPhotonFromHiggs.end(), utils::sort_CandidatesByPt);
         if(genPhotonFromHiggs.size() < 2) continue;
         LorentzVector genHiggsBoson;
         genHiggsBoson = genPhotonFromHiggs[0].p4() + genPhotonFromHiggs[1].p4(); //higgs from photon*/
         
         //reconstruct the gen transverse energy
         reco::GenParticleCollection genPhotonFromHiggs;
         for (unsigned int i =0; i < gen.size(); i++){
             reco::GenParticle genParticlePho = gen[i];
             if(!(fabs(genParticlePho.pdgId())==22 && fabs(genParticlePho.status())==1))    continue;
             //if(genParticlePho.mother()!= nullptr && !(fabs(genParticlePho.mother()->pdgId())==25))    continue;
             //if(genParticlePho.mother()== nullptr || fabs(genParticlePho.mother()->pdgId())==25)   cout<<"pass"<<endl;
             genPhotonFromHiggs.push_back(genParticlePho); //photon originating directly from higgs boson
             
         }
         std::sort(genPhotonFromHiggs.begin(), genPhotonFromHiggs.end(), utils::sort_CandidatesByPt);
         if(genPhotonFromHiggs.size() < 2) continue;
         //cout<<"nice to meet you!"<<endl;
         
         LorentzVector genHiggsBoson(0,0,0,0);
         genHiggsBoson = genPhotonFromHiggs[0].p4() + genPhotonFromHiggs[1].p4(); //higgs from photon
         
//         HIGGS_MASS_A = genHiggsBoson.mass();
//         tree1->Fill();
         //if( sel_Photons[0].pt()<30 ) continue;
         //if( sel_Photons[1].pt()<20 ) continue;
         
         //if (sel_Photons[0].pt()<30) continue;
         //if (sel_Photons[1].pt()<20) continue;
         
         
         
         
         //if(two_sel_Photons)
         //{
         //    mon.fillHisto("PHOTON_pT",     "Mu_Led", sel_Photons[0].pt(),  weight);
         //    mon.fillHisto("PHOTON_pT",  "Mu_SubLed", sel_Photons[1].pt(),  weight);
         //    mon.fillHisto("PHOTON_Eta",    "Mu_Led", sel_Photons[0].eta(), weight);
         //    mon.fillHisto("PHOTON_Eta", "Mu_SubLed", sel_Photons[1].eta(), weight);
             
         //}
         
        // mon.fillHisto("PHOTON_pT",     "Third", sel_Photons[2].pt(),  weight);
        // mon.fillHisto("PHOTON_pT",     "Forth", sel_Photons[3].pt(),  weight);
        // mon.fillHisto("PHOTON_Eta",    "Third", sel_Photons[2].eta(), weight);
        // mon.fillHisto("PHOTON_Eta",    "Forth", sel_Photons[3].eta(), weight);
         
         
         
         //if(sel_Photons[0].pt()<30) continue;
         //if(sel_Photons[1].pt()<20) continue;
         
         
         
         
      
       //  std::vector<pat::Photon> * PATPhotons = new std::vector<pat::Photon>();
        // edm::View<pat::Photon>::const_iterator itPhoton;
         pat::PhotonCollection matchingphoton;
             pat::PhotonCollection selPhotons;
             int nPho55=0; int nPho100=0;
             for(size_t ipho=0; ipho<sel_Photons.size(); ipho++){
                 pat::Photon photon = sel_Photons[ipho];
                 
                 if (ipho==0 && photon.pt()<=30) continue;
                 if (ipho==1 && photon.pt()<=20) continue;
                 
                 if (abs(photon.superCluster()->position().eta())>2.5) continue;
                 if (abs(photon.superCluster()->position().eta())<1.566&&abs(photon.superCluster()->position().eta())>1.442)continue;
                 
                 if (photon.r9()<=0.8) continue;
                 if ( photon.chargedHadronIso()>=20&&(photon.chargedHadronIso()/photon.pt()>=0.3) ) continue;
                 
               
                 
                 
                // std::vector<pat::Photon::ShowerShape>  ShowerShapepho;
                // *ShowerShapepho = photons;
                 
                 
                 /*
                 pat::Photon::ShowerShape ShowerShapepho;
                 pat::Photon *phot1 = *ShowerShapepho;
                 *phot1 = photon;*/
                 
                 //pat::Photon::ShowerShape *ShowerShapepho;
                 //pat::Photon *phot1;
                 
              //   pat::Photon *photonss = &photons[ipho];
                 
                 
                 //pat::Photon::ShowerShape *ShowerShapepho = &(reco::Photon::ShowerShape)photon;
                 mon.fillHisto("phopt", "trg", photon.pt(), weight);
                 mon.fillHisto("phoeta", "trg", photon.eta(), weight);

                 
                 
                 
                 
                 /*
                 double largest_pt;
                 double second_largest_pt;
                 
                 if (ipho ! = (sel_Photons.size()-1))
                 {
                     largest_pt = sel_Photons[ipho].pt();
                     if (sel_Photons[ipho].pt()>sel_Photons[ipho+1].pt())
                         
                         
                         
                 }
                 */
                 
                 //int FIRST  = rand()%2;
                 //int SECOND = (FIRST+1)%2;
                 
                 
                 
                 
                 
                 
                 // Ptr<pat::Photon> pho = photons.ptrAt( ipho );
                 
            // Ptr<pat::Photon> pho;
             //Ptr<pat::Photon> ptrAt();
              //   View<pat::Photon>::ptrAt();
            // pho = photons.ptrAt( ipho );

                // std::vector<pat::Photon> * PATPhotons = new std::vector<pat::Photon>();
                // for (edm::View<pat::Photon>::const_iterator itPhoton = photons.begin(); itPhoton != photons.end(); itPhoton++) {
                          // construct the Photon from the ref -> save ref to original object
                 
             //    unsigned int idx = itPhoton - photons.begin();
             //             edm::RefToBase<pat::Photon> photonRef = photons.refAt(idx);
             //             edm::Ptr<pat::Photon> photonPtr = photons.ptrAt(idx);
             //             Photon aPhoton(photonRef);
              //            auto phoPtr = photons.ptrAt(idx);
                 
                 //match
                 
                /*
                 //reconstruct the gen transverse energy
                 reco::GenParticleCollection genPhotonFromHiggs;
                 for (unsigned int i =0; i < gen.size(); i++){
                     reco::GenParticle genParticlePho = gen[i];
                     if(fabs(genParticlePho.pdgId())==22 && fabs(genParticlePho.status())==1){
                         if(fabs(genParticlePho.mother()->pdgId())==25 && genParticlePho.mother()->status()==1)
                             genPhotonFromHiggs.push_back(genParticlePho); //photon originating directly from higgs boson
                     }
                 }
                 std::sort(genPhotonFromHiggs.begin(), genPhotonFromHiggs.end(), utils::sort_CandidatesByPt);
                 if(genPhotonFromHiggs.size() < 2) continue;
                 LorentzVector genHiggsBoson;
                 genHiggsBoson = genPhotonFromHiggs[0].p4() + genPhotonFromHiggs[1].p4(); //higgs from photon*/

                 ////////////////////////////////////////////
                 ////////////////////////////////////////////
                 /*
                 ///prcselection of photon
                 if(photon.isEB())
                 {
                     if(photon.r9()>0.85)
                     {
                         if(  photon.hadronicOverEm()>0.08||photon.hadronicOverEm()==0.08||photon.full5x5_r9()<0.5||photon.full5x5_r9()==0.5 )
                             continue;
                     }
                     if(photon.r9()<0.85&&photon.r9()==0.85)
                     {
                         if(  photon.hadronicOverEm()>0.08||photon.hadronicOverEm()==0.08||photon.full5x5_sigmaIetaIeta()>0.015||photon.full5x5_sigmaIetaIeta()==0.015||photon.full5x5_r9()<0.5||photon.full5x5_r9()==0.5||photon.photonIso()>4.0||photon.photonIso()==4.0||photon.trkSumPtSolidConeDR04()>6.0||photon.trkSumPtSolidConeDR04()==6.0) continue;

                     }
                 }
                 if(photon.isEE())
                 {
                     if(photon.r9()>0.90)
                     {
                         if(  photon.hadronicOverEm()>0.08||photon.hadronicOverEm()==0.08||photon.full5x5_r9()<0.8||photon.full5x5_r9()==0.8 )
                             continue;
                     }
                     if(photon.r9()<0.90&&photon.r9()==0.90)
                     {
                         if(  photon.hadronicOverEm()>0.08||photon.hadronicOverEm()==0.08||photon.full5x5_sigmaIetaIeta()>0.035||photon.full5x5_sigmaIetaIeta()==0.035||photon.full5x5_r9()<0.8||photon.full5x5_r9()==0.8||photon.photonIso()>4.0||photon.photonIso()==4.0||photon.trkSumPtSolidConeDR04()>6.0||photon.trkSumPtSolidConeDR04()==6.0) continue;
                         
                     }
                 }

                 */
                 ////////////////////////////////////////////
                 ////////////////////////////////////////////
                 ////////////////////////////////////////////
                 ////////////////////////////////////////////
                 ///another expression of preselection of photon
                 
                 bool EBHighR9(false);
                 bool EBLowR9(false);
                 bool EEHighR9(false);
                 bool EELowR9(false);
                 if(photon.isEB())
                 {
                     if(photon.r9()>0.85)
                     {
                         if(  photon.hadronicOverEm()<0.08&&photon.full5x5_r9()>0.5 )
                             EBHighR9 = true;
                     }
                     if(photon.r9()<=0.85)
                     {
                         if(  photon.hadronicOverEm()<0.08&&photon.full5x5_sigmaIetaIeta()<0.015&&photon.full5x5_r9()>0.5&&photon.photonIso()<4.0&&photon.trkSumPtSolidConeDR04()<6.0 )
                             EBLowR9 = true;
                         
                     }
                 }
                 if(photon.isEE())
                 {
                     if(photon.r9()>0.90)
                     {
                         if(  photon.hadronicOverEm()<0.08&&photon.full5x5_r9()>0.8 )
                             EEHighR9 = true;
                     }
                     if(photon.r9()<=0.90)
                     {
                         if(  photon.hadronicOverEm()<0.08&&photon.full5x5_sigmaIetaIeta()<0.035&&photon.full5x5_r9()>0.8&&photon.photonIso()<4.0&&photon.trkSumPtSolidConeDR04()<6.0)
                             EELowR9 = true;
                         
                     }
                 }
                 
                 
                 bool pass_Pre(false);
                 pass_Pre = EBHighR9||EBLowR9||EEHighR9||EELowR9 ;
                 
                 
                 mon.fillHisto("pass_Preseletion",   "EBHighR9_" ,EBHighR9, weight);
                 mon.fillHisto("pass_Preseletion",   "EBLowR9_"  ,EBLowR9,  weight);
                 mon.fillHisto("pass_Preseletion",   "EEHighR9_" ,EEHighR9, weight);
                 mon.fillHisto("pass_Preseletion",   "EELowR9_"  ,EELowR9,  weight);
                 mon.fillHisto("pass_Preseletion",   "FOUR_"     ,pass_Pre, weight);

                 if ( !(EBHighR9||EBLowR9||EEHighR9||EELowR9)  ) continue;
                 
                 
                 
                 
                 
                 
                 
                 for( size_t iipho = 0; iipho < genPhotonFromHiggs.size(); iipho++ ) {
                     
                     double dphi = genPhotonFromHiggs[iipho].momentum().phi()-photon.phi();
                     if (std::fabs(dphi)>TMath::Pi())
                     { dphi = dphi < 0? (2*TMath::Pi()) + dphi : dphi - 2*TMath::Pi() ; }
                     double deta = genPhotonFromHiggs[iipho].momentum().eta()-photon.eta();
                     
                     dR = sqrt(pow((deta),2) + pow(dphi,2));
                     if ( dR<0.1 && dR< minDr ) {
                         minDr=dR;
                         iMatch=ipho;
                     }
                 }
                 
                 /*
                 for( size_t iipho = 0; iipho < PATPhotons.size(); iipho++ ) {
                 
                        double dphi = PATPhotons[iipho].superCluster()->position().phi()-photon.phi();
                        if (std::fabs(dphi)>TMath::Pi())
                                { dphi = dphi < 0? (2*TMath::Pi()) + dphi : dphi - 2*TMath::Pi() ; }
                        double deta = PATPhotons[iipho].superCluster()->position().eta()-photon.eta();
                 
                        dR = sqrt(pow((deta),2) + pow(dphi,2));
                     if ( dR<0.1 && dR< minDr ) {
                                  minDr=dR;
                                  iMatch=ipho;
                     }
                 }*/
            
                 
                 if ( iMatch >-1) { matched=true; nmatch++;}
                 if ( ! matched) continue;
                
                 ///////lepton and photon matching///20171205
                 bool pass_Lep_pho(false);
                 //bool pass_gsfLep_pho(false);
                 double minDRlg_Lep_pho(9999.);
                 for(size_t ilep=0; ilep<sel_Leptons.size(); ilep++)
                 {
                     minDRlg_Lep_pho=TMath::Min(minDRlg_Lep_pho,deltaR( photon.p4(), sel_Leptons[ilep].p4()));
                     if (sel_Leptons[ilep].pdgId()==11 && minDRlg_Lep_pho>1.0 )pass_Lep_pho = true;
                     if (sel_Leptons[ilep].pdgId()==13 && minDRlg_Lep_pho>0.5 )pass_Lep_pho = true;
                 }
                 //if (sel_Leptons[ilep].pdgId()==11) mon.fillHisto("Pass_deltaR", "pass_ele_pho",  pass_Lep_pho, weight);
                 //if (sel_Leptons[ilep].pdgId()==13) mon.fillHisto("Pass_deltaR", "pass_muon_pho", pass_Lep_pho, weight);
                 mon.fillHisto("Pass_deltaR", "pass_Lep_pho", pass_Lep_pho, weight);
                 if(!pass_Lep_pho) continue;
           
                 
                  ///////gsf and photon matching///20171205
                 if (photon.passElectronVeto())
                 {
                     double DeltaR_gsfLep_pho(9999.);
                     double min_DeltaR_gsfLep_pho(9999.);
                     bool pass_gsfLep_pho(false);
                     for(size_t ilep=0; ilep<sel_Leptons.size(); ilep++)
                     {
                         //if (sel_Leptons[ilep].pdgId()==13) {pass_gsfLep_pho = true; break; }
                         if (sel_Leptons[ilep].pdgId()==11)
                         {
                             double dphi = sel_Leptons[ilep].el.momentum().phi()-photon.phi();
                             if (std::fabs(dphi)>TMath::Pi())
                             { dphi = dphi < 0? (2*TMath::Pi()) + dphi : dphi - 2*TMath::Pi() ; }
                             double deta = sel_Leptons[ilep].el.momentum().eta()-photon.eta();
                             
                             DeltaR_gsfLep_pho = sqrt(pow((deta),2) + pow(dphi,2));
                             
                             if (DeltaR_gsfLep_pho < min_DeltaR_gsfLep_pho) DeltaR_gsfLep_pho = min_DeltaR_gsfLep_pho;
                             

                             
                             //minDRlg_gsfLep_pho=TMath::Min(minDRlg_gsfLep_pho,deltaR( photon.p4(), sel_Leptons[ilep].el.gsfTrack().p4()));
                         }
                     }
                     if ( min_DeltaR_gsfLep_pho>0.4 )pass_gsfLep_pho = true;
                     
                     mon.fillHisto("Pass_deltaR","pass_gsf_pho",pass_gsfLep_pho,weight);
                     if(!pass_gsfLep_pho) continue;
                 }
                 
                 
                 
                 matchingphoton.push_back(sel_Photons[ipho]);
                 std::sort(matchingphoton.begin(),   matchingphoton.end(), utils::sort_CandidatesByPt);
                 
                 //number_of_vertex = vtx.size();
                 //tree1->Fill();
                // pat::Photon matchingphoton = photons[ipho];
                 
                 
                 /*
                 
                 for(size_t ilep=0; ilep<selLeptons.size(); ilep++){
                     minDRlg=TMath::Min(minDRlg,deltaR( photon.p4(), selLeptons[ilep].p4()));
                 }
                 if(minDRlg<0.1) continue;
                 
                 if(photon.pt()<55)continue;
                 if(fabs(photon.superCluster()->eta())>1.4442 ) continue;
                 if(!patUtils::passId(photon, rho, patUtils::llvvPhotonId::Tight)) continue;
                 selPhotons.push_back(photon);
                 */

            ////////////////////////////////////////////////////////////////////////////////
            ///////////////////////////////////111/////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////
                 
           
           
           /*
                string AAA[4] = {"First_","Second_","Third_","Forth_"};
                 
               
                if(ipho<4)
                {
                    mon.fillHisto("PHOTON_pT",     AAA[ipho].c_str(), sel_Photons[ipho].pt(),  weight);
                    mon.fillHisto("PHOTON_Eta",    AAA[ipho].c_str(), sel_Photons[ipho].eta(), weight);
                }
                 
                 
                 //    mon.fillHisto("PHOTON_pT",  "Mu_SubLed", sel_Photons[1].pt(),  weight);
                 //    mon.fillHisto("PHOTON_Eta", "Mu_SubLed", sel_Photons[1].eta(), weight);
                 // mon.fillHisto("PHOTON_pT",     "Third", sel_Photons[2].pt(),  weight);
                 // mon.fillHisto("PHOTON_pT",     "Forth", sel_Photons[3].pt(),  weight);
                 // mon.fillHisto("PHOTON_Eta",    "Third", sel_Photons[2].eta(), weight);
                 // mon.fillHisto("PHOTON_Eta",    "Forth", sel_Photons[3].eta(), weight);
                 
                 
                 
                 
                 
             // PhotonID Variables
             mon.fillHisto("photonEcalIso",         "",      photon.ecalRecHitSumEtConeDR04(),       weight);
             mon.fillHisto("photonHcalIso",         "",     photon.hcalTowerSumEtConeDR04(),        weight);
             mon.fillHisto("photonTrackSolidIso",   "" ,   photon.trkSumPtSolidConeDR04(),         weight);
             mon.fillHisto("photonTrackHollowIso",  "" ,  photon.trkSumPtHollowConeDR04(),        weight);
             mon.fillHisto("photonTrackCountSolid", "",  photon.nTrkSolidConeDR04(),             weight);
             mon.fillHisto("photonTrackCountHollow","",  photon.nTrkHollowConeDR04() ,           weight);
             mon.fillHisto("photonInEBEtagap",      "", photon.isEBEtaGap(),                    weight);
             mon.fillHisto("photonInEBPhigap",      "",  photon.isEBPhiGap(),                    weight);
             mon.fillHisto("photonInEERinggap",     "",  photon.isEERingGap(),                   weight);
             mon.fillHisto("photonInEEDeegap",      "", photon.isEEDeeGap(),                    weight);
             mon.fillHisto("photonInEEgap",         "",  photon.isEBEEGap(),                     weight);
             mon.fillHisto("photonR9",              "",  photon.r9() ,                             weight);
             
             
             // Photon Variables
             mon.fillHisto("photonEt",    "",    photon.et(),    weight);
             mon.fillHisto("photonEta",    "",    photon.eta(),    weight);
             mon.fillHisto("photonPhi",    "",    photon.phi(),    weight);
             mon.fillHisto("photonHoverE",    "",    photon.hadronicOverEm(),    weight);
             
             //mon.fillHisto("photonPt", "", photon.pt(), weight);
             
             mon.fillHisto("photonPt", "", photon.pt(), weight);
             
             // Photon's SuperCluster Variables
             // eta is with respect to detector (not physics) vertex,
             // thus Et and eta are different from photon.
             float superClusterEt = (photon.superCluster()->energy())/(cosh(photon.superCluster()->position().eta()));
             mon.fillHisto("photonScEt",    "",    superClusterEt,    weight);
             mon.fillHisto("photonScEta",    "",    photon.superCluster()->position().eta(),    weight);
             mon.fillHisto("photonScPhi",    "",    photon.superCluster()->position().phi(),    weight);
             mon.fillHisto("photonScEtaWidth",    "",    photon.superCluster()->etaWidth(),    weight);
                 
             
             mon.fillHisto("photonScPhiWidth",    "",    photon.superCluster()->phiWidth(),    weight);
             mon.fillHisto("photonScRawEnergy",    "",    photon.superCluster()->rawEnergy(),    weight);
             mon.fillHisto("photonScEnergy",    "",    photon.superCluster()->energy(),    weight);
            
             mon.fillHisto("photonrho",  ""  ,      rho      ,  weight);
             mon.fillHisto("photone2x2",     ""  ,  photon.showerShapeVariables().e2x2   ,  weight);
             mon.fillHisto("photone5x5",     ""  ,  photon.e5x5()   ,  weight);
             mon.fillHisto("photoneffSigmaRR",     ""  ,  photon.showerShapeVariables().effSigmaRR   ,  weight);
             mon.fillHisto("photonsigmaIetaIeta",  ""  , photon.sigmaIetaIeta()  ,  weight);
             mon.fillHisto("photonsigmaEtaEta",  ""  ,  photon.sigmaEtaEta()    ,  weight);
             mon.fillHisto("photonsigmaIphiIphi",  ""  ,  photon.showerShapeVariables().sigmaIphiIphi    ,  weight);
             mon.fillHisto("photonsigmaIetaIphi",  ""  ,  photon.showerShapeVariables().sigmaIetaIphi    ,  weight);
             mon.fillHisto("photonfull5x5_r9",  ""  ,  photon.full5x5_r9()          ,  weight);
            
             mon.fillHisto("photonr2x5",  ""  ,  photon.r2x5()    ,  weight);
             
             mon.fillHisto("photonE2x2overE5x5",  ""  ,  photon.showerShapeVariables().e2x2/photon.e5x5()  ,  weight);
             
             mon.fillHisto("photonIso",  ""  , photon.photonIso(),weight);
             mon.fillHisto("photonIsosumChargedParticlePt",  ""  , photon.sumChargedParticlePt(),weight);
             mon.fillHisto("photonchargedHadronIso",  ""  , photon.chargedHadronIso(),weight);
             mon.fillHisto("photonchargedHadronIsoWrongVtx",  ""  , photon.chargedHadronIsoWrongVtx(),weight);

                 
                 
                 
                 pho_EE = photon.isEE();
                 pho_EB = photon.isEB();
                 pho_ecalRecHitSumEtConeDR04 = photon.ecalRecHitSumEtConeDR04();
                 pho_hcalTowerSumEtConeDR04  = photon.hcalTowerSumEtConeDR04();
                 pho_trkSumPtSolidConeDR04   = photon.trkSumPtSolidConeDR04();
                 pho_trkSumPtHollowConeDR04  = photon.trkSumPtHollowConeDR04();
                 pho_nTrkSolidConeDR04       = photon.nTrkSolidConeDR04();
                 pho_nTrkHollowConeDR04      = photon.nTrkHollowConeDR04();
                 pho_r9                      = photon.r9();
                 
                 //pho__ecalRecHitSumEtConeDR04 = photon.ecalRecHitSumEtConeDR04();
                 //pho__hcalTowerSumEtConeDR04  = photon.hcalTowerSumEtConeDR04();
                 //pho__trkSumPtSolidConeDR04   = photon.trkSumPtSolidConeDR04();
                 //pho__trkSumPtHollowConeDR04  = photon.trkSumPtHollowConeDR04();
                 //pho__nTrkSolidConeDR04       = photon.nTrkSolidConeDR04();
                 //pho__nTrkHollowConeDR04      = photon.nTrkHollowConeDR04();
                 //pho__R9                      = photon.r9();
                
                 pho_Et                    = photon.et();
                 pho_Eta                   = photon.eta();
                 pho_Phi                   = photon.phi();
                 pho_HOverE                = photon.hadronicOverEm();
                 pho_Pt                    = photon.pt();
                 pho_ScEt                  = superClusterEt;
                 pho_ScEta                 = photon.superCluster()->position().eta();
                 pho_ScPhi                 = photon.superCluster()->position().phi();
                 pho_ScEtaWidth            = photon.superCluster()->etaWidth();
                 pho_ScPhiWidth            = photon.superCluster()->phiWidth();
                 pho_ScRawEnergy           = photon.superCluster()->rawEnergy();
                 pho_ScEnergy              = photon.superCluster()->energy();
                 pho_rho                   = rho;
                 pho_E2x2                  = photon.showerShapeVariables().e2x2;
                 pho_E5x5                  = photon.e5x5();
                 pho_EffSigmaRR            = photon.showerShapeVariables().effSigmaRR;
                 pho_SigmaIetaIeta         = photon.sigmaIetaIeta();
                 pho_SigmaEtaEta           = photon.sigmaEtaEta();
                 pho_SigmaIphiIphi         = photon.showerShapeVariables().sigmaIphiIphi;
                 pho_SigmaIetaIphi         = photon.showerShapeVariables().sigmaIetaIphi;
                 pho_Full5x5_R9            = photon.full5x5_r9();
                 pho_R2x5                  = photon.r2x5();
                 pho_E2x2overE5x5          = photon.showerShapeVariables().e2x2/photon.e5x5();
                 pho_PhotonIso             = photon.photonIso();
                 pho_SumChargedParticlePt  = photon.sumChargedParticlePt();
                 pho_ChargedHadronIso      = photon.chargedHadronIso();
                 //pho_ChargedHadronIsoWrongVtx      = photon.chargedHadronIsoWrongVtx();
                 
                 //float maxiso=photon.vtx[0].point()->chargedHadronIso();
                 photon.setVertex(vtx[0].position());
                 float maxiso=photon.chargedHadronIso();
                 for(int iijjkk=0;iijjkk<vtx.size();iijjkk++)
                 {
                     photon.setVertex(vtx[iijjkk].position());
                     
                     if(maxiso > photon.chargedHadronIso())
                     //maxiso = photon.setVertex(vtx[iijjkk]).chargedHadronIso();
                        // maxiso = photon.setVertex(vtx[iijjkk].position()).chargedHadronIso();
                         maxiso = photon.chargedHadronIso();
                 }
                 pho_ChargedHadronIsoWrongVtx = maxiso;
                 
                 ///tree1->Branch("recPhoton", &pho_ecalRecHitSumEtConeDR04, "pho_ecalRecHitSumEtConeDR04/F:pho_hcalTowerSumEtConeDR04:pho_trkSumPtSolidConeDR04:pho_trkSumPtHollowConeDR04:pho_nTrkSolidConeDR04:pho_nTrkHollowConeDR04:pho_r9");
                 tree1->Fill();
                 
          
                 
                 
          //   tree1->Branch("recPhoton", &photon.ecalRecHitSumEtConeDR04(), "ecalRecHitSumEtConeDR04()/F:hcalTowerSumEtConeDR04():trkSumPtSolidConeDR04():isolationHollowTrkCone:nTrkSolidCone:nTrkHollowCone:isEBGap:isEEGap:isEBEEGap:r9:et:eta:phi:hadronicOverEm");

             if (photon.isEB())
             {
                 
                 mon.fillHisto("photonScEta",           "__EB",   photon.superCluster()->position().eta(),           weight);
                 mon.fillHisto("photonEcalIso",         "__EB",      photon.ecalRecHitSumEtConeDR04(),       weight);
                 mon.fillHisto("photonHcalIso",         "__EB",     photon.hcalTowerSumEtConeDR04(),        weight);
                 mon.fillHisto("photonTrackSolidIso",   "__EB" ,   photon.trkSumPtSolidConeDR04(),         weight);
                 mon.fillHisto("photonTrackHollowIso",  "__EB" ,  photon.trkSumPtHollowConeDR04(),        weight);
                 mon.fillHisto("photonTrackCountSolid", "__EB",  photon.nTrkSolidConeDR04(),             weight);
                 mon.fillHisto("photonTrackCountHollow","__EB",  photon.nTrkHollowConeDR04() ,           weight);
                 mon.fillHisto("photonR9",              "__EB",  photon.r9() ,                             weight);
                 mon.fillHisto("photonEt",    "__EB",    photon.et(),    weight);
                 mon.fillHisto("photonEta",    "__EB",    photon.eta(),    weight);
                 mon.fillHisto("photonPhi",    "__EB",    photon.phi(),    weight);
                 mon.fillHisto("photonHoverE",    "__EB",    photon.hadronicOverEm(),    weight);
                 
                 mon.fillHisto("photonPt", "__EB", photon.pt(), weight);
                 
                 mon.fillHisto("photonScEt",    "__EB",    superClusterEt,    weight);
                 mon.fillHisto("photonScEta",    "__EB",    photon.superCluster()->position().eta(),    weight);
                 mon.fillHisto("photonScPhi",    "__EB",    photon.superCluster()->position().phi(),    weight);
                 mon.fillHisto("photonScEtaWidth",    "__EB",    photon.superCluster()->etaWidth(),    weight);
                
                 mon.fillHisto("photonScPhiWidth",    "__EB",    photon.superCluster()->phiWidth(),    weight);
                 mon.fillHisto("photonScRawEnergy",    "__EB",    photon.superCluster()->rawEnergy(),    weight);
                 mon.fillHisto("photonScEnergy",    "__EB",    photon.superCluster()->energy(),    weight);
                 
                 mon.fillHisto("photonrho",  "__EB"  ,      rho      ,  weight);
                 mon.fillHisto("photone2x2",     "__EB"  ,  photon.showerShapeVariables().e2x2   ,  weight);
                 mon.fillHisto("photone5x5",     "__EB"  ,  photon.e5x5()   ,  weight);
                 mon.fillHisto("photoneffSigmaRR",     "__EB"  ,  photon.showerShapeVariables().effSigmaRR   ,  weight);
                 mon.fillHisto("photonsigmaIetaIeta",  "__EB"  ,  photon.sigmaIetaIeta()  ,  weight);
                 mon.fillHisto("photonsigmaEtaEta",  "__EB"  ,  photon.sigmaEtaEta()    ,  weight);
                 mon.fillHisto("photonsigmaIphiIphi",  "__EB"  ,  photon.showerShapeVariables().sigmaIphiIphi    ,  weight);
                 mon.fillHisto("photonsigmaIetaIphi",  "__EB"  ,  photon.showerShapeVariables().sigmaIetaIphi    ,  weight);
                 mon.fillHisto("photonfull5x5_r9",  "__EB"  ,  photon.full5x5_r9()          ,  weight);
                 
                 mon.fillHisto("photonr2x5",  "__EB"  ,  photon.r2x5()    ,  weight);
                 
                 mon.fillHisto("photonE2x2overE5x5",  "__EB"  ,  photon.showerShapeVariables().e2x2/photon.e5x5()  ,  weight);
                 
                 
                 mon.fillHisto("photonIso",  "__EB"  , photon.photonIso(),weight);
                 mon.fillHisto("photonIsosumChargedParticlePt",  "__EB"  , photon.sumChargedParticlePt(),weight);
                 mon.fillHisto("photonchargedHadronIso",  "__EB"  , photon.chargedHadronIso(),weight);
                 mon.fillHisto("photonchargedHadronIsoWrongVtx",  "__EB"  , photon.chargedHadronIsoWrongVtx(),weight);
                 
                 
                 pho_ecalRecHitSumEtConeDR04 = photon.ecalRecHitSumEtConeDR04();
                 pho_hcalTowerSumEtConeDR04  = photon.hcalTowerSumEtConeDR04();
                 pho_trkSumPtSolidConeDR04   = photon.trkSumPtSolidConeDR04();
                 pho_trkSumPtHollowConeDR04  = photon.trkSumPtHollowConeDR04();
                 pho_nTrkSolidConeDR04       = photon.nTrkSolidConeDR04();
                 pho_nTrkHollowConeDR04      = photon.nTrkHollowConeDR04();
                 pho_r9                      = photon.r9();
                 
                 pho_Et                    = photon.et();
                 pho_Eta                   = photon.eta();
                 pho_Phi                   = photon.phi();
                 pho_HOverE                = photon.hadronicOverEm();
                 pho_Pt                    = photon.pt();
                 pho_ScEt                  = superClusterEt;
                 pho_ScEta                 = photon.superCluster()->position().eta();
                 pho_ScPhi                 = photon.superCluster()->position().phi();
                 pho_ScEtaWidth            = photon.superCluster()->etaWidth();
                 pho_ScPhiWidth            = photon.superCluster()->phiWidth();
                 pho_ScRawEnergy           = photon.superCluster()->rawEnergy();
                 pho_ScEnergy              = photon.superCluster()->energy();
                 pho_rho                   = rho;
                 pho_E2x2                  = photon.showerShapeVariables().e2x2;
                 pho_E5x5                  = photon.e5x5();
                 pho_EffSigmaRR            = photon.showerShapeVariables().effSigmaRR;
                 pho_SigmaIetaIeta         = photon.sigmaIetaIeta();
                 pho_SigmaEtaEta           = photon.sigmaEtaEta();
                 pho_SigmaIphiIphi         = photon.showerShapeVariables().sigmaIphiIphi;
                 pho_SigmaIetaIphi         = photon.showerShapeVariables().sigmaIetaIphi;
                 pho_Full5x5_R9            = photon.full5x5_r9();
                 pho_R2x5                  = photon.r2x5();
                 pho_E2x2overE5x5          = photon.showerShapeVariables().e2x2/photon.e5x5();
                 pho_PhotonIso             = photon.photonIso();
                 pho_SumChargedParticlePt  = photon.sumChargedParticlePt();
                 pho_ChargedHadronIso      = photon.chargedHadronIso();
                 pho_ChargedHadronIsoWrongVtx      = photon.chargedHadronIsoWrongVtx();
                
                 treeEB->Fill();
                 



             }
             else if (photon.isEE())
             {
              
                 mon.fillHisto("photonScEta",           "__EE",   photon.superCluster()->position().eta(),           weight);
                 mon.fillHisto("photonEcalIso",         "__EE",      photon.ecalRecHitSumEtConeDR04(),       weight);
                 mon.fillHisto("photonHcalIso",         "__EE",     photon.hcalTowerSumEtConeDR04(),        weight);
                 mon.fillHisto("photonTrackSolidIso",   "__EE" ,   photon.trkSumPtSolidConeDR04(),         weight);
                 mon.fillHisto("photonTrackHollowIso",  "__EE" ,  photon.trkSumPtHollowConeDR04(),        weight);
                 mon.fillHisto("photonTrackCountSolid", "__EE",  photon.nTrkSolidConeDR04(),             weight);
                 mon.fillHisto("photonTrackCountHollow","__EE",  photon.nTrkHollowConeDR04() ,           weight);
                 mon.fillHisto("photonR9",              "__EE",  photon.r9() ,                             weight);
                 mon.fillHisto("photonEt",    "__EE",    photon.et(),    weight);
                 mon.fillHisto("photonEta",    "__EE",    photon.eta(),    weight);
                 mon.fillHisto("photonPhi",    "__EE",    photon.phi(),    weight);
                 mon.fillHisto("photonHoverE",    "__EE",    photon.hadronicOverEm(),    weight);
                 
                 mon.fillHisto("photonPt", "__EE", photon.pt(), weight);
                 
                 mon.fillHisto("photonScEt",    "__EE",    superClusterEt,    weight);
                 mon.fillHisto("photonScEta",    "__EE",    photon.superCluster()->position().eta(),    weight);
                 mon.fillHisto("photonScPhi",    "__EE",    photon.superCluster()->position().phi(),    weight);
                 mon.fillHisto("photonScEtaWidth",    "__EE",    photon.superCluster()->etaWidth(),    weight);
                 
                 mon.fillHisto("photonScPhiWidth",    "__EE",    photon.superCluster()->phiWidth(),    weight);
                 mon.fillHisto("photonScRawEnergy",    "__EE",    photon.superCluster()->rawEnergy(),    weight);
                 mon.fillHisto("photonScEnergy",    "__EE",    photon.superCluster()->energy(),    weight);
                 
                 mon.fillHisto("photonrho",  "__EE"  ,      rho      ,  weight);
                 mon.fillHisto("photone2x2",     "__EE"  ,  photon.showerShapeVariables().e2x2   ,  weight);
                 mon.fillHisto("photone5x5",     "__EE"  ,  photon.e5x5()   ,  weight);
                 mon.fillHisto("photoneffSigmaRR",     "__EE"  ,  photon.showerShapeVariables().effSigmaRR   ,  weight);
                 mon.fillHisto("photonsigmaIetaIeta",  "__EE"  ,  photon.sigmaIetaIeta()  ,  weight);
                 mon.fillHisto("photonsigmaEtaEta",  "__EE"  ,  photon.sigmaEtaEta()    ,  weight);
                 mon.fillHisto("photonsigmaIphiIphi",  "__EE"  ,  photon.showerShapeVariables().sigmaIphiIphi    ,  weight);
                 mon.fillHisto("photonsigmaIetaIphi",  "__EE"  ,  photon.showerShapeVariables().sigmaIetaIphi    ,  weight);
                 mon.fillHisto("photonfull5x5_r9",  "__EE"  ,  photon.full5x5_r9()          ,  weight);
                 
                 mon.fillHisto("photonr2x5",  "__EE"  ,  photon.r2x5()    ,  weight);
                 
                 mon.fillHisto("photonE2x2overE5x5",  "__EE"  ,  photon.showerShapeVariables().e2x2/photon.e5x5()  ,  weight);
                 
                 mon.fillHisto("photonIso",  "__EE"  , photon.photonIso(),weight);
                 mon.fillHisto("photonIsosumChargedParticlePt",  "__EE"  , photon.sumChargedParticlePt(),weight);
                 mon.fillHisto("photonchargedHadronIso",  "__EE"  , photon.chargedHadronIso(),weight);
                 mon.fillHisto("photonchargedHadronIsoWrongVtx",  "__EE"  , photon.chargedHadronIsoWrongVtx(),weight);
                 
                 pho_ecalRecHitSumEtConeDR04 = photon.ecalRecHitSumEtConeDR04();
                 pho_hcalTowerSumEtConeDR04  = photon.hcalTowerSumEtConeDR04();
                 pho_trkSumPtSolidConeDR04   = photon.trkSumPtSolidConeDR04();
                 pho_trkSumPtHollowConeDR04  = photon.trkSumPtHollowConeDR04();
                 pho_nTrkSolidConeDR04       = photon.nTrkSolidConeDR04();
                 pho_nTrkHollowConeDR04      = photon.nTrkHollowConeDR04();
                 pho_r9                      = photon.r9();
                 
                 pho_Et                    = photon.et();
                 pho_Eta                   = photon.eta();
                 pho_Phi                   = photon.phi();
                 pho_HOverE                = photon.hadronicOverEm();
                 pho_Pt                    = photon.pt();
                 pho_ScEt                  = superClusterEt;
                 pho_ScEta                 = photon.superCluster()->position().eta();
                 pho_ScPhi                 = photon.superCluster()->position().phi();
                 pho_ScEtaWidth            = photon.superCluster()->etaWidth();
                 pho_ScPhiWidth            = photon.superCluster()->phiWidth();
                 pho_ScRawEnergy           = photon.superCluster()->rawEnergy();
                 pho_ScEnergy              = photon.superCluster()->energy();
                 pho_rho                   = rho;
                 pho_E2x2                  = photon.showerShapeVariables().e2x2;
                 pho_E5x5                  = photon.e5x5();
                 pho_EffSigmaRR            = photon.showerShapeVariables().effSigmaRR;
                 pho_SigmaIetaIeta         = photon.sigmaIetaIeta();
                 pho_SigmaEtaEta           = photon.sigmaEtaEta();
                 pho_SigmaIphiIphi         = photon.showerShapeVariables().sigmaIphiIphi;
                 pho_SigmaIetaIphi         = photon.showerShapeVariables().sigmaIetaIphi;
                 pho_Full5x5_R9            = photon.full5x5_r9();
                 pho_R2x5                  = photon.r2x5();
                 pho_E2x2overE5x5          = photon.showerShapeVariables().e2x2/photon.e5x5();
                 pho_PhotonIso             = photon.photonIso();
                 pho_SumChargedParticlePt  = photon.sumChargedParticlePt();
                 pho_ChargedHadronIso      = photon.chargedHadronIso();
                 pho_ChargedHadronIsoWrongVtx      = photon.chargedHadronIsoWrongVtx();
                 
                 treeEE->Fill();
             }
              
                 
        */  
                 
                 
                 
             
            // bool inAnyGap = pho->isEBEEGap() || (pho->isEB()&&pho->isEBEtaGap()) ||(pho->isEB()&&pho->isEBPhiGap())  || (pho->isEE()&&pho->isEERingGap()) || (pho->isEE()&&pho->isEEDeeGap()) ;
            // bool inAnyGap = pho->isEBEEGap() || (pho->isEB()&&pho->isEBEtaGap()) ||(pho->isEB()&&pho->isEBPhiGap())  || (pho->isEE()&&pho->isEERingGap()) || (pho->isEE()&&pho->isEEDeeGap()) ;
             
             
             
             
             
             
            if(photonTrigger && (photon.pt()<(triggerThreshold) || photon.pt()>(triggerThresholdHigh+10)))continue;

	    //Calibrate photon energy
                const EcalRecHitCollection* recHits = (photon.isEB()) ? recHitCollectionEBHandle.product() : recHitCollectionEEHandle.product();
                unsigned int gainSeed = patUtils::GainSeed(photon,recHits);
                 if(!isMC){

                 double scale_corr=phScaler.ScaleCorrection(ev.eventAuxiliary().run(),photon.isEB(),photon.r9(), photon.superCluster()->eta(), photon.et(),gainSeed);
                  //At this point, the new data energy will be:
                 // E_new=E_old*(scale_corr);
                  TLorentzVector p4(photon.px(),photon.py(),photon.pz(),photon.energy());
                  photon.setP4(LorentzVector(p4.Px()*scale_corr,p4.Py()*scale_corr,p4.Pz()*scale_corr,p4.E()*scale_corr ) ); 
                 }

                 if(isMC){

                 double sigma=phScaler.getSmearingSigma(ev.eventAuxiliary().run(),photon.isEB(),photon.r9(), photon.superCluster()->eta(), photon.et(),gainSeed,0,0);
                //Put the last two inputs at 0,0 for the nominal value of sigma
                //Now smear the MC energy
                  TRandom3 *rgen_ = new TRandom3(0);
                  double smearValue = rgen_->Gaus(1, sigma) ;
                  TLorentzVector p4(photon.px(),photon.py(),photon.pz(),photon.energy());
                  photon.setP4(LorentzVector(p4.Px()*smearValue,p4.Py()*smearValue,p4.Pz()*smearValue,p4.E()*smearValue ) ); 
                 }
//	    PhotonEnCorrector.calibrate(photon, ev.eventAuxiliary().run(), edm::StreamID::invalidStreamID());

	    //Removed all the phtons which are alsp reconstructed ad Electron and muons
            double minDRlg(9999.);
            for(size_t ilep=0; ilep<selLeptons.size(); ilep++){
              minDRlg=TMath::Min(minDRlg,deltaR( photon.p4(), selLeptons[ilep].p4()));
            }
	    if(minDRlg<0.1) continue;

            if(photon.pt()<55)continue;
            if(fabs(photon.superCluster()->eta())>1.4442 ) continue;
            if(!patUtils::passId(photon, rho, patUtils::llvvPhotonId::Tight)) continue;

            selPhotons.push_back(photon);
            if(photon.pt()>55)nPho55++;
            if(photon.pt()>100)nPho100++;

           }


    //    LorentzVector Higgsgammagamma(0,0,0,0);
    //     for(unsigned int iigen=0; iigen<matchingphoton.size(); iigen++){
    //         Higgsgammagamma += matchingphoton[iigen].p4();
    //     }
    //     HIGGS_MASS_gammagamma = Higgsgammagamma.mass();
    //     tree1->Fill();
         
         
         
           std::sort(selLeptons.begin(),   selLeptons.end(), utils::sort_CandidatesByPt);
           std::sort(extraLeptons.begin(), extraLeptons.end(), utils::sort_CandidatesByPt);


	    if (isMC_Wlnu_inclusive || isMC_Wlnu_HT100){ //Avoid double counting and make our W#rightarrow l#nu exclusif of the dataset with a cut on HT...
  	    bool isHT100 = false;

    	  //Let's create our own HT variable
      	double vHT =0;
      	for(size_t ig=0; ig<genJets.size(); ig++){
      	  //cross-clean with selected leptons and photons
      	  double minDRlj(9999.); for(size_t ilep=0; ilep<selLeptons.size(); ilep++)  minDRlj = TMath::Min( minDRlj, deltaR(genJets[ig],selLeptons[ilep]) );
      	  double minDRlg(9999.); for(size_t ipho=0; ipho<selPhotons.size(); ipho++)  minDRlg = TMath::Min( minDRlg, deltaR(genJets[ig],selPhotons[ipho]) );
      	  if(minDRlj<0.4 || minDRlg<0.4) continue;

	        vHT += genJets[ig].pt();
	      }
	      if(vHT >100) isHT100 = true;

	      if(isMC_Wlnu_inclusive && isHT100) continue; //reject event
	      if(isMC_Wlnu_HT100 && !isHT100) continue; //reject event

	    }


/*

           //update the met for lepton energy scales
           met.setP4(met.p4() - muDiff - elDiff); //note this also propagates to all MET uncertainties
           met.setUncShift(met.px() - muDiff.px()*0.002, met.py() - muDiff.py()*0.002, met.sumEt() - muDiff.pt()*0.002, pat::MET::METUncertainty::MuonEnUp);   //assume 1% uncertainty on muon rochester
           met.setUncShift(met.px() + muDiff.px()*0.002, met.py() + muDiff.py()*0.002, met.sumEt() + muDiff.pt()*0.002, pat::MET::METUncertainty::MuonEnDown); //assume 1% uncertainty on muon rochester
           met.setUncShift(met.px() - elDiff_forMET.px(), met.py() - elDiff_forMET.py(), met.sumEt() - elDiff_forMET.pt(), pat::MET::METUncertainty::ElectronEnUp);   //assume 1% uncertainty on electron scale correction
           met.setUncShift(met.px() + elDiff_forMET.px(), met.py() + elDiff_forMET.py(), met.sumEt() + elDiff_forMET.pt(), pat::MET::METUncertainty::ElectronEnDown); //assume 1% uncertainty on electron scale correction
*/
         //
         //JET/MET ANALYSIS
         //
         //add scale/resolution uncertainties and propagate to the MET
         if(isMC || is2016data) utils::cmssw::updateJEC(jets,jesCor,totalJESUnc,rho,vtx.size(),isMC);

         //select the jets
         std::map<string, pat::JetCollection> selJetsVar;
         std::map<string, int   > njetsVar;
         std::map<string, int   > nbtagsVar;
         std::map<string, double> mindphijmetVar;
         for(unsigned int ivar=0;ivar<jetVarNames.size();ivar++){njetsVar[jetVarNames[ivar]] = 0;}  //initialize
         for(unsigned int ivar=0;ivar<jetVarNames.size();ivar++){mindphijmetVar[jetVarNames[ivar]] = 9999.0;}  //initialize
         nbtagsVar[""] = 0; nbtagsVar["_eff_bup"] = 0; nbtagsVar["_eff_bdown"] = 0;  //initialize
/*
         for(size_t ijet=0; ijet<jets.size(); ijet++){
             pat::Jet jet = jets[ijet]; //copy the jet, such that we can update it

             if(jet.pt()<15 || fabs(jet.eta())>4.7 ) continue;

             //mc truth for this jet
             const reco::GenJet* genJet=jet.genJet();
             TString jetType( genJet && genJet->pt()>0 ? "truejetsid" : "pujetsid" );

             //cross-clean with selected leptons and photons
             double minDRlj(9999.); for(size_t ilep=0; ilep<selLeptons.size(); ilep++)  minDRlj = TMath::Min( minDRlj, deltaR(jet,selLeptons[ilep]) );
             double minDRlg(9999.); for(size_t ipho=0; ipho<selPhotons.size(); ipho++)  minDRlg = TMath::Min( minDRlg, deltaR(jet,selPhotons[ipho]) );
             if(minDRlj<0.4 || minDRlg<0.4) continue;

             //jet id
             bool passPFloose = patUtils::passPFJetID("Loose", jet);
             bool passLooseSimplePuId = true; //patUtils::passPUJetID(jet); //FIXME Broken in miniAOD V2 : waiting for JetMET fix. (Hugo)
             if(jet.pt()>30){
                 mon.fillHisto(jetType,"",fabs(jet.eta()),0);
                 if(passPFloose)                        mon.fillHisto("jetId", jetType,fabs(jet.eta()),1);
                 if(passLooseSimplePuId)                mon.fillHisto("jetId", jetType,fabs(jet.eta()),2);
                 if(passPFloose && passLooseSimplePuId) mon.fillHisto("jetId", jetType,fabs(jet.eta()),3);
             }
             if(!passPFloose || !passLooseSimplePuId) continue;


            //check for btagging
            if(jet.pt()>30 && fabs(jet.eta())<2.5){
              bool hasCSVtag = (jet.bDiscriminator("pfCombinedInclusiveSecondaryVertexV2BJetTags")>btagLoose);
              bool hasCSVtagUp = hasCSVtag;
              bool hasCSVtagDown = hasCSVtag;
              //update according to the SF measured by BTV
              if(isMC){
                  int flavId=jet.partonFlavour();  double eta=jet.eta();
		  btsfutil.SetSeed(ev.eventAuxiliary().event()*10 + ijet*10000);
                  if      (abs(flavId)==5){
					//  74X recommendation
					//     btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal   .eval(BTagEntry::FLAV_B   , eta, jet.pt()), beff);
                                        //     btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCalUp .eval(BTagEntry::FLAV_B   , eta, jet.pt()), beff);
                                        //     btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCalDn .eval(BTagEntry::FLAV_B   , eta, jet.pt()), beff);
                                        //  80X recommendation
                                        btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal80X.eval_auto_bounds("central", BTagEntry::FLAV_B   , eta, jet.pt()), beff);
                                        btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCal80X.eval_auto_bounds("up", BTagEntry::FLAV_B   , eta, jet.pt()), beff);
                                        btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCal80X.eval_auto_bounds("down", BTagEntry::FLAV_B   , eta, jet.pt()), beff);
                  }else if(abs(flavId)==4){
					//  74X recommendation
					//     btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal   .eval(BTagEntry::FLAV_C   , eta, jet.pt()), beff);
					//     btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCalUp .eval(BTagEntry::FLAV_C   , eta, jet.pt()), beff);
					//     btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCalDn .eval(BTagEntry::FLAV_C   , eta, jet.pt()), beff);
                                        //  80X recommendation
			                btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal80X.eval_auto_bounds("central", BTagEntry::FLAV_C   , eta, jet.pt()), beff);
                                        btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCal80X.eval_auto_bounds("up", BTagEntry::FLAV_C   , eta, jet.pt()), beff);
                                        btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCal80X.eval_auto_bounds("down", BTagEntry::FLAV_C   , eta, jet.pt()), beff);
                  }else{
					//  74X recommendation
					//     btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCalL  .eval(BTagEntry::FLAV_UDSG, eta, jet.pt()), leff);
                                        //     btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCalLUp.eval(BTagEntry::FLAV_UDSG, eta, jet.pt()), leff);
                                        //     btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCalLDn.eval(BTagEntry::FLAV_UDSG, eta, jet.pt()), leff);
                                        //  80X recommendation
			                btsfutil.modifyBTagsWithSF(hasCSVtag    , btagCal80X.eval_auto_bounds("central", BTagEntry::FLAV_UDSG   , eta, jet.pt()), leff);
                                        btsfutil.modifyBTagsWithSF(hasCSVtagUp  , btagCal80X.eval_auto_bounds("up", BTagEntry::FLAV_UDSG   , eta, jet.pt()), leff);
                                        btsfutil.modifyBTagsWithSF(hasCSVtagDown, btagCal80X.eval_auto_bounds("down", BTagEntry::FLAV_UDSG   , eta, jet.pt()), leff);
                  }
              }

              if(hasCSVtag    )nbtagsVar[""          ]++;
              if(hasCSVtagUp  )nbtagsVar["_eff_bup"  ]++;
              if(hasCSVtagDown)nbtagsVar["_eff_bdown"]++;
            }


            for(unsigned int ivar=0;ivar<jetVarNames.size();ivar++){
	       if(!isMC && ivar>0) continue;
               pat::Jet varJet = jet;
               if(ivar!=0) varJet.setP4(jet.p4() * jet.userFloat(jetVarNames[ivar]));
               selJetsVar[jetVarNames[ivar]].push_back(varJet);

               if(varJet.pt()>30){
                  njetsVar[jetVarNames[ivar]]++;

                  float dphijmet=fabs(deltaPhi(met.corP4(metcor).phi(), varJet.phi()));
                  if(dphijmet<mindphijmetVar[jetVarNames[ivar]]) mindphijmetVar[jetVarNames[ivar]]=dphijmet;
               }
            }
         }    */
         //sort all jet collection by pT
         for(auto jetCollIt = selJetsVar.begin(); jetCollIt!=selJetsVar.end(); jetCollIt++){
            std::sort(jetCollIt->second.begin(), jetCollIt->second.end(), utils::sort_CandidatesByPt);
         }


         //save weight
         double initialWeight = weight;

         //compute scale uncertainty once and for all
         std::pair<double, double> scaleUncVar = patUtils::scaleVariation(ev);  //compute it only once

         // LOOP ON SYSTEMATIC VARIATION FOR THE STATISTICAL ANALYSIS
/*         for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
          if(!isMC && ivar>0 ) continue; //loop on variation only for MC samples

          //start from a nominal
          float weight = initialWeight;

           //Theoretical Uncertanties: PDF, Alpha and Scale
           if(varNames[ivar]=="_th_alphas")     weight *= patUtils::alphaVariation(ev);
           if(varNames[ivar]=="_th_pdf")        weight *= patUtils::pdfVariation(ev);

           //EwkCorrections variation
           if ( varNames[ivar]=="_th_ewkup")    weight *= ewkCorrections_up;
           if ( varNames[ivar]=="_th_ewkdown")  weight *= ewkCorrections_down;

           //pileup variations
           if(varNames[ivar]=="_puup")          weight *= puWeightUp;
           if(varNames[ivar]=="_pudown")        weight *= puWeightDown;

           //recompute MET with variation
           LorentzVector imet = met.corP4(metcor);
           if(varNames[ivar]=="_scale_jup")      imet = met.shiftedP4(pat::MET::METUncertainty::JetEnUp           , metcor);
           if(varNames[ivar]=="_scale_jdown")    imet = met.shiftedP4(pat::MET::METUncertainty::JetEnDown         , metcor);
           if(varNames[ivar]=="_res_jup")        imet = met.shiftedP4(pat::MET::METUncertainty::JetResUp          , metcor);
           if(varNames[ivar]=="_res_jdown")      imet = met.shiftedP4(pat::MET::METUncertainty::JetResDown        , metcor);
           if(varNames[ivar]=="_scale_umetup")   imet = met.shiftedP4(pat::MET::METUncertainty::UnclusteredEnUp   , metcor);
           if(varNames[ivar]=="_scale_umetdown") imet = met.shiftedP4(pat::MET::METUncertainty::UnclusteredEnDown , metcor);
           if(varNames[ivar]=="_scale_mup")      imet = met.shiftedP4(pat::MET::METUncertainty::MuonEnUp          , metcor);
           if(varNames[ivar]=="_scale_mdown")    imet = met.shiftedP4(pat::MET::METUncertainty::MuonEnDown        , metcor);
           if(varNames[ivar]=="_stat_eup")      imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnUp      , metcor);
           if(varNames[ivar]=="_stat_edown")    imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnDown    , metcor);
           if(varNames[ivar]=="_sys_eup")      imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnUp      , metcor);
           if(varNames[ivar]=="_sys_edown")    imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnDown    , metcor);
           if(varNames[ivar]=="_GS_eup")      imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnUp      , metcor);
           if(varNames[ivar]=="_GS_edown")    imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnDown    , metcor);
           if(varNames[ivar]=="_resRho_eup")      imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnUp      , metcor);
           if(varNames[ivar]=="_resRho_edown")    imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnDown    , metcor);
           if(varNames[ivar]=="_resPhi_edown")    imet = met.shiftedP4(pat::MET::METUncertainty::ElectronEnDown    , metcor);

           if(selLeptonsVar    .find(varNames[ivar].Data())!=selLeptonsVar    .end())selLeptons     = selLeptonsVar    [varNames[ivar].Data()];
           if(extraLeptonsVar    .find(varNames[ivar].Data())!=extraLeptonsVar    .end())extraLeptons     = extraLeptonsVar    [varNames[ivar].Data()];
           auto selJets      = selJetsVar[""];        if(selJetsVar    .find(varNames[ivar].Data())!=selJetsVar    .end())selJets     = selJetsVar    [varNames[ivar].Data()];
           auto njets        = njetsVar [""];         if(njetsVar      .find(varNames[ivar].Data())!=njetsVar      .end())njets       = njetsVar      [varNames[ivar].Data()];
           auto nbtags       = nbtagsVar[""];         if(nbtagsVar     .find(varNames[ivar].Data())!=nbtagsVar     .end())nbtags      = nbtagsVar     [varNames[ivar].Data()];
           auto mindphijmet  = mindphijmetVar[""];    if(mindphijmetVar.find(varNames[ivar].Data())!=mindphijmetVar.end())mindphijmet = mindphijmetVar[varNames[ivar].Data()];

            //
            // ASSIGN CHANNEL
            //
            double weightBefLoop = weight;
            for(unsigned int L=0;L<3;L++){  //Loop to assign a Z-->ll channel to photons
               if(L>0 && !(photonTrigger && gammaWgtHandler) )continue; //run it only for photon reweighting
               weight = weightBefLoop;
               std::vector<TString> chTags;
               TString evCat;
               int dilId(1);
               LorentzVector boson(0,0,0,0);
               if(selLeptons.size()==2  && !gammaWgtHandler){  //this is not run if photon reweighting is activated to avoid mixing
                   for(size_t ilep=0; ilep<2; ilep++){
                       dilId *= selLeptons[ilep].pdgId();
                       int id(abs(selLeptons[ilep].pdgId()));
		       if(is2016MC) {
                           if(id==11)weight *= lepEff.getRecoEfficiency( selLeptons[ilep].el.superCluster()->eta(), id).first; //Reconstruction eff
                           else if(id==13)weight *= lepEff.getTrackingEfficiency( selLeptons[ilep].eta(), id).first; //Tracking eff
                           weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[ilep].pt(), selLeptons[ilep].eta(), id,  id ==11 ? "tight"    : "tkHighPT"   ,patUtils::CutVersion::ICHEP16Cut ).first : 1.0; //ID
			   if(id==13){
                           weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[ilep].pt(), selLeptons[ilep].eta(), id, "HZZ2l2nuiso",patUtils::CutVersion::ICHEP16Cut ).first : 1.0;} //ISO w.r.t ID
		       } else if(isMC && !is2016MC){
                           weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[ilep].pt(), selLeptons[ilep].eta(), id,  id ==11 ? "tight"    : "tight"   ,patUtils::CutVersion::Spring15Cut25ns).first : 1.0; //ID
                           weight *= isMC ? lepEff.getLeptonEfficiency( selLeptons[ilep].pt(), selLeptons[ilep].eta(), id,  id ==11 ? "tightiso" : "tightiso",patUtils::CutVersion::Spring15Cut25ns).first : 1.0; //ISO w.r.t ID

		       }

                       boson += selLeptons[ilep].p4();
                   }
                   //check the channel
                   if( abs(dilId)==121){  chTags.push_back("ee");   chTags.push_back("ll"); }
                   if( abs(dilId)==169){  chTags.push_back("mumu"); chTags.push_back("ll"); }
                   if( abs(dilId)==143){  chTags.push_back("emu");  }
		   if (isOldMC) {
		     if(isMC && abs(dilId)==169)weight *= lepEff.getTriggerEfficiencySF(selLeptons[0].pt(), selLeptons[0].eta(), selLeptons[1].pt(), selLeptons[1].eta(), dilId,is2016MC).first;
		     if(isMC && abs(dilId)==121)weight *= lepEff.getTriggerEfficiencySF(selLeptons[0].pt(), selLeptons[0].el.superCluster()->eta(), selLeptons[1].pt(), selLeptons[1].el.superCluster()->eta(), dilId,is2016MC).first;  //commented for ee as inefficiencies should be covered by the singleMu/El triggers
                   }
		   evCat=eventCategoryInst.GetCategory(selJets,boson);
               }else if(selPhotons.size()==1 && photonTrigger){
                   dilId=22;
                   if(L==0)                         {chTags.push_back("gamma");
                   }else if(L==1 && gammaWgtHandler){chTags.push_back("ee");   chTags.push_back("ll");
                   }else if(L==2 && gammaWgtHandler){chTags.push_back("mumu"); chTags.push_back("ll");
                   }else{ continue;
                   }
                   boson = selPhotons[0].p4();
                   evCat=eventCategoryInst.GetCategory(selJets,boson);
                   if(L>0 && gammaWgtHandler)boson = gammaWgtHandler->getMassiveP4(boson, string(L==1?"ee":"mumu")+evCat);
                   std::vector<Float_t> photonVars;
                   photonVars.push_back(boson.pt());
                   float photonWeightMain=1.0;
		   float photonPuWeight=1.0;
                   if(L>0 && gammaWgtHandler) {
		     photonWeightMain=gammaWgtHandler->getWeightFor(photonVars,string(L==1?"ee":"mumu")+evCat);

		     if (doPUCorrections) {
		       TString puWeightsFileUrl(puWeightsFilePath[0].c_str());
		       gSystem->ExpandPathName(puWeightsFileUrl);
		       puWeightsFile=TFile::Open(puWeightsFileUrl);

		       if (puWeightsFile) {
			 TH2D* h_pu_zpt_weight = (TH2D*)puWeightsFile->Get(L==1?"h_rho_zpt_weight_e":"h_rho_zpt_weight_m");
			 photonPuWeight=h_pu_zpt_weight->GetBinContent(h_pu_zpt_weight->FindBin(boson.pt(), vtx.size()));
			 puWeightsFile->Close();
		       }
		     }
		   }
                   weight *= triggerPrescale * photonWeightMain * photonPuWeight;
		   if(is2016MC) weight *= phoEff.getPhotonEfficiency(selPhotons[0].pt(), selPhotons[0].superCluster()->eta(), "tight",patUtils::CutVersion::ICHEP16Cut ).first;
               }else{
                  continue;
               }

               std::vector<TString> tags(1,"all");
               for(size_t ich=0; ich<chTags.size(); ich++){
                 tags.push_back( chTags[ich] );
                 tags.push_back( chTags[ich]+evCat );
               }

               //////////////////////////
               //                      //
               //  BASELINE SELECTION  //
               //                      //
               //////////////////////////

 
 
 
               bool passMass(fabs(boson.mass()-91)<15);
               bool passQt(boson.pt()>55);
               bool passThirdLeptonVeto( selLeptons.size()==2 && extraLeptons.size()==0 );
               bool passBtags(nbtags==0);
               bool passMinDphijmet( njets==0 || mindphijmet>0.5);

	       double b_dphi=fabs(deltaPhi(boson.phi(),imet.phi())); 
	       bool passDphi(b_dphi>0.5);

               if(dilId==22){
                   passMass=photonTrigger;
                   passThirdLeptonVeto=(selLeptons.size()==0 && extraLeptons.size()==0);
               }

              if(varNames[ivar]=="_lepveto" && !passThirdLeptonVeto){
                 int NExtraLep = std::max(0, int(selLeptons.size()) + int(extraLeptons.size()) - 2);
                 if(((rand()%1000)/1000.0) < pow(0.04, NExtraLep))passThirdLeptonVeto=true;  //4% Id uncertainty exponent Number of aditional leptons
              }

              double mt=0;
              if(passQt && passThirdLeptonVeto && passMinDphijmet && (boson.mass()>40 && boson.mass()<200)) mt=higgs::utils::transverseMass(boson,imet,true);


               if(ivar==0){  //fill control plots only for the nominal systematic

                  mon.fillHisto("nleptons",tags,selLeptons.size(), weight);
                  mon.fillHisto("npho", tags, selPhotons.size(), weight);
                  mon.fillHisto("npho55", tags, nPho55, weight);
                  mon.fillHisto("npho100", tags, nPho100, weight);
                  if(photonTrigger && selPhotons.size()>0)mon.fillHisto("photonpt", tags[tags.size()-1]+photonTriggerTreshName,   selPhotons[0].pt(), weight);


                  // Photon trigger efficiencies
                  // Must be run without the photonTrigger requirement on top of of the Analysis.
                  if (photonTriggerStudy && selPhotons.size() ){
                    TString tag="trigger";
                    pat::Photon iphoton = selPhotons[0];

                    mon.fillHisto("phopt", tag, iphoton.pt(),weight);
                    mon.fillHisto("phoeta", tag, iphoton.eta(), weight);
                    trigUtils::photonControlSample(ev, iphoton, mon, tag);
                    trigUtils::photonControlEff(ev, iphoton, mon, tag);

                    for(size_t itag=0; itag<tags.size(); itag++){
                      //update the weight
                      TString icat=tags[itag];
                      mon.fillHisto("phopt", icat, iphoton.pt(),weight);
                      mon.fillHisto("phoeta", icat, iphoton.eta(),weight);
                      trigUtils::photonControlSample(ev, iphoton, mon, icat);
                      trigUtils::photonControlEff(ev, iphoton, mon, icat);
                    }
                  } // end Trigger efficiencies


		  mon.fillHisto("eventflow",  tags,0,weight);

                  mon.fillHisto("nvtxraw",  tags,vtx.size(),weight/puWeight);
                  mon.fillHisto("nvtx",  tags,vtx.size(),weight);
                  mon.fillHisto("rho",  tags,rho,weight);

		  mon.fillHisto("zpt_vs_nvtx",tags,boson.pt(),vtx.size(),weight);
		  mon.fillHisto("zpt_vs_rho",tags,boson.pt(),rho,weight);

                  if(chTags.size()==0) continue;
                  mon.fillHisto("eventflow",  tags,1,weight);
                  if(dilId!=22){
                    mon.fillHisto("leadpt",      tags,selLeptons[0].pt(),weight);
                    mon.fillHisto("trailerpt",   tags,selLeptons[1].pt(),weight);
                    mon.fillHisto("leadeta",     tags,fabs(selLeptons[0].eta()),weight);
                    mon.fillHisto("trailereta",  tags,fabs(selLeptons[1].eta()),weight);
                  }

                  mon.fillHisto("zmass", tags,boson.mass(),weight);
                  mon.fillHisto("zy",    tags,fabs(boson.Rapidity()),weight);

                  if(passMass){
                    mon.fillHisto("eventflow",tags, 2,weight);
                    mon.fillHisto("zpt",      tags, boson.pt(),weight);
                    mon.fillHisto("zpt_rebin",tags, boson.pt(),weight,true);
                    if(imet.pt()>125)mon.fillHisto("zptMet125",      tags, boson.pt(),weight);


                    //these two are used to reweight photon -> Z, the 3rd is a control
                    mon.fillHisto("qt",       tags, boson.pt(),weight,true);
                    mon.fillHisto("qtraw",    tags, boson.pt(),weight/triggerPrescale,true);

                    if(passQt){
                      mon.fillHisto("eventflow",tags,3,weight);
                      int nExtraLeptons((selLeptons.size()-2)+extraLeptons.size());
                      mon.fillHisto("nextraleptons",tags,nExtraLeptons,weight);
                      if(nExtraLeptons>0){
                        LorentzVector thirdLepton(selLeptons.size()>2 ?  selLeptons[1].p4() : extraLeptons[0].p4());
                        double dphi=fabs(deltaPhi(thirdLepton.phi(),imet.phi()));
                        double mt3rd=TMath::Sqrt(2*thirdLepton.pt()*imet.pt()*(1-TMath::Cos(dphi)));
                        mon.fillHisto("thirdleptonpt",tags,thirdLepton.pt(),weight);
                        mon.fillHisto("thirdleptoneta",tags,fabs(thirdLepton.eta()),weight);
                        mon.fillHisto("thirdleptonmt",tags,mt3rd,weight);
                      }
                      if(passThirdLeptonVeto){

                        mon.fillHisto("eventflow",tags,4,weight);

			if (selJets.size()>0) {
			  mon.fillHisto( "leadjet_pt",tags,selJets[0].pt(),weight);
			  mon.fillHisto( "leadjet_eta",tags,fabs(selJets[0].eta()),weight);
			  mon.fillHisto( "leadjet_phi",tags,selJets[0].phi(),weight);
			}

                        for(size_t ijet=0; ijet<selJets.size(); ijet++){
                          if(selJets[ijet].pt()<30 || fabs(selJets[ijet].eta())>2.5) continue;

                          float csv(selJets[ijet].bDiscriminator("pfCombinedInclusiveSecondaryVertexV2BJetTags"));
                          mon.fillHisto( "csv",tags,csv,weight);
                          if(!isMC) continue;
                          int flavId=selJets[ijet].partonFlavour();
                          TString jetFlav("others");
                          if(abs(flavId)==5)      jetFlav="b";
                          else if(abs(flavId)==4) jetFlav="c";
                          mon.fillHisto( "csv"+jetFlav,tags,csv,weight);
                        }
                        mon.fillHisto( "nbtags",tags,nbtags,weight);

                        if(passBtags){
                          mon.fillHisto("eventflow",tags,5,weight);

                          mon.fillHisto( "mindphijmet",tags,mindphijmet,weight);
                          if(imet.pt()>25)mon.fillHisto( "mindphijmet25",tags,mindphijmet,weight);
                          if(imet.pt()>50)mon.fillHisto( "mindphijmet50",tags,mindphijmet,weight);
                          if(imet.pt()>80)mon.fillHisto( "mindphijmetNM1",tags,mindphijmet,weight);
                          if(passMinDphijmet){
                            mon.fillHisto("eventflow",tags,6,weight);

			    mon.fillHisto("met_vs_dphi",tags,imet.pt(),b_dphi,weight);
			    mon.fillHisto("mt_vs_dphi",tags,mt,b_dphi,weight);    

			    mon.fillHisto( "dphi_boson_met",tags,b_dphi,weight);          
			    if (imet.pt()>125) mon.fillHisto( "dphi_boson_met125",tags,b_dphi,weight);   

			    if (passDphi) {
			      mon.fillHisto("eventflow",tags,7,weight);

			      //this one is used to sample the boson mass: cuts may shape Z lineshape
			      mon.fillHisto("qmass",       tags, boson.mass(),weight);
			      mon.fillHisto( "njets",tags,njets,weight);

			      //   double b_dphi=fabs(deltaPhi(boson.phi(),imet.phi()));
			      mon.fillHisto( "metphi",tags,imet.phi(),weight);
			      mon.fillHisto( "metphiUnCor",tags,met.corP4(pat::MET::METCorrectionLevel::Type1).phi(),weight);
			      mon.fillHisto( "bosonnvtx",tags,vtx.size(),weight);
			      mon.fillHisto( "bosonrho",tags,rho,weight);
			      
			      mon.fillHisto( "bosoneta",tags,boson.eta(),weight);
			      mon.fillHisto( "bosonphi",tags,boson.phi(),weight);
			      mon.fillHisto( "bosonphiHG",tags,boson.phi(),weight);
			      
			      mon.fillHisto( "met",tags,imet.pt(),weight,true);
			      mon.fillHisto( "metpuppi",tags,puppimet.pt(),weight,true);
			      mon.fillHisto( "balance",tags,imet.pt()/boson.pt(),weight);
			      
			      TVector2 met2(imet.px(),imet.py());
			      TVector2 boson2(boson.px(), boson.py());
			      double axialMet(boson2*met2); axialMet/=-boson.pt();
			      mon.fillHisto( "axialmet",tags,axialMet,weight);
			      
			      mon.fillHisto( "mt",tags,mt,weight,true);
			      
			      if(imet.pt()>optim_Cuts1_met[0]) {
				mon.fillHisto( "mtcheckpoint",  tags, mt,       weight, true);
				mon.fillHisto( "metcheckpoint", tags, imet.pt(), weight, true);
			      }
			      
			      if(imet.pt()>80){
				mon.fillHisto("eventflow",tags,8,weight);
				mon.fillHisto( "mtNM1",tags,mt,weight,true);
				mon.fillHisto( "balanceNM1",tags,imet.pt()/boson.pt(),weight);
				mon.fillHisto( "axialmetNM1",tags,axialMet,weight);
			      }
			      
			      if(imet.pt()>125){
				mon.fillHisto("eventflow",tags,9,weight);
				
				mon.fillHisto( "metfinal",tags,imet.pt(),weight,true);
				mon.fillHisto( "mtfinal",tags,mt,weight,true);
				mon.fillHisto( "mindphijmetfinal",tags,mindphijmet,weight);
				mon.fillHisto( "njetsfinal",tags,njets,weight);

				if(!isMC){
				  char buffer[1024];
				  sprintf(buffer, "\ncat=%s %9i:%6i:%9lli @ %50s\n",  tags[tags.size()-1].Data(), ev.eventAuxiliary().run(), ev.eventAuxiliary().luminosityBlock(), ev.eventAuxiliary().event(), urls[f].c_str() );  debugText+=buffer;
				  sprintf(buffer, " - nLep=%2i nSoftLept=%2i nPhotons=%2i  nJets=%2i\n", int(selLeptons.size()), int(extraLeptons.size()), int(selPhotons.size()), int(selJets.size())  ); debugText+=buffer;
				  sprintf(buffer, " - MET=%8.2f mT=%8.2f nvtx=%3i\n", imet.pt(), mt, int(vtx.size()) ); debugText+=buffer;
				  sprintf(buffer, " - MET type1XY=%8.2f type1=%8.2f uncorrected=%8.2f\n", met.corP4(pat::MET::METCorrectionLevel::Type1XY).pt(), met.corP4(pat::MET::METCorrectionLevel::Type1).pt(), met.corP4(pat::MET::METCorrectionLevel::Raw).pt() ); debugText+=buffer;
				  sprintf(buffer, " - LeptonScale changes on MET mu=%8.2f  el=%8.2f\n", muDiff.pt(), elDiff.pt() ); debugText+=buffer;
				  sprintf(buffer, " - Z pT=%6.2f eta=%+6.2f phi=%+6.2f\n", boson.pt(), boson.eta(), boson.phi() ); debugText+=buffer;
				  if(selLeptons.size()>0)sprintf(buffer, " - lep0 Id=%+3i pT=%6.2f, eta=%+6.2f phi=%+6.2f\n", selLeptons[0].pdgId(), selLeptons[0].pt(), selLeptons[0].eta(), selLeptons[0].phi()  ); debugText+=buffer;
				  if(selLeptons.size()>1)sprintf(buffer, " - lep1 Id=%+3i pT=%6.2f, eta=%+6.2f phi=%+6.2f\n", selLeptons[1].pdgId(), selLeptons[1].pt(), selLeptons[1].eta(), selLeptons[1].phi()  ); debugText+=buffer;
				}
			      }

			      if(mt>500){
				mon.fillHisto( "metNM1",tags,imet.pt(),weight,true);
			      }
			      
			      //pre-VBF control
			      if(njets>=2){
				LorentzVector dijet=selJets[0].p4()+selJets[1].p4();
				float deta=fabs(selJets[0].eta()-selJets[1].eta());
				float dphi=fabs(deltaPhi(selJets[0].phi(),selJets[1].phi()));
				float pt1(selJets[0].pt()),pt2(selJets[1].pt());
				mon.fillHisto( "leadjetpt",tags,pt1,weight);
				mon.fillHisto( "trailerjetpt",tags,pt2,weight);
				if(pt1>30 && pt2>30){
				  float eta1(selJets[0].eta()),eta2(selJets[1].eta());
				  float fwdEta( fabs(eta1)>fabs(eta2) ? eta1 : eta2);
				  float cenEta( fabs(eta1)>fabs(eta2) ? eta2 : eta1);
				  mon.fillHisto("vbfjeteta", tags,fabs(fwdEta),  weight);
				  mon.fillHisto("vbfjeteta", tags,fabs(cenEta),  weight);
				  mon.fillHisto("fwdjeteta",tags,fabs(fwdEta),  weight);
				  mon.fillHisto("cenjeteta",tags,fabs(cenEta),  weight);
				  mon.fillHisto("vbfdetajj",tags,deta,        weight);
				  if(deta>4.0){
				    mon.fillHisto("vbfmjj",   tags,dijet.mass(),weight,true);
				    if(dijet.mass()>500){
				      mon.fillHisto("vbfdphijj",tags,dphi,        weight);
				      int countJetVeto = 0;
				      for(size_t ijet=2; ijet<selJets.size(); ijet++){
					if((selJets[ijet].eta()<selJets[0].eta() && selJets[ijet].eta()>selJets[1].eta()) ||
					   (selJets[ijet].eta()>selJets[0].eta() && selJets[ijet].eta()<selJets[1].eta())){
                                          countJetVeto++;
					}
				      }
				      mon.fillHisto("vbfcjv",tags,countJetVeto,        weight);
				    }
				  }
				  
				  if(imet.pt()>125){
				    
                                    mon.fillHisto("vbfdetajjfinal",tags,deta,        weight);
                                    if(deta>4.0){mon.fillHisto("vbfdphijjfinal",tags,dphi,        weight);}
				    
                                    if(!isMC){
				      char buffer[1024];
				      sprintf(buffer, " - VBF mjj=%8.2f  dEta=%+6.2f dPhi=%+6.2f\n", dijet.mass(), deta, dphi   ); debugText+=buffer;
				      sprintf(buffer, " - VBF jet1 pT=%6.2f eta=%+6.2f phi=%+6.2f\n", selJets[0].pt(), selJets[0].eta(), selJets[0].phi()   ); debugText+=buffer;
				      sprintf(buffer, " - VBF jet2 pT=%6.2f eta=%+6.2f phi=%+6.2f\n", selJets[1].pt(), selJets[1].eta(), selJets[1].phi()   ); debugText+=buffer;
                                    }
				  }
				  
				}
			      }
			    }
			  }
			}
		      }
		    }
		  }

                  bool isZsideBand    ( (boson.mass()>40  && boson.mass()<70) || (boson.mass()>110 && boson.mass()<200) );
                  if(passQt && passThirdLeptonVeto && passMinDphijmet && passDphi && (boson.mass()>40 && boson.mass()<200)){
                     if(passBtags){
                        if(imet.pt()>50 )mon.fillHisto("zmass_bveto50" , tags,boson.mass(),weight);
                        if(imet.pt()>80 )mon.fillHisto("zmass_bveto80" , tags,boson.mass(),weight);
                        if(imet.pt()>125)mon.fillHisto("zmass_bveto125", tags,boson.mass(),weight);
                        if(passMass){
                            mon.fillHisto( "met_Inbveto",tags,imet.pt(),weight);
                           if(imet.pt()>50 )mon.fillHisto("mt_Inbveto50" , tags,mt,weight);
                           if(imet.pt()>80 )mon.fillHisto("mt_Inbveto80" , tags,mt,weight);
                           if(imet.pt()>125)mon.fillHisto("mt_Inbveto125", tags,mt,weight);
                        }else if(isZsideBand){
                            mon.fillHisto( "met_Outbveto",tags,imet.pt(),weight);
                           if(imet.pt()>50 )mon.fillHisto("mt_Outbveto50" , tags,mt,weight);
                           if(imet.pt()>80 )mon.fillHisto("mt_Outbveto80" , tags,mt,weight);
                           if(imet.pt()>125)mon.fillHisto("mt_Outbveto125", tags,mt,weight);
                        }
                     }else{
                        if(imet.pt()>50 )mon.fillHisto("zmass_btag50" , tags,boson.mass(),weight);
                        if(imet.pt()>80 )mon.fillHisto("zmass_btag80" , tags,boson.mass(),weight);
                        if(imet.pt()>125)mon.fillHisto("zmass_btag125", tags,boson.mass(),weight);
                        if(passMass){
                            mon.fillHisto( "met_Inbtag",tags,imet.pt(),weight);
                           if(imet.pt()>50 )mon.fillHisto("mt_Inbtag50" , tags,mt,weight);
                           if(imet.pt()>80 )mon.fillHisto("mt_Inbtag80" , tags,mt,weight);
                           if(imet.pt()>125)mon.fillHisto("mt_Inbtag125", tags,mt,weight);
                        }else if(isZsideBand){
                            mon.fillHisto( "met_Outbtag",tags,imet.pt(),weight);
                           if(imet.pt()>50 )mon.fillHisto("mt_Outbtag50" , tags,mt,weight);
                           if(imet.pt()>80 )mon.fillHisto("mt_Outbtag80" , tags,mt,weight);
                           if(imet.pt()>125)mon.fillHisto("mt_Outbtag125", tags,mt,weight);
                        }
                     }
                  }

               }


               //if(ivar==0)printf("%9i:%9lli SYST:%30s  Met=%8.3f mT=%8.3f  Weight=%6.2E %i %i %i %i %i\n",  ev.eventAuxiliary().run(), ev.eventAuxiliary().event(), "NOSYST", imet.pt(), higgs::utils::transverseMass(boson,imet,true), weight, passBtags?1:0, passMass?1:0, passQt?1:0, passThirdLeptonVeto?1:0, passMinDphijmet?1:0 );

               if(passBtags && passMass && passQt && passThirdLeptonVeto && passMinDphijmet && passDphi){

                  mon.fillHisto(TString("mtSyst")+varNames[ivar],tags, mt,weight);
                  mon.fillHisto(TString("metSyst")+varNames[ivar],tags, imet.pt(),weight);

                  //scan the MET cut and fill the shapes
                  for(unsigned int index=0;index<optim_Cuts1_met.size();index++){
                     if(imet.pt()>optim_Cuts1_met[index]){
                       for(unsigned int nri=0;nri<NRparams.size();nri++){
                          //Higgs line shape
                          float shapeWeight = 1;   //used for shape dependent weights (avoid overwritting chWeights)
                          
                          if( isMELA ){
                                if(lMelaShapeWeights[0][MelaMode]==0){
                                        shapeWeight = 0;
                                }else{
                                        shapeWeight = (weight*lMelaShapeWeights[nri][MelaMode])/lMelaShapeWeights[0][MelaMode];
                                }
                          } else if( !isMELA ){
                                shapeWeight =  weight;
                          } 
                        
                          if( !isMELA ){
                             double weightToOtherNRI = ( (lShapeWeights[nri][0] * lShapeWeights[nri][1]) / (lShapeWeights[0][0] * lShapeWeights[0][1]) );  //remove weights form nri=0 as those are already in the nominal weight and apply the one for NRI!=0;
                             if(!std::isnan((double)weightToOtherNRI))shapeWeight *= weightToOtherNRI;

                             if( varNames[ivar]=="_signal_normdown"   ) shapeWeight*=lShapeWeights[nri][2];
                             if( varNames[ivar]=="_signal_lshapedown" ) shapeWeight*=lShapeWeights[nri][3];
                             if( varNames[ivar]=="_signal_normup"     ) shapeWeight*=lShapeWeights[nri][4];
                             if( varNames[ivar]=="_signal_lshapeup"   ) shapeWeight*=lShapeWeights[nri][5];
                          } else if( isMELA ){
                             if( varNames[ivar]=="_signal_normdown"   ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                             if( varNames[ivar]=="_signal_lshapedown" ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                             if( varNames[ivar]=="_signal_normup"     ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                             if( varNames[ivar]=="_signal_lshapeup"   ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                          }

                          mon.fillHisto(TString("mt_shapes")+NRsuffix[nri]+varNames[ivar],tags,index, mt,shapeWeight);
                          mon.fillHisto(TString("met_shapes")+NRsuffix[nri]+varNames[ivar],tags,index, imet.pt(),shapeWeight);
                       }
                    }
                 }
              }

               if(passQt && passThirdLeptonVeto && passMinDphijmet && passDphi && (boson.mass()>40 && boson.mass()<200)){
                  bool isZ_SB ( (boson.mass()>40  && boson.mass()<70) || (boson.mass()>110 && boson.mass()<200) );
                  bool isZ_upSB ( (boson.mass()>110 && boson.mass()<200) );
                  //scan the MET cut and fill the shapes
                  for(unsigned int index=0;index<optim_Cuts1_met.size();index++){
                     if(imet.pt()>optim_Cuts1_met[index]){
                       for(unsigned int nri=0;nri<NRparams.size();nri++){
                          //Higgs line shape
                          float shapeWeight = 1; //used for shape dependent weights (avoid overwritting chWeights)

                          if( isMELA ){
                                if(lMelaShapeWeights[0][MelaMode]==0){
                                        shapeWeight = 0;
                                }else{
                                        shapeWeight = (weight*lMelaShapeWeights[nri][MelaMode])/lMelaShapeWeights[0][MelaMode];
                                }
                          } else if( !isMELA ){
                                shapeWeight =  weight;
                          } 
                        
                          if( !isMELA ){
                             double weightToOtherNRI = ( (lShapeWeights[nri][0] * lShapeWeights[nri][1]) / (lShapeWeights[0][0] * lShapeWeights[0][1]) );  //remove weights form nri=0 as those are already in the nominal weight and apply the one for NRI!=0;
                             if(!std::isnan((double)weightToOtherNRI))shapeWeight *= weightToOtherNRI;
                             if( varNames[ivar]=="_signal_normdown"   ) shapeWeight*=lShapeWeights[nri][2];
                             if( varNames[ivar]=="_signal_lshapedown" ) shapeWeight*=lShapeWeights[nri][3];
                             if( varNames[ivar]=="_signal_normup"     ) shapeWeight*=lShapeWeights[nri][4];
                             if( varNames[ivar]=="_signal_lshapeup"   ) shapeWeight*=lShapeWeights[nri][5];
                          }else if( isMELA ){
                             if( varNames[ivar]=="_signal_normdown"   ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                             if( varNames[ivar]=="_signal_lshapedown" ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                             if( varNames[ivar]=="_signal_normup"     ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                             if( varNames[ivar]=="_signal_lshapeup"   ) shapeWeight*=lMelaShapeWeights[nri][MelaMode];
                          }

                          if(passBtags && passMass)mon.fillHisto(TString("mt_shapes_NRBctrl")+NRsuffix[nri]+varNames[ivar],tags,index, 0.5,shapeWeight);
                          if(passBtags && isZ_SB)mon.fillHisto(TString("mt_shapes_NRBctrl")+NRsuffix[nri]+varNames[ivar],tags,index, 1.5,shapeWeight);
                          if(passBtags && isZ_upSB)mon.fillHisto(TString("mt_shapes_NRBctrl")+NRsuffix[nri]+varNames[ivar],tags,index, 2.5,shapeWeight);
                          if(!passBtags && passMass)mon.fillHisto(TString("mt_shapes_NRBctrl")+NRsuffix[nri]+varNames[ivar],tags,index, 3.5,shapeWeight);
                          if(!passBtags && isZ_SB)mon.fillHisto(TString("mt_shapes_NRBctrl")+NRsuffix[nri]+varNames[ivar],tags,index, 4.5,shapeWeight);
                          if(!passBtags && isZ_upSB)mon.fillHisto(TString("mt_shapes_NRBctrl")+NRsuffix[nri]+varNames[ivar],tags,index, 5.5,shapeWeight);

                       }
                    }
                 }
              }
            }
         }        */
     
        
         
         
         
         /*
         for(size_t ijkpho=0; ijkpho<(matchingphoton.size()-1); ijkpho++)
         {
             bool wrong_pt_order (false);
             //pat::Photon photonphoAA = matchingphoton[ijkpho];
             //pat::Photon photonphoBB = matchingphoton[ijkpho+1];
             //double AApt = photonphoAA.pt();
             //double BBpt = photonphoBB.pt();
             
             double AApt = matchingphoton[ijkpho].pt();
             double BBpt = matchingphoton[ijkpho+1].pt();
             
             if ( AApt < BBpt )
                 wrong_pt_order = true;
             //double etaf = matchingphoton[FIRST].eta();
             //double ptf  = matchingphoton[FIRST].pt();
             //double etas = matchingphoton[SECOND].eta();
             //double pts  = matchingphoton[SECOND].pt();
             //if(abs(etaf)>2.5||abs(etas)>2.5)continue;

             mon.fillHisto("wrong_pt_order",";wrong_pt_order;events",  wrong_pt_order,  weight);
             
         
         }
         */
         ///////////////////////////////////////////
         ///   matching higgs mass
         ///////////////////////////////////////////
         float MASSgammagamma;
         LorentzVector Higgsgammagamma(0,0,0,0);
         for(unsigned int iigen=0; iigen<matchingphoton.size(); iigen++){
             Higgsgammagamma += matchingphoton[iigen].p4();
         }
         MASSgammagamma = Higgsgammagamma.mass();
         //pho_numphotons = matching_photon.size();
         
         //////////////////////////////////////////////////// ////////////////////////////////////////////////////////////
         ///Do a selection by pt over matching higgs mass. If passing, the data will be stored into matching_photon.   ///
         //////////////////////////////////////////////////////// ////////////////////////////////////////////////////////
         pat::PhotonCollection matching_photon;
         //float leading_Photon_Pt(0);
         //float subleading_Photon_Pt(0);
         for(size_t iiiipho=0; iiiipho<matchingphoton.size(); iiiipho++){
             pat::Photon photonpho = matchingphoton[iiiipho];
             //if (iiiipho==0) leading_Photon_Pt = photonpho.pt();
             //if (iiiipho==1) subleading_Photon_Pt = photonpho.pt();
             
             if (iiiipho==0 && photonpho.pt()/MASSgammagamma <= 0.375 ) continue;
             if (iiiipho==1 && photonpho.pt()/MASSgammagamma <= 0.25  ) continue;
             
             if (iiiipho==0)
             {
                 VH_leading_pho_pt = photonpho.pt();
                 VH_leadingPhoPtOverHiggsMass = photonpho.pt()/MASSgammagamma;
                 treeVH_leading->Fill();
             }
             
             if (iiiipho==1)
             {
                 VH_subleading_pho_pt = photonpho.pt();
                 VH_SubleadingPhoPtOverHiggsMass = photonpho.pt()/MASSgammagamma;
                 treeVH_subleading->Fill();
             }
             
             matching_photon.push_back(photonpho);
         }

          std::sort(matching_photon.begin(),   matching_photon.end(), utils::sort_CandidatesByPt);
         
         /////////////////////////////////////
         ///// store information about VH  ///
         /////////////////////////////////////
         for(size_t iiiipho=0; iiiipho<matching_photon.size(); iiiipho++)
         {
             pat::Photon photonpho = matching_photon[iiiipho];
             if (iiiipho==0)
             {
                 leading_Photon_Pt = photonpho.pt();
                 //leading_Photon_Pt_OverHiggsMass = leading_Photon_Pt/HIGGS_MASS_gammagamma;
             }
         }
         
         
         LorentzVector V(0,0,0,0);
         LorentzVector VH(0,0,0,0);
         for( int iiigen=0; iiigen<sel_Leptons.size(); iiigen++){
             V += sel_Leptons[iiigen].p4();
         }
         V_MASS_LepLep = V.mass();
         
         LorentzVector Higgs_gamma_gamma(0,0,0,0);
         for(unsigned int iigen=0; iigen<matching_photon.size(); iigen++){
             Higgs_gamma_gamma += matching_photon[iigen].p4();
         }
         HIGGS_MASS_gammagamma = Higgs_gamma_gamma.mass();
         pho_numphotons = matching_photon.size();
         //number_of_vertex = vtx.size();
         //tree1->Fill();
         
         leading_Photon_Pt_OverHiggsMass = leading_Photon_Pt/HIGGS_MASS_gammagamma;
         
         VH = V + Higgs_gamma_gamma;
         
         mon.fillHisto("PT_VH",     "", VH.pt(),         weight);
         mon.fillHisto("Mass_VH",   "", VH.mass(),       weight);
         mon.fillHisto("Et_VH",     "", VH.Et(),         weight);
         mon.fillHisto("Eta_VH",    "", VH.Eta(),        weight);
         mon.fillHisto("Rapidity_VH", "", VH.Rapidity(),       weight);
         mon.fillHisto("ColinearRapidity_VH",     "", VH.ColinearRapidity(),       weight);

         VH_pt = VH.pt();
         VH_mass = VH.mass();
         VH_et = VH.Et();
         VH_eta = VH.Eta();
         VH_rapidity = VH.Rapidity();
         VH_colinearrapidity = VH.ColinearRapidity();

         
  
         double dR_VH = 9999999.;
         double CosTheta = 0;
         double dphi = V.phi()-Higgs_gamma_gamma.phi();
         mon.fillHisto("DeltaPhi_VH", "", dphi,       weight);

         VH_deltaphi = dphi;
         
         if (std::fabs(dphi)>TMath::Pi())
         { dphi = dphi < 0? (2*TMath::Pi()) + dphi : dphi - 2*TMath::Pi() ; }
         double deta = V.eta()-Higgs_gamma_gamma.eta();
         mon.fillHisto("DeltaEta_VH", "", deta,       weight);
        
         VH_deltaeta = deta;
         
         CosTheta = cos(deta);
         dR_VH = sqrt(pow((deta),2) + pow(dphi,2));
         
         mon.fillHisto("CosTheta_VH", "", CosTheta,   weight);
         mon.fillHisto("DeltaR_VH"  , "", dR_VH,         weight);
       
         VH_costhera = CosTheta;
         VH_r = dR_VH;

         treeVH->Fill();
         
         /////////////////////////////////////////
         ///// store information about photon  ///
         /////////////////////////////////////////

         
         for(size_t iiiipho=0; iiiipho<matching_photon.size(); iiiipho++){
             pat::Photon photonpho = matching_photon[iiiipho];
             /*
             if (iiiipho==0 && photonpho.pt()/HIGGS_MASS_gammagamma <= 0.375 ) continue;
             if (iiiipho==1 && photonpho.pt()/HIGGS_MASS_gammagamma <= 0.25  ) continue;

             if (iiiipho==0)
             {
                 VH_leading_pho_pt = photonpho.pt();
                 VH_leadingPhoPtOverHiggsMass = photonpho.pt()/HIGGS_MASS_gammagamma;
                 treeVH_leading->Fill();
             }
             
             if (iiiipho==1)
             {
                 VH_subleading_pho_pt = photonpho.pt();
                 VH_SubleadingPhoPtOverHiggsMass = photonpho.pt()/HIGGS_MASS_gammagamma;
                 treeVH_subleading->Fill();
             }
             
             //treeVH->Fill();
             */
             number_of_vertex = vtx.size();
             //tree1->Fill();
             
             string AAA[4] = {"First_","Second_","Third_","Forth_"};
             
             
             if(iiiipho<4)
             {
                 mon.fillHisto("PHOTON_pT",     AAA[iiiipho].c_str(), sel_Photons[iiiipho].pt(),  weight);
                 mon.fillHisto("PHOTON_Eta",    AAA[iiiipho].c_str(), sel_Photons[iiiipho].eta(), weight);
             }

         
             ////////////////////////////////////////////////////////////////////////////////
             ////////////////////////////////////222////////////////////////////////////////////
             ////////////////////////////////////////////////////////////////////////////////
             
             
             mon.fillHisto("photonEcalIso",         "_matching",      photonpho.ecalRecHitSumEtConeDR04(),       weight);
             mon.fillHisto("photonHcalIso",         "_matching",     photonpho.hcalTowerSumEtConeDR04(),        weight);
             mon.fillHisto("photonTrackSolidIso",   "_matching" ,   photonpho.trkSumPtSolidConeDR04(),         weight);
             mon.fillHisto("photonTrackHollowIso",  "_matching" ,  photonpho.trkSumPtHollowConeDR04(),        weight);
             mon.fillHisto("photonTrackCountSolid", "_matching",  photonpho.nTrkSolidConeDR04(),             weight);
             mon.fillHisto("photonTrackCountHollow","_matching",  photonpho.nTrkHollowConeDR04() ,           weight);
             mon.fillHisto("photonInEBEtagap",      "_matching", photonpho.isEBEtaGap(),                    weight);
             mon.fillHisto("photonInEBPhigap",      "_matching",  photonpho.isEBPhiGap(),                    weight);
             mon.fillHisto("photonInEERinggap",     "_matching",  photonpho.isEERingGap(),                   weight);
             mon.fillHisto("photonInEEDeegap",      "_matching", photonpho.isEEDeeGap(),                    weight);
             mon.fillHisto("photonInEEgap",         "_matching",  photonpho.isEBEEGap(),                     weight);
             mon.fillHisto("photonR9",              "_matching",  photonpho.r9() ,                             weight);
             
             
             // Photon Variables
             mon.fillHisto("photonEt",    "_matching",    photonpho.et(),    weight);
             mon.fillHisto("photonEta",    "_matching",    photonpho.eta(),    weight);
             mon.fillHisto("photonPhi",    "_matching",    photonpho.phi(),    weight);
             mon.fillHisto("photonHoverE",    "_matching",    photonpho.hadronicOverEm(),    weight);
             
             mon.fillHisto("photonPt", "_matching", photonpho.pt(), weight);
             
             
             
             // Photon's SuperCluster Variables
             // eta is with respect to detector (not physics) vertex,
             // thus Et and eta are different from photon.
             float matchsuperClusterEt = (photonpho.superCluster()->energy())/(cosh(photonpho.superCluster()->position().eta()));
             mon.fillHisto("photonScEt",    "_matching",    matchsuperClusterEt,    weight);
             mon.fillHisto("photonScEta",    "_matching",    photonpho.superCluster()->position().eta(),    weight);
             mon.fillHisto("photonScPhi",    "_matching",    photonpho.superCluster()->position().phi(),    weight);
             mon.fillHisto("photonScEtaWidth",    "_matching",    photonpho.superCluster()->etaWidth(),    weight);
             
             
             mon.fillHisto("photonScPhiWidth",    "_matching",    photonpho.superCluster()->phiWidth(),    weight);
             mon.fillHisto("photonScRawEnergy",    "_matching",    photonpho.superCluster()->rawEnergy(),    weight);
             mon.fillHisto("photonScEnergy",    "_matching",    photonpho.superCluster()->energy(),    weight);
             
             mon.fillHisto("photonrho",  "_matching"  ,      rho      ,  weight);
             mon.fillHisto("photone2x2",     "_matching"  ,  photonpho.showerShapeVariables().e2x2   ,  weight);
             mon.fillHisto("photone5x5",     "_matching"  ,  photonpho.e5x5()   ,  weight);
             mon.fillHisto("photoneffSigmaRR",     "_matching"  ,  photonpho.showerShapeVariables().effSigmaRR   ,  weight);
             mon.fillHisto("photonsigmaIetaIeta",  "_matching"  , photonpho.sigmaIetaIeta()  ,  weight);
             mon.fillHisto("photonsigmaEtaEta",  "_matching"  ,  photonpho.sigmaEtaEta()    ,  weight);
             mon.fillHisto("photonsigmaIphiIphi",  "_matching"  ,  photonpho.showerShapeVariables().sigmaIphiIphi    ,  weight);
             mon.fillHisto("photonsigmaIetaIphi",  "_matching"  ,  photonpho.showerShapeVariables().sigmaIetaIphi    ,  weight);
             mon.fillHisto("photonfull5x5_r9",  "_matching"  ,  photonpho.full5x5_r9()          ,  weight);
             
             mon.fillHisto("photonr2x5",  "_matching"  ,  photonpho.r2x5()    ,  weight);
             
             mon.fillHisto("photonE2x2overE5x5",  "_matching"  ,  photonpho.showerShapeVariables().e2x2/photonpho.e5x5()  ,  weight);
             
             mon.fillHisto("photonIso",  "_matching"  , photonpho.photonIso(),weight);
             mon.fillHisto("photonIsosumChargedParticlePt",  "_matching"  , photonpho.sumChargedParticlePt(),weight);
             mon.fillHisto("photonchargedHadronIso",  "_matching"  , photonpho.chargedHadronIso(),weight);
             mon.fillHisto("photonchargedHadronIsoWrongVtx",  "_matching"  , photonpho.chargedHadronIsoWrongVtx(),weight);
             
             mon.fillHisto("Num_Vector_preshowerClusters", "_",photonpho.preshowerClusters().size(),weight);
             mon.fillHisto("photonPreshowerEOverScRawEnergy",  "_matching"  , photonpho.superCluster()->preshowerEnergy()/photonpho.superCluster()->rawEnergy(),weight);
             
             
             
             pho_EE = photonpho.isEE();
             pho_EB = photonpho.isEB();
             pho_ecalRecHitSumEtConeDR04 = photonpho.ecalRecHitSumEtConeDR04();
             pho_hcalTowerSumEtConeDR04  = photonpho.hcalTowerSumEtConeDR04();
             pho_trkSumPtSolidConeDR04   = photonpho.trkSumPtSolidConeDR04();
             pho_trkSumPtHollowConeDR04  = photonpho.trkSumPtHollowConeDR04();
             pho_nTrkSolidConeDR04       = photonpho.nTrkSolidConeDR04();
             pho_nTrkHollowConeDR04      = photonpho.nTrkHollowConeDR04();
             pho_r9                      = photonpho.r9();
             /*
              pho__ecalRecHitSumEtConeDR04 = photon.ecalRecHitSumEtConeDR04();
              pho__hcalTowerSumEtConeDR04  = photon.hcalTowerSumEtConeDR04();
              pho__trkSumPtSolidConeDR04   = photon.trkSumPtSolidConeDR04();
              pho__trkSumPtHollowConeDR04  = photon.trkSumPtHollowConeDR04();
              pho__nTrkSolidConeDR04       = photon.nTrkSolidConeDR04();
              pho__nTrkHollowConeDR04      = photon.nTrkHollowConeDR04();
              pho__R9                      = photon.r9();*/
             
             pho_Et                    = photonpho.et();
             pho_Eta                   = photonpho.eta();
             pho_Phi                   = photonpho.phi();
             pho_HOverE                = photonpho.hadronicOverEm();
             pho_Pt                    = photonpho.pt();
             pho_ScEt                  = matchsuperClusterEt;
             pho_ScEta                 = photonpho.superCluster()->position().eta();
             pho_ScPhi                 = photonpho.superCluster()->position().phi();
             pho_ScEtaWidth            = photonpho.superCluster()->etaWidth();
             pho_ScPhiWidth            = photonpho.superCluster()->phiWidth();
             pho_ScRawEnergy           = photonpho.superCluster()->rawEnergy();
             pho_ScEnergy              = photonpho.superCluster()->energy();
             pho_rho                   = rho;
             pho_E2x2                  = photonpho.showerShapeVariables().e2x2;
             pho_E5x5                  = photonpho.e5x5();
             pho_EffSigmaRR            = photonpho.showerShapeVariables().effSigmaRR;
             pho_SigmaIetaIeta         = photonpho.sigmaIetaIeta();
             pho_SigmaEtaEta           = photonpho.sigmaEtaEta();
             pho_SigmaIphiIphi         = photonpho.showerShapeVariables().sigmaIphiIphi;
             pho_SigmaIetaIphi         = photonpho.showerShapeVariables().sigmaIetaIphi;
             pho_Full5x5_R9            = photonpho.full5x5_r9();
             pho_R2x5                  = photonpho.r2x5();
             pho_E2x2overE5x5          = photonpho.showerShapeVariables().e2x2/photonpho.e5x5();
             pho_PhotonIso             = photonpho.photonIso();
             pho_SumChargedParticlePt  = photonpho.sumChargedParticlePt();
             pho_ChargedHadronIso      = photonpho.chargedHadronIso();
             
             //pho_ChargedHadronIsoWrongVtx      = photonpho.chargedHadronIsoWrongVtx();
             pho_PreshowerEOverScRawEnergy =  photonpho.superCluster()->preshowerEnergy()/photonpho.superCluster()->rawEnergy();
             
             //float maxiso=photon.vtx[0].point()->chargedHadronIso();
             photonpho.setVertex(vtx[0].position());
             float maxiso=photonpho.chargedHadronIso();
             //cout<<"origin: "<<maxiso;
             for(int iijjkk=0;iijjkk<vtx.size();iijjkk++)
             {
                 photonpho.setVertex(vtx[iijjkk].position());
                 
                 //cout<<"   "<<"changed iso: "<<photonpho.chargedHadronIso()<<endl;
                 if(maxiso != photonpho.chargedHadronIso()) cout<<" changed the iso";
                 if(maxiso > photonpho.chargedHadronIso())
                     //maxiso = photon.setVertex(vtx[iijjkk]).chargedHadronIso();
                     // maxiso = photon.setVertex(vtx[iijjkk].position()).chargedHadronIso();
                     maxiso = photonpho.chargedHadronIso();
             }
             pho_ChargedHadronIsoWrongVtx = maxiso;
             
             
             ///tree1->Branch("recPhoton", &pho_ecalRecHitSumEtConeDR04, "pho_ecalRecHitSumEtConeDR04/F:pho_hcalTowerSumEtConeDR04:pho_trkSumPtSolidConeDR04:pho_trkSumPtHollowConeDR04:pho_nTrkSolidConeDR04:pho_nTrkHollowConeDR04:pho_r9");
             tree1->Fill();

             
             
             
             if (photonpho.isEB())
             {
                 
                 mon.fillHisto("photonScEta",           "_matching__EB",   photonpho.superCluster()->position().eta(),           weight);
                 mon.fillHisto("photonEcalIso",         "_matching__EB",      photonpho.ecalRecHitSumEtConeDR04(),       weight);
                 mon.fillHisto("photonHcalIso",         "_matching__EB",     photonpho.hcalTowerSumEtConeDR04(),        weight);
                 mon.fillHisto("photonTrackSolidIso",   "_matching__EB" ,   photonpho.trkSumPtSolidConeDR04(),         weight);
                 mon.fillHisto("photonTrackHollowIso",  "_matching__EB" ,  photonpho.trkSumPtHollowConeDR04(),        weight);
                 mon.fillHisto("photonTrackCountSolid", "_matching__EB",  photonpho.nTrkSolidConeDR04(),             weight);
                 mon.fillHisto("photonTrackCountHollow","_matching__EB",  photonpho.nTrkHollowConeDR04() ,           weight);
                 mon.fillHisto("photonR9",              "_matching__EB",  photonpho.r9() ,                             weight);
                 mon.fillHisto("photonEt",    "_matching__EB",    photonpho.et(),    weight);
                 mon.fillHisto("photonEta",    "_matching__EB",    photonpho.eta(),    weight);
                 mon.fillHisto("photonPhi",    "_matching__EB",    photonpho.phi(),    weight);
                 mon.fillHisto("photonHoverE",    "_matching__EB",    photonpho.hadronicOverEm(),    weight);
                 
                 mon.fillHisto("photonPt", "_matching__EB", photonpho.pt(), weight);
                 
                 mon.fillHisto("photonScEt",    "_matching__EB",    matchsuperClusterEt,    weight);
                 mon.fillHisto("photonScEta",    "_matching__EB",    photonpho.superCluster()->position().eta(),    weight);
                 mon.fillHisto("photonScPhi",    "_matching__EB",    photonpho.superCluster()->position().phi(),    weight);
                 mon.fillHisto("photonScEtaWidth",    "_matching__EB",    photonpho.superCluster()->etaWidth(),    weight);
                 
                 mon.fillHisto("photonScPhiWidth",    "_matching__EB",    photonpho.superCluster()->phiWidth(),    weight);
                 mon.fillHisto("photonScRawEnergy",    "_matching__EB",    photonpho.superCluster()->rawEnergy(),    weight);
                 mon.fillHisto("photonScEnergy",    "_matching__EB",    photonpho.superCluster()->energy(),    weight);
                 
                 mon.fillHisto("photonrho",  "_matching__EB"  ,      rho      ,  weight);
                 mon.fillHisto("photone2x2",     "_matching__EB"  ,  photonpho.showerShapeVariables().e2x2   ,  weight);
                 mon.fillHisto("photone5x5",     "_matching__EB"  ,  photonpho.e5x5()   ,  weight);
                 mon.fillHisto("photoneffSigmaRR",     "_matching__EB"  ,  photonpho.showerShapeVariables().effSigmaRR   ,  weight);
                 mon.fillHisto("photonsigmaIetaIeta",  "_matching__EB"  ,  photonpho.sigmaIetaIeta()  ,  weight);
                 mon.fillHisto("photonsigmaEtaEta",  "_matching__EB"  ,  photonpho.sigmaEtaEta()    ,  weight);
                 mon.fillHisto("photonsigmaIphiIphi",  "_matching__EB"  ,  photonpho.showerShapeVariables().sigmaIphiIphi    ,  weight);
                 mon.fillHisto("photonsigmaIetaIphi",  "_matching__EB"  ,  photonpho.showerShapeVariables().sigmaIetaIphi    ,  weight);
                 mon.fillHisto("photonfull5x5_r9",  "_matching__EB"  ,  photonpho.full5x5_r9()          ,  weight);
                 
                 mon.fillHisto("photonr2x5",  "_matching__EB"  ,  photonpho.r2x5()    ,  weight);
                 
                 mon.fillHisto("photonE2x2overE5x5",  "_matching__EB"  ,  photonpho.showerShapeVariables().e2x2/photonpho.e5x5()  ,  weight);
                 
                 
                 mon.fillHisto("photonIso",  "_matching__EB"  , photonpho.photonIso(),weight);
                 mon.fillHisto("photonIsosumChargedParticlePt",  "_matching__EB"  , photonpho.sumChargedParticlePt(),weight);
                 mon.fillHisto("photonchargedHadronIso",  "_matching__EB"  , photonpho.chargedHadronIso(),weight);
                 mon.fillHisto("photonchargedHadronIsoWrongVtx",  "_matching__EB"  , photonpho.chargedHadronIsoWrongVtx(),weight);
                 
                 mon.fillHisto("photonPreshowerEOverScRawEnergy",  "_matching__EB"  , photonpho.superCluster()->preshowerEnergy()/photonpho.superCluster()->rawEnergy(),weight);
                 
                 
                 pho_ecalRecHitSumEtConeDR04 = photonpho.ecalRecHitSumEtConeDR04();
                 pho_hcalTowerSumEtConeDR04  = photonpho.hcalTowerSumEtConeDR04();
                 pho_trkSumPtSolidConeDR04   = photonpho.trkSumPtSolidConeDR04();
                 pho_trkSumPtHollowConeDR04  = photonpho.trkSumPtHollowConeDR04();
                 pho_nTrkSolidConeDR04       = photonpho.nTrkSolidConeDR04();
                 pho_nTrkHollowConeDR04      = photonpho.nTrkHollowConeDR04();
                 pho_r9                      = photonpho.r9();
                 /*
                  pho__ecalRecHitSumEtConeDR04 = photon.ecalRecHitSumEtConeDR04();
                  pho__hcalTowerSumEtConeDR04  = photon.hcalTowerSumEtConeDR04();
                  pho__trkSumPtSolidConeDR04   = photon.trkSumPtSolidConeDR04();
                  pho__trkSumPtHollowConeDR04  = photon.trkSumPtHollowConeDR04();
                  pho__nTrkSolidConeDR04       = photon.nTrkSolidConeDR04();
                  pho__nTrkHollowConeDR04      = photon.nTrkHollowConeDR04();
                  pho__R9                      = photon.r9();*/
                 
                 pho_Et                    = photonpho.et();
                 pho_Eta                   = photonpho.eta();
                 pho_Phi                   = photonpho.phi();
                 pho_HOverE                = photonpho.hadronicOverEm();
                 pho_Pt                    = photonpho.pt();
                 pho_ScEt                  = matchsuperClusterEt;
                 pho_ScEta                 = photonpho.superCluster()->position().eta();
                 pho_ScPhi                 = photonpho.superCluster()->position().phi();
                 pho_ScEtaWidth            = photonpho.superCluster()->etaWidth();
                 pho_ScPhiWidth            = photonpho.superCluster()->phiWidth();
                 pho_ScRawEnergy           = photonpho.superCluster()->rawEnergy();
                 pho_ScEnergy              = photonpho.superCluster()->energy();
                 pho_rho                   = rho;
                 pho_E2x2                  = photonpho.showerShapeVariables().e2x2;
                 pho_E5x5                  = photonpho.e5x5();
                 pho_EffSigmaRR            = photonpho.showerShapeVariables().effSigmaRR;
                 pho_SigmaIetaIeta         = photonpho.sigmaIetaIeta();
                 pho_SigmaEtaEta           = photonpho.sigmaEtaEta();
                 pho_SigmaIphiIphi         = photonpho.showerShapeVariables().sigmaIphiIphi;
                 pho_SigmaIetaIphi         = photonpho.showerShapeVariables().sigmaIetaIphi;
                 pho_Full5x5_R9            = photonpho.full5x5_r9();
                 pho_R2x5                  = photonpho.r2x5();
                 pho_E2x2overE5x5          = photonpho.showerShapeVariables().e2x2/photonpho.e5x5();
                 pho_PhotonIso             = photonpho.photonIso();
                 pho_SumChargedParticlePt  = photonpho.sumChargedParticlePt();
                 pho_ChargedHadronIso      = photonpho.chargedHadronIso();
                 //pho_ChargedHadronIsoWrongVtx      = photonpho.chargedHadronIsoWrongVtx();
                 pho_PreshowerEOverScRawEnergy =  photonpho.superCluster()->preshowerEnergy()/photonpho.superCluster()->rawEnergy();
                 
                 
                 //float maxiso=photon.vtx[0].point()->chargedHadronIso();
                 photonpho.setVertex(vtx[0].position());
                 float maxiso=photonpho.chargedHadronIso();
                 for(int iijjkk=0;iijjkk<vtx.size();iijjkk++)
                 {
                     photonpho.setVertex(vtx[iijjkk].position());
                     
                     if(maxiso > photonpho.chargedHadronIso())
                         //maxiso = photon.setVertex(vtx[iijjkk]).chargedHadronIso();
                         // maxiso = photon.setVertex(vtx[iijjkk].position()).chargedHadronIso();
                         maxiso = photonpho.chargedHadronIso();
                 }
                 pho_ChargedHadronIsoWrongVtx = maxiso;
                 

                 treeEB->Fill();
                 
                 
             }
             else if (photonpho.isEE())
             {
                 
                 mon.fillHisto("photonScEta",           "_matching__EE",   photonpho.superCluster()->position().eta(),           weight);
                 mon.fillHisto("photonEcalIso",         "_matching__EE",      photonpho.ecalRecHitSumEtConeDR04(),       weight);
                 mon.fillHisto("photonHcalIso",         "_matching__EE",     photonpho.hcalTowerSumEtConeDR04(),        weight);
                 mon.fillHisto("photonTrackSolidIso",   "_matching__EE" ,   photonpho.trkSumPtSolidConeDR04(),         weight);
                 mon.fillHisto("photonTrackHollowIso",  "_matching__EE" ,  photonpho.trkSumPtHollowConeDR04(),        weight);
                 mon.fillHisto("photonTrackCountSolid", "_matching__EE",  photonpho.nTrkSolidConeDR04(),             weight);
                 mon.fillHisto("photonTrackCountHollow","_matching__EE",  photonpho.nTrkHollowConeDR04() ,           weight);
                 mon.fillHisto("photonR9",              "_matching__EE",  photonpho.r9() ,                             weight);
                 mon.fillHisto("photonEt",    "_matching__EE",    photonpho.et(),    weight);
                 mon.fillHisto("photonEta",    "_matching__EE",    photonpho.eta(),    weight);
                 mon.fillHisto("photonPhi",    "_matching__EE",    photonpho.phi(),    weight);
                 mon.fillHisto("photonHoverE",    "_matching__EE",    photonpho.hadronicOverEm(),    weight);
                 
                 mon.fillHisto("photonPt", "_matching__EE", photonpho.pt(), weight);
                 
                 mon.fillHisto("photonScEt",    "_matching__EE",    matchsuperClusterEt,    weight);
                 mon.fillHisto("photonScEta",    "_matching__EE",    photonpho.superCluster()->position().eta(),    weight);
                 mon.fillHisto("photonScPhi",    "_matching__EE",    photonpho.superCluster()->position().phi(),    weight);
                 mon.fillHisto("photonScEtaWidth",    "_matching__EE",    photonpho.superCluster()->etaWidth(),    weight);
                 
                 mon.fillHisto("photonScPhiWidth",    "_matching__EE",    photonpho.superCluster()->phiWidth(),    weight);
                 mon.fillHisto("photonScRawEnergy",    "_matching__EE",    photonpho.superCluster()->rawEnergy(),    weight);
                 mon.fillHisto("photonScEnergy",    "_matching__EE",    photonpho.superCluster()->energy(),    weight);
                 
                 mon.fillHisto("photonrho",  "_matching__EE"  ,      rho      ,  weight);
                 mon.fillHisto("photone2x2",     "_matching__EE"  ,  photonpho.showerShapeVariables().e2x2   ,  weight);
                 mon.fillHisto("photone5x5",     "_matching__EE"  ,  photonpho.e5x5()   ,  weight);
                 mon.fillHisto("photoneffSigmaRR",     "_matching__EE"  ,  photonpho.showerShapeVariables().effSigmaRR   ,  weight);
                 mon.fillHisto("photonsigmaIetaIeta",  "_matching__EE"  ,  photonpho.sigmaIetaIeta()  ,  weight);
                 mon.fillHisto("photonsigmaEtaEta",  "_matching__EE"  ,  photonpho.sigmaEtaEta()    ,  weight);
                 mon.fillHisto("photonsigmaIphiIphi",  "_matching__EE"  ,  photonpho.showerShapeVariables().sigmaIphiIphi    ,  weight);
                 mon.fillHisto("photonsigmaIetaIphi",  "_matching__EE"  ,  photonpho.showerShapeVariables().sigmaIetaIphi    ,  weight);
                 mon.fillHisto("photonfull5x5_r9",  "_matching__EE"  ,  photonpho.full5x5_r9()          ,  weight);
                 
                 mon.fillHisto("photonr2x5",  "_matching__EE"  ,  photonpho.r2x5()    ,  weight);
                 
                 mon.fillHisto("photonE2x2overE5x5",  "_matching__EE"  ,  photonpho.showerShapeVariables().e2x2/photonpho.e5x5()  ,  weight);
                 
                 mon.fillHisto("photonIso",  "_matching__EE"  , photonpho.photonIso(),weight);
                 mon.fillHisto("photonIsosumChargedParticlePt",  "_matching__EE"  , photonpho.sumChargedParticlePt(),weight);
                 mon.fillHisto("photonchargedHadronIso",  "_matching__EE"  , photonpho.chargedHadronIso(),weight);
                 mon.fillHisto("photonchargedHadronIsoWrongVtx",  "_matching__EE"  , photonpho.chargedHadronIsoWrongVtx(),weight);
                 
                 mon.fillHisto("photonPreshowerEOverScRawEnergy",  "_matching__EE"  , photonpho.superCluster()->preshowerEnergy()/photonpho.superCluster()->rawEnergy(),weight);
                 
                 pho_ecalRecHitSumEtConeDR04 = photonpho.ecalRecHitSumEtConeDR04();
                 pho_hcalTowerSumEtConeDR04  = photonpho.hcalTowerSumEtConeDR04();
                 pho_trkSumPtSolidConeDR04   = photonpho.trkSumPtSolidConeDR04();
                 pho_trkSumPtHollowConeDR04  = photonpho.trkSumPtHollowConeDR04();
                 pho_nTrkSolidConeDR04       = photonpho.nTrkSolidConeDR04();
                 pho_nTrkHollowConeDR04      = photonpho.nTrkHollowConeDR04();
                 pho_r9                      = photonpho.r9();
                 /*
                  pho__ecalRecHitSumEtConeDR04 = photon.ecalRecHitSumEtConeDR04();
                  pho__hcalTowerSumEtConeDR04  = photon.hcalTowerSumEtConeDR04();
                  pho__trkSumPtSolidConeDR04   = photon.trkSumPtSolidConeDR04();
                  pho__trkSumPtHollowConeDR04  = photon.trkSumPtHollowConeDR04();
                  pho__nTrkSolidConeDR04       = photon.nTrkSolidConeDR04();
                  pho__nTrkHollowConeDR04      = photon.nTrkHollowConeDR04();
                  pho__R9                      = photon.r9();*/
                 
                 pho_Et                    = photonpho.et();
                 pho_Eta                   = photonpho.eta();
                 pho_Phi                   = photonpho.phi();
                 pho_HOverE                = photonpho.hadronicOverEm();
                 pho_Pt                    = photonpho.pt();
                 pho_ScEt                  = matchsuperClusterEt;
                 pho_ScEta                 = photonpho.superCluster()->position().eta();
                 pho_ScPhi                 = photonpho.superCluster()->position().phi();
                 pho_ScEtaWidth            = photonpho.superCluster()->etaWidth();
                 pho_ScPhiWidth            = photonpho.superCluster()->phiWidth();
                 pho_ScRawEnergy           = photonpho.superCluster()->rawEnergy();
                 pho_ScEnergy              = photonpho.superCluster()->energy();
                 pho_rho                   = rho;
                 pho_E2x2                  = photonpho.showerShapeVariables().e2x2;
                 pho_E5x5                  = photonpho.e5x5();
                 pho_EffSigmaRR            = photonpho.showerShapeVariables().effSigmaRR;
                 pho_SigmaIetaIeta         = photonpho.sigmaIetaIeta();
                 pho_SigmaEtaEta           = photonpho.sigmaEtaEta();
                 pho_SigmaIphiIphi         = photonpho.showerShapeVariables().sigmaIphiIphi;
                 pho_SigmaIetaIphi         = photonpho.showerShapeVariables().sigmaIetaIphi;
                 pho_Full5x5_R9            = photonpho.full5x5_r9();
                 pho_R2x5                  = photonpho.r2x5();
                 pho_E2x2overE5x5          = photonpho.showerShapeVariables().e2x2/photonpho.e5x5();
                 pho_PhotonIso             = photonpho.photonIso();
                 pho_SumChargedParticlePt  = photonpho.sumChargedParticlePt();
                 pho_ChargedHadronIso      = photonpho.chargedHadronIso();
                 //pho_ChargedHadronIsoWrongVtx      = photonpho.chargedHadronIsoWrongVtx();
                 pho_PreshowerEOverScRawEnergy =  photonpho.superCluster()->preshowerEnergy()/photonpho.superCluster()->rawEnergy();
                 
                 //float maxiso=photon.vtx[0].point()->chargedHadronIso();
                 photonpho.setVertex(vtx[0].position());
                 float maxiso=photonpho.chargedHadronIso();
                 for(int iijjkk=0;iijjkk<vtx.size();iijjkk++)
                 {
                     photonpho.setVertex(vtx[iijjkk].position());
                     
                     if(maxiso > photonpho.chargedHadronIso())
                         //maxiso = photon.setVertex(vtx[iijjkk]).chargedHadronIso();
                         // maxiso = photon.setVertex(vtx[iijjkk].position()).chargedHadronIso();
                         maxiso = photonpho.chargedHadronIso();
                 }
                 pho_ChargedHadronIsoWrongVtx = maxiso;
                 
                 
                 treeEE->Fill();

                 
                 
             }
             
             ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
             ////////////////////////////////////////////////222////////////////////////////////////////////////////////////////////
             ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          
         }
         
         mon.fillHisto("numberofphoton", "", photons.size(), weight);
         mon.fillHisto("numberofmatchphoton", "", nmatch, weight);
     
     }
     printf("\n");
     delete file;
  }

  //##############################################
  //########     SAVING HISTO TO FILE     ########
  //##############################################
  //

  //scale all events by 1/N to avoid the initial loop to stupidly count the events
  //mon.Scale(1.0/totalNumEvent);

  TString terminationCmd = "";
  //save control plots to file
  printf("Results save in local directory and moved to %s\n", outUrl.Data());

  //save all to the file
  terminationCmd += TString("mv out.root ") + outUrl + ";";
  //TFile *ofile=TFile::Open("out.root", "recreate");
    
  ofile->cd();
   //ofile=TFile::Open("out.root");
  mon.Write();
  //tree_nvtx->Write();
  //treegammagamma->Write();
  //treeMM->Write();
  tree1->Write();
  treeEE->Write();
  treeEB->Write();
  treeVH->Write();
  treeVH_leading->Write();
  treeVH_subleading->Write();
  ofile->Close();

  if(!isMC && debugText!=""){
     TString outTxtUrl= outUrl + ".txt";
     terminationCmd += TString("mv out.txt ") + outTxtUrl + ";";
     FILE* outTxtFile = fopen("out.txt", "w");
     fprintf(outTxtFile, "%s", debugText.c_str());
     printf("TextFile URL = %s\n",outTxtUrl.Data());
     if(outTxtFile)fclose(outTxtFile);
  }

  //Now that everything is done, dump the list of lumiBlock that we processed in this job
  if(!isMC){
     terminationCmd += TString("mv out.json ") + ((outUrl.ReplaceAll(".root",""))+".json") + ";";
     goodLumiFilter.FindLumiInFiles(urls);
     goodLumiFilter.DumpToJson("out.json");
  }

  system(terminationCmd.Data());

}
