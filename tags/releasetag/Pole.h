#ifndef POLE_H
#define POLE_H
//
// The idea is to seperate out Pole-specific code from the Coverage class.
// class Pole
// ----------
// User input:
//   * Measured N(observed), efficiency and background
//   * Distribution of efficiency and background - include the possibility
//     to have arbitrary distributions (a new class... class Observable)
//   * Optional: an array containing the values of the folded PDF (the double integral)
//   * Optional: true signal - use it to check if it's in- or outside the CL - <Coverage>
// Output:
//   * Lower and upper limit
//   * Optional: if true signal is given, then stop limit search when it's clear
//     that the true signal is in- or outside the range.
// Methods:
//   * Generate weight table (from dists of nuasance-parameters) - POSSIBILITY TO REUSE/SAVE
//   * Generate s_best and R(s,n) - reuse if eff/bkg not changed
//
// class Coverage
// --------------
//
// Input:
//   * Ranges of signal, efficiency, background
//   * Distributions of eff,bkg and correlation
//   * Later: An arbitrary number of observables with their distributions (class Observable)
//
//
//
//

#include <iostream>
#include <vector>
#include <cmath>
#include <sys/times.h>

#include "Range.h"
#include "Tabulated.h"

// enum DISTYPE {
//   DIST_NONE, DIST_GAUS, DIST_POIS, DIST_FLAT;
// };
class Pole {
public:
  Pole();
  ~Pole();
  // Main parameters
  void useLogNormal(bool flag) { m_useLogNormal=flag;} //TMP
  //
  void setCL(double cl)    { m_cl = cl; }
  void setNobserved(int nobs) {m_nObserved = nobs;}
  void setEffDist(double mean,double sigma, bool dist=true); // dists - if dist=false then no distribution!
  void setBkgDist(double mean,double sigma, bool dist=true);
  // POLE construction
  void setEffInt(double low, double high, double step); // efficiency range for integral (7) [AFTER the previous]
  void setEffInt(double scale=5.0, double step=-1.0);   // dito but range given in no. sigma
  void setBkgInt(double low, double high, double step); // dito background
  void setBkgInt(double scale=5.0, double step=-1.0);   // dito but range given in no. sigma
  // Checks the integration range for eff and bkg
  bool checkParams();
  // Set belt max value
  void setBelt(int v)    { m_nBelt = v; }
  void setBeltMax(int v) { m_nBeltMax = v; } //allocated maximum
  int  suggestBelt();                // will suggest a m_nBelt based on no. observed
  void setDmus(double dmus) { m_dmus = (dmus > 0.0 ? dmus:m_stepMin); }
  // POLE test hypothesis range
  void setTestHyp(double low, double high, double step); // test mu=s+b for likelihood ratio
  // POLE true signal, used only if coverage run
  void setTrueSignal(double s) { m_sTrue = s; } // true signal
  void setCoverage(bool flag) {m_coverage = flag;} // true if coverage run
  //
  void setVerbose(int v=0) { m_verbose=v; }
  void initPoisson(int nlambda=10000, int nn=51, double lmbmax=100); // will init poisson table
  void initGauss(int ndata  =10000, double mumax=1000.0); // will init gauss table
  ////// TEMPORARY SOLUTION /////
  double logNormal(double x, double nmean, double nsigma);
  ///////////////////////////////
  //
  void initIntArrays();   // will initialise integral arrays (if needed)
  void initBeltArrays(bool suggest=false);  // will initialise belt arrays (if needed)
  void initIntegral();    // calculates double integral kernal (eff*bkg*db*de) according to setup (7)
  // POLE
  inline double calcProb(int n, double s); // calculates probability (7)
  void findBestMu(int n); // finds the best fit (mu=s+b) for a given n. Fills m_bestMu[n] and m_bestMuProb[n].
  void findAllBestMu();   // dito for all n (loop n=0; n<m_nMuUsed)
  void calcLimit(double s); // calculate (4) and find limits, called from findLimits()
  bool findLimits();        // finds CL limits
  bool findCoverageLimits();//  same as previous but stop if it's obvious that initial true mean is inside or outside
  void printLimit(bool doTitle=false);
  // 
  void printSetup();
  // POLE
  void analyseExperiment();  // makes the double integral and finds the coverage
  //
  // Access functions
  //
  double getStepMin()    { return m_stepMin; }
  double getCL()         { return m_cl; }
  double getSTrue()      { return m_sTrue; }
  bool   getCoverage()   { return m_coverage; }
  int    getNObserved()  { return m_nObserved; }
  double getEffMeas()    { return m_effMeas; }
  double getEffSigma()   { return m_effSigma; }
  bool   isEffNoDist()   { return m_effNoDist; }
  // Background, gaussian
  double getBkgMeas()  { return m_bkgMeas; }
  double getBkgSigma() { return m_bkgSigma; }
  bool   isBkgNoDist() { return m_bkgNoDist; }
  // range and steps in double integral (7), in principle infinite
  Range  *getEffRangeInt() { return &m_effRangeInt; }
  // range and steps in double integral (7)
  Range  *getBkgRangeInt() { return &m_bkgRangeInt; }
  // Test range for the likelihood ratio calculation (4)
  Range  *getHypTest()     { return &m_hypTest; }
  //
  bool    isValidInt()   { return m_validInt; }
  int     getNInt()      { return m_nInt; }
  int     getNIntMax()   { return m_nIntMax; }
  double *getWeightInt() { return m_weightInt; }
  double *getEffInt()    { return m_effInt; }
  double *getBkgInt()    { return m_bkgInt; }
  //
  double  getDmus() { return m_dmus; }
  int     getNBelt() { return m_nBelt; }
  int     getNBeltMax() { return m_nBeltMax; }
  bool    isValidBestMu()   { return m_validBestMu; }
  double *getBestMuProb() { return m_bestMuProb; }
  double *getBestMu() { return m_bestMu; }
  double *getMuProb() { return m_muProb; }
  double *getLhRatio() { return m_lhRatio; }
  //
  double getSumProb()    { return m_sumProb; }
  double getLowerLimit() { return m_lowerLimit; }
  double getUpperLimit() { return m_upperLimit; }
  //  
private:
  bool m_useLogNormal; //TMP

  Poisson m_poisson;
  Gauss   m_gauss;
  //
  int    m_verbose;
  //
  double m_stepMin;
  double m_s2pi;
  // CL, confidence limit
  double m_cl;
  // True signal - used in coverage studies
  double m_sTrue;
  bool   m_coverage;
  // Number of observed events
  int    m_nObserved;
  // Efficiency, gaussian
  double m_effMeas;
  double m_effSigma;
  bool   m_effNoDist;
  // Background, gaussian
  double m_bkgMeas;
  double m_bkgSigma;
  bool   m_bkgNoDist;
  // range and steps in double integral (7), in principle infinite
  Range  m_effRangeInt;
  double m_effIntMin;
  double m_effIntMax;
  double m_effScaleInt;
  // range and steps in double integral (7)
  Range  m_bkgRangeInt;
  double m_bkgIntMin;
  double m_bkgIntMax;
  double m_bkgScaleInt;
  // Test range for the likelihood ratio calculation (4)
  Range  m_hypTest;
  double m_hypTestMin;
  double m_hypTestMax;
  double m_hypTestStep;
  int    m_hypTestN; // obtained from range and stepsize
  //
  // Kernel of double integral (7)
  //
  bool    m_validInt;  // true if integral is valid
  int     m_nInt;      // == m_effIntN*m_bkgIntN
  int     m_nIntMax;   // allocated
  double *m_weightInt; // array containing weights (Gauss(e)*Gauss(b)*de*db), size = n_points
  double *m_effInt;    // array containg e used in integral (e)
  double *m_bkgInt;    // array containg b used in integral (b)
  //
  // Arrays of best fit and limits
  //
  // POLE
  double  m_dmus;       // step size in search for s_best (LHR)
  int     m_nBelt;      // how many Nobs are tested to find R (likelihood ratio)
  int     m_nBeltMax;   // dito allocated
  std::vector<int> m_nBeltList; // list of suggested nBelt - filled in constructor
  bool    m_validBestMu;//
  double *m_bestMuProb; // prob. of best mu=e*s+b, index == (N observed)
  double *m_bestMu;     // best mu=e*s+b
  double *m_muProb;     // prob for mu
  double *m_lhRatio;    // likelihood ratio
  double m_sumProb;    // sum of probs for conf.belt construction
  bool   m_foundLower; // true if lower limit is found
  bool   m_foundUpper; // true if an upper limit is found
  double m_lowerLimit; // lowerlimit obtained from ordering (4)
  double m_upperLimit; // upperlimit
  //
};

inline double Pole::calcProb(int n, double s) {  
  int i;
  double p = 0.0; 
  //
  for(i=0;i<m_nInt;i++) {
    p += m_weightInt[i]*m_poisson.getVal(n,m_effInt[i]*s + m_bkgInt[i]);
  }
  return p;
}

// TEMPORARY solution!!!
inline double Pole::logNormal(double x, double nmean, double nsigma) {
  return (x>0.0 ? m_gauss.getVal(log(x),nmean,nsigma)/x:0.0);
}
#endif
