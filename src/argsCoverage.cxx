#include <iostream>
#include <fstream>
#include <iomanip>
#include <tclap/CmdLine.h> // Command line parser
#include "Coverage.h"


using namespace TCLAP;
void argsCoverage(Coverage *coverage, LIMITS::Pole *pole, int argc, char *argv[]) {
  if (coverage==0) return;
  if (pole==0) return;
  //
  pole->initDefault();
  time_t timer;
  time(&timer);
  unsigned int rndSeed=static_cast<unsigned int>(timer);
  //
  try {
    // Create a CmdLine object.
    // Arg1 = string printed at the end whenever --help is used or an error occurs
    // Arg2 = delimiter character between opt and its value
    // Arg3 = version number given when --version is used
    CmdLine cmd("Try again, friend.", ' ', "0.99");

    ValueArg<int>    nLoops(    "","nloops",  "number of loops",    false,1,"int",cmd);

    ValueArg<int>    rSeed(     "","rseed",   "rnd seed" ,          false,rndSeed,"int",cmd);
    ValueArg<int>    rSeedOfs(  "","rseedofs","rnd seed offset" ,   false,0,"int",cmd);

    ValueArg<double> confLevel( "","cl",       "confidence level",false,0.9,"float",cmd);

    ValueArg<int>    method(    "m","method",     "method (1 - FHC2 (def), 2 - MBT)",false,1,"int",cmd);
    SwitchArg        noTabulated("K","notab","do not use tabulated poisson",false);
    cmd.add(noTabulated);
    //
    SwitchArg        doStats("C","stats", "collect statistics",false);
    cmd.add(doStats);
    SwitchArg        doFixSig("S","fixsig", "fixed meas. N(observed)",false);
    cmd.add(doFixSig);

    ValueArg<double> minProb( "","minp",       "minimum probability",false,-1.0,"float",cmd);
    //
    ValueArg<std::string> dump("","dump",    "dump filename",false,"","string",cmd);

    ValueArg<int>    verboseCov(   "V","verbcov", "verbose coverage",false,0,"int",cmd);
    ValueArg<int>    verbosePol(   "W","verbpol", "verbose pole",    false,0,"int",cmd);

    ValueArg<double> sMin(      "","smin",    "min s_true",         false,0.0,"float",cmd);
    ValueArg<double> sMax(      "","smax",    "max s_true",         false,0.2,"float",cmd);
    ValueArg<double> sStep(     "","sstep",   "step s_true",        false,0.1,"float",cmd);

    ValueArg<int>    effDist(   "","effdist",   "efficiency distribution",false,2,"int",cmd);
    ValueArg<double> effSigma(  "","effsigma",  "sigma of efficiency",false,0.2,"float",cmd);
    ValueArg<double> effScale(  "","effscale",  "effscale factor",false,1.0,"float",cmd);
    ValueArg<double> effMin(    "","effmin",    "min eff true",       false,1.0,"float",cmd);
    ValueArg<double> effMax(    "","effmax",    "max eff true",       false,1.0,"float",cmd);
    ValueArg<double> effStep(   "","effstep",   "step eff true",      false,1.0,"float",cmd);

    ValueArg<int>    bkgDist(   "","bkgdist",   "background distribution",false,0,"int",cmd);
    ValueArg<double> bkgSigma(  "","bkgsigma",  "sigma of background",false,0.2,"float",cmd);
    ValueArg<double> bkgScale(  "","bkgscale",  "bkgscale factor",false,1.0,"float",cmd);
    ValueArg<double> bkgMin(    "","bkgmin",    "min bkg true",false,0.0,"float",cmd);
    ValueArg<double> bkgMax(    "","bkgmax",    "max bkg true",false,0.0,"float",cmd);
    ValueArg<double> bkgStep(   "","bkgstep",   "step bkg true",false,1.0,"float",cmd);
    //
    ValueArg<double> beCorr(    "", "corr",   "corr(bkg,eff)",false,0.0,"float",cmd);
   //
    ValueArg<double> dMus(      "", "dmus",     "step size in findBestMu",false,0.002,"float",cmd);
    ValueArg<int>    nMus(      "", "nmus",     "maximum number of steps in findBestMu",false,100,"float",cmd);
    //
    ValueArg<double> threshBS("","threshbs",  "binary search (limit) threshold" ,false,0.0001,"float",cmd);
    ValueArg<double> threshPrec("","threshprec",  "threshold for accepting a cl" ,false,0.0001,"float",cmd);
    //
//     ValueArg<double> hypTestMin( "","hmin",   "hypothesis test min" ,false,0.0,"float",cmd);
//     ValueArg<double> hypTestMax( "","hmax",   "hypothesis test max" ,false,35.0,"float",cmd);
//     ValueArg<double> hypTestStep("","hstep",  "hypothesis test step" ,false,0.01,"float",cmd);
    //
    ValueArg<double> effIntNSigma(  "","effintnsigma","eff num sigma in integral", false,5.0,"float",cmd);
    ValueArg<double> bkgIntNSigma(  "","bkgintnsigma","bkg num sigma in integral", false,5.0,"float",cmd);
    ValueArg<int>    gslIntNCalls(  "","gslintncalls","number of calls in GSL integrator", false,100,"int",cmd);
    //
    ValueArg<double> tabPoleSMin(   "","tabpolesmin", "Pole table: minimum signal", false,0.0,"float",cmd);
    ValueArg<double> tabPoleSMax(   "","tabpolesmax", "Pole table: maximum signal", false,1.0,"float",cmd);
    ValueArg<int>    tabPoleSNStep( "","tabpolesn",   "Pole table: number of signals", false,10,"int",cmd);
    ValueArg<int>    tabPoleNMin(   "","tabpolenmin", "Pole table: minimum N(obs)", false, 0,"int",cmd);
    ValueArg<int>    tabPoleNMax(   "","tabpolenmax", "Pole table: maximum N(obs)", false,10,"int",cmd);
    SwitchArg        tabPole(       "I","poletab",    "Pole table: tabulated",false);
    cmd.add(tabPole);

    ValueArg<double> tabPoisMuMin( "","poismumin", "Poisson table: minimum mean value", false,0.0,"float",cmd);
    ValueArg<double> tabPoisMuMax( "","poismumax", "Poisson table: maximum mean value", false,35.0,"float",cmd);
    ValueArg<int>    tabPoisMuN(   "","poismun",   "Poisson table: number of mean value", false,200,"int",cmd);
    ValueArg<int>    tabPoisNmin(  "","poisnmin",  "Poisson table: minimum value of N", false,0,"int",cmd);
    ValueArg<int>    tabPoisNmax(  "","poisnmax",  "Poisson table: maximum value of N", false,35,"int",cmd);
    SwitchArg        tabPois(     "P","poistab",   "Poisson table: tabulated",false);
    cmd.add(tabPois);

    cmd.parse(argc,argv);
    //
    // First set Pole
    //
    pole->setPoisson(&PDF::gPoisson);
    pole->setGauss(&PDF::gGauss);
    pole->setGauss2D(&PDF::gGauss2D);
    pole->setLogNormal(&PDF::gLogNormal);

    pole->setMethod(method.getValue());
    pole->setVerbose(verbosePol.getValue());
    pole->setNObserved(0);
    pole->setUseCoverage(true);
    pole->setCL(confLevel.getValue());
    //
    pole->setBestMuStep(dMus.getValue());
    pole->setBestMuNmax(nMus.getValue());
    //
    pole->setEffPdfScale( effScale.getValue() );
    pole->setEffPdf( effMin.getValue(), effSigma.getValue(), static_cast<PDF::DISTYPE>(effDist.getValue()) );
    pole->setEffObs();

    pole->setBkgPdfScale( bkgScale.getValue() );
    pole->setBkgPdf( bkgMin.getValue(), bkgSigma.getValue(), static_cast<PDF::DISTYPE>(bkgDist.getValue()) );
    pole->setBkgObs();

    pole->checkEffBkgDists();
    pole->setEffBkgPdfCorr(beCorr.getValue());

    pole->setTrueSignal(sMin.getValue());

    //    pole->getMeasurement().setEffInt(effIntScale.getValue(),effIntN.getValue());
    //    pole->getMeasurement().setBkgInt(bkgIntScale.getValue(),bkgIntN.getValue());

    pole->setIntEffNSigma(effIntNSigma.getValue());
    pole->setIntBkgNSigma(bkgIntNSigma.getValue());
    pole->setIntGslNCalls(gslIntNCalls.getValue());
    //
    pole->setIntSigRange( tabPoleSMin.getValue(), tabPoleSMax.getValue(), tabPoleSNStep.getValue());
    pole->setIntNobsRange(tabPoleNMin.getValue(), tabPoleNMax.getValue());
    pole->setTabulateIntegral(tabPole.getValue());

    pole->setBSThreshold(threshBS.getValue());
    pole->setPrecThreshold(threshPrec.getValue());
    //
    pole->setHypTestRange(0.0,1.0,0.1);//hypTestMin.getValue(), hypTestMax.getValue(), hypTestStep.getValue());

    pole->setMinMuProb(minProb.getValue());
    //
    if (tabPois.getValue()) {
      PDF::gPrintStat = false;
      PDF::gPoisson.initTabulator();
      PDF::gPoisson.setTabMean( tabPoisMuN.getValue(), tabPoisMuMin.getValue(), tabPoisMuMax.getValue() );
      PDF::gPoisson.setTabN(tabPoisNmin.getValue(),tabPoisNmax.getValue());
      PDF::gPoisson.tabulate();
      PDF::gPoisson.clrStat();
    }
    pole->initAnalysis();

    //
    // Now coverage
    //
    coverage->setPole(pole);
    coverage->setDumpBase(dump.getValue().c_str());
    coverage->setVerbose(verboseCov.getValue());
    //
    coverage->collectStats(doStats.getValue());
    coverage->setNloops(nLoops.getValue());
    coverage->setSeed(rSeed.getValue()+rSeedOfs.getValue());
    //
    coverage->setFixedSig(doFixSig.getValue());
    coverage->setSTrue(sMin.getValue(), sMax.getValue(), sStep.getValue());
    coverage->setEffTrue(effMin.getValue(), effMax.getValue(), effStep.getValue());
    coverage->setBkgTrue(bkgMin.getValue(), bkgMax.getValue(), bkgStep.getValue());
  }
  catch (ArgException & e) {
    std::cout << "ERROR: " << e.error() << " " << e.argId() << std::endl;
  }
}
