#ifndef POLE_H
#define POLE_H
//
// The idea is to seperate out Pole-specific code from the Coverage class.
// class Pole
// ----------
// User input:
//   * Measured N(observed), efficiency and background
//   * Distribution of efficiency and background - include the possibility
//     to have arbitrary distributions (a 
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
#include <iomanip>
#include <vector>
#include <cmath>
#include <ctime>
#include <utility>
#include <map>
#include <string>
#include <list>
#include <fstream>
#include <sstream>

#if __GNUC__ < 3
#include <cstdio>
#endif

#include "Tools.h"
#include "Range.h"
#include "Measurement.h"



/*! @class Pole
 *
 *  @brief The main class containing the methods for calculating limits.
 *  
 *  This class calculates confidence intervals for
 *  a Poisson process with background using a frequentist confidence belt
 *  construction.
 *  It assumes that the measurement can be described as follows:
 *  \f[N_{obs} = \epsilon s + b\f]
 *  where
 *  - \f$N_{obs}\f$ = number of observed events (Poisson)
 *  - \f$\epsilon\f$ = measured efficiency with a known (or assumed) distribution
 *  - \f$s\f$ = unknown signal for which the limits are to be found
 *  - \f$b\f$ = measured background with a known (or assumed) distribution
 *
 *  The PDF used for describing the number of observed events is:  
 *  \f[
 *  q(n)_{s+b} =  
 *  \frac{1}{2\pi\sigma_{b}\sigma_{\epsilon}} \times
 *  \intop_0^{\infty}\intop_0^{\infty}p(n)_{b+ 
 *  \epsilon's}\;\;
 *  f_{b,\sigma_b}(b')\;\;
 *  g_{\epsilon,\sigma_{\epsilon}}(\epsilon')\;\;
 *  db'd\epsilon'
 *  \f]
 *  where \f$f()\f$ and \f$g()\f$ are the PDF for the background and efficiency respectively.
 *
 *  In order to find the lower and upper limits, a so called confidence belt is constructed.
 *  For each value of \f$s\f$, \f$n_1\f$ and \f$n_2\f$ are found such that
 *  \f$\sum_{n=n_1}^{n_2} q(n)_{s+b} = 1 - \alpha\f$ where \f$1-\alpha\f$ is the confidence limit.
 *  The confidence belt is then given by the set \f$[n_1(s+b,\alpha),n_2(s+b,\alpha)]\f$.
 *  An upper and lower limit is found by finding the intersection of the vertical line \f$n = N_{obs}\f$
 *  and the boundaries of the belt (not exactly true due to the discreteness of a Poisson).
 *
 *  However, the confidence belt is not unambigously defined. A specific ordering scheme is required.
 *  The method used for selecting \f$n_1\f$ and
 *  \f$n_2\f$ is based on likelihood ratios (aka Feldman & Cousins). For each n, a \f$s_{best}\f$ is found that
 *  will maximise the likelihood \f$\mathcal{L}(n)_{s+b}\f$ (mathematically it's just \f$q(n)_{s+b}\f$,
 *  but here it's not used as a PDF but rather as a hypothesis). For a fixed s, a likelihood ratio is calculated
 *  \f[R(n,s)_{\mathcal{L}} = \frac{\mathcal{L}_{s+b}(n)}{\mathcal{L}_{s_{best}+b}(n)}\f]
 *  and the n are included in the sum starting with the highest rank (ratio) and continuing with decreasing rank until
 *  the sum (ref) equals the requested confidence.
 *
 *  \b SETUP
 *
 *  Basic
 *  - setCL() : Confidence limit, default 0.9
 *    the requested confidence [0.0,1.0]
 *  - setNObserved() : Number of observed events
 *  - setEffPdf() : Measured efficiency\n
 *    efficiency distribution (mean,sigma and distribution type ( DISTYPE ))
 *  - setBkgPdf() : Measured background\n
 *    background distribution (mean,sigma and distribution type ( DISTYPE ))
 *  - setEffBkgCorr() : Correlation between eff and bkg (if applicable)
 *    correlation coefficient [-1.0,1.0]
 *  - setEffObs( double m ) : Set the observed efficiency\n
 *    if no argument is given, the mean of the pdf is used
 *  - setBkgObs( double m ) : Set the observed background\n
 *    see previous
 *
 *  Integration
 *  - setEffInt() : Integration range of efficiency\n
 *    The integration range must cover the PDF such that the tails are
 *    negligable.
 *  - setBkgInt() : Dito, background
 *
 *  Belt construction
 *  - setBelt() : Set the maximum n in the belt construction\n
 *    For large signals and/or events, this value might be increased. 
 *    If the nBelt < 1, then this is automatically selected (see suggestBelt()).
 *  - calcBelt() : Calculates the confidence belt [n1(s,b),n2(s,b)] for all (s,b).
 *  - calcBelt(s,n1,n2,v) : Dito but for a specific (s,b)
 *
 *  Finding \f$s_{best}\f$
 *  - setBestMuStep() : Sets the precision in findBestMu().\n
 *    Default = 0.01 and it should normally be fine.
 *  - setMethod() : sets the Likelihood ratio method (MBT or FHC2)
 *
 *  Hypothesis testing
 *  - setTestHyp() : test range for s+b\n
 *    This sets the range and step size (precision) for finding the limits.\n
 *    The default range is [0.0,35.0] with a step size of 0.01.
 *
 *  Coverage related
 *  - setTrueSignal() : Set the true signal.
 *  - setCoverage() : Set the coverage flag.\n
 *    Setting this flag causes the limit calculation to terminate the scan as soon as it is
 *    decided whether the true signal is inside or outside the confidence limit.
 *
 *  Tabulated PDFs
 *  - initPoisson() : Initialises a tabulated poisson.\n
 *    It is not required but it greatly speeds up the coverage calculations.
 *  - initGauss() : Dito but for a gauss pdf.\n
 *    The gain in speed is much less significant (REMOVE???)
 *
 *  \b RUNNING
 *
 *  Print
 *  - printSetup() : Prints the setup to stdout.
 *  - printLimit() : Prints the calculated limit.
 *
 *  General
 *  - analyseExperiment() : Calculates the limit using the current setup.
 *
 *  Debug
 *  - setVerbose() : Sets verbose level.
 *
 *  \b EXAMPLES
 *  see polelim.cxx (limit calculation) and polecov.cxx (coverage study)
 *
 * @author Fredrik Tegenfeldt (fredrik.tegenfeldt@cern.ch)
 */
//  e^{\frac{-(b-b')^2}{2\sigma_b^2}}\;\;
//  e^{\frac{- (1 -\epsilon')^2}{2\sigma_{\epsilon}^2}}

enum RLMETHOD {
  RL_NONE=0,
  RL_FHC2,
  RL_MBT
};

class Pole {
public:
  /*! @name Constructors/destructor */
  //@{
  //! main constructor
  Pole();

  //! destructor
 ~Pole();
  //@}

  /*! @name Main interfaces */
  //@{
  //! do the limit calculation or coverage determination
  bool analyseExperiment();

  //! generate a random observation (observable + nuisance parameters)
  void generatePseudoExperiment() {
    m_measurement.generatePseudoExperiment();
    m_validBestMu = false;
  }

  //! running with the current setup
  void execute();

  //! execute one event
  void exeEvent(bool first);

  //! execute events from an input file
  void exeFromFile();
  //@}


  /*! @name Set parameters concerning the measurement */

  //! Set input file
  void setInputFile( const char *s ) { m_inputFile = s; }
  //! Set the number of lines to read from the input file
  void setInputFileLines( int nmax ) { m_inputFileLines = nmax; }

  //! Set measurement
  void setMeasurement( const MeasPoisEB & m ) { m_measurement.copy(m); }

  //! set the confidence level
  void setCL(double cl)    { m_cl = cl; if ((cl>1.0)||(cl<0.0)) m_cl=0.9;}

  //! set the CL method to be used
  void setMethod( RLMETHOD m ) { m_method = m; }
  //! set the Cl method to be used (int input)
  void setMethod( int m ) { m_method = RLMETHOD(m); }

  //! set the number of observed events
  void setNObserved(int nobs) { m_nBeltUsed = nobs; m_measurement.setObsVal(nobs); }

  //! set pdf of efficiency
  void setEffPdf(double mean,double sigma, PDF::DISTYPE dist=PDF::DIST_GAUS) {
    m_measurement.setEffPdf(mean,sigma,dist);
    m_validBestMu = false;
  }
  //! set pdf of background
  void setBkgPdf(double mean,double sigma, PDF::DISTYPE dist=PDF::DIST_GAUS) {
    m_measurement.setBkgPdf(mean,sigma,dist);
    m_validBestMu = false;
  }
  //! set pdf mean of efficiency
  void setEffPdfMean( double m ) {
    m_measurement.setEffPdfMean(m);
    m_validBestMu = false;
  }
  //! set pdf sigma of efficiency
  void setEffPdfSigma( double s ) {
    m_measurement.setEffPdfSigma(s);
    m_validBestMu = false;
  }
  //! set pdf mean of background
  void setBkgPdfMean( double m ) {
    m_measurement.setBkgPdfMean(m);
    m_validBestMu = false;
  }
  //! set pdf sigma of background
  void setBkgPdfSigma( double s ) {
    m_measurement.setBkgPdfSigma(s);
    m_validBestMu = false;
  }
  //! set the observed efficiency
  void setEffObs(double mean) {
    m_measurement.setEffObs(mean);
    m_validBestMu = false;
  }
  //! set the observed background
  void setBkgObs(double mean) {
    m_measurement.setBkgObs(mean);
    m_validBestMu = false;
  }
  //! set the observed efficiency using the pdf mean
  void setEffObs() {
    m_measurement.setEffObs();
    m_validBestMu = false;
  }
  //! set the observed background using the pdf mean
  void setBkgObs() {
    m_measurement.setBkgObs();
    m_validBestMu = false;
  }
  //! set eff,bkg correlation...
  void setEffPdfBkgCorr(double corr)    { m_measurement.setBEcorr(corr); }

  //! set the integral range for efficiency
  /*!
    \param scale is the number of sigmas to cover
    \param n is the number of steps in the integral
   */
  void setEffInt( double scale, int n ) { m_measurement.setEffInt(scale,n); }
  //! set the integral range for background
  /*!
    \param scale is the number of sigmas to cover
    \param n is the number of steps in the integral
   */
  void setBkgInt( double scale, int n ) { m_measurement.setBkgInt(scale,n); }

  //! set a scaling number to scale the output limits
  void setScaleLimit(double s) { m_scaleLimit = (s>0.0 ? s:1.0); }
  //@}

  /*! @name Set parameters concerning the precision of the calculations */
  //@{
  //! scan step size for finding s_best
  void setBestMuStep(double dmus,double stepmin=0.001) { m_bestMuStep = (dmus > 0.0 ? dmus:stepmin); }
  //! maximum number of points allowed searching for s_best
  void setBestMuNmax(int n)                            { m_bestMuNmax = n; }

  //! set the cutoff probability for the tails in calcLhRatio()
  /*!
    If the given limit is < 0, then it's calculated as floor(log10(1-CL))-2.0.
   */
  // TODO: need to check this against precision in hypothesis testing
  void setMinMuProb(double m=-1) {
    if (m<0.0) {
      double e=floor(log10(1-m_cl))-2.0;
      m_minMuProb = pow(10.0,e);
    } else {
      m_minMuProb = m;
    }
  }
  //! binary search stopping threshold
  /*!
    The binary search in calcLimit() will stop when the change in mutest is below this value.
    Default value is 0.001.
  */
  void setBSThreshold(double step=0.001)    { m_thresholdBS    = (step>0 ? step:0.001); }
  //! set the threshold defining when a CL is close enough
  /*!
    This value defines when the probability for a signal is close enough to the CL.
    The cutoff is $x = 1.0 - (\alpha_{est}/\alpha_{req})$.
    Default value is 0.01.
   */
  void setAlphaThreshold( double da=0.01)   { m_thresholdAlpha = (da>0 ? da:0.01); }

  //! set range for mutest in calcBelt() etc (NOT used in the limit calculation)
  void setTestHyp(double low, double high, double step);

  //! set maximum difference from unity in normOK()
  void setNormMaxDiff(double dpmax=0.001) { m_normMaxDiff=dpmax; }
  //@}


  /*! @name Coverage specific setup */
  //@{
  //! set the true signal
  void setTrueSignal(double s)   { m_measurement.setTrueSignal(s); }
  //! set the 'use-coverage' flag
  /*!
    If this flag is set true, the calcLimitCoverage() is called instead of calcLimit().
  */
  void setUseCoverage(bool flag) {m_coverage = flag;}
  //@}


  /*! @name Consistency checks - THESE NEED WORK - NOT YET USED */
  //@{
  //! check that the distributions are ok
  bool checkEffBkgDists();
  //! check that all parameters are ok
  bool checkParams() { return true; }
  //@}

  /*! @name Debugging */
  //@{
  //! set verbosity - 0 is no verbosity
  void setVerbose(int v=0) { m_verbose=v; }
  //@}

  /*! @name PDF instances */
  //@{
  void setPoisson(  const PDF::PoisTab   *pdf)  {m_poisson=pdf;}
  void setGauss(    const PDF::Gauss     *pdf)  {m_gauss=pdf;}
  void setGauss2D(  const PDF::Gauss2D   *pdf)  {m_gauss2d=pdf;}
  void setLogNormal(const PDF::LogNormal *pdf)  {m_logNorm=pdf;}
  //@}

  /*! @name Initialising the calculation */
  //@{
  //! initialises integrals etc, calls initIntArrays() among others
  void initAnalysis();
  //! will initialise belt arrays
  void initBeltArrays();
  //@}


  /*! @name Calculation of belt, limits, likelihood ratios etc */
  //@{
  //! finds all mu_best - only used if the method is RLMETHOD::RL_FHC2
  void findAllBestMu();   // dito for all n (loop n=0; n<m_nMuUsed)
  //! finds the mu_best for the given N
  void findBestMu(int n);
  //! calculate the construct
  void calcConstruct();
  //! calculate the construct for the given signal
  void calcConstruct(double s, bool title);
  //! calculate the confidence belt
  void calcBelt();
  //! calculate the confidence belt for the given signal
  double calcBelt(double s, int & n1, int & n2,bool verb, bool title);
  //! calculate the confidence limits
  bool calcLimit();
  //! calculate the confidence limit probability for the given signal
  int  calcLimit(double s);
  //! check if s(true) lies within the limit
  bool calcCoverageLimit();
  //! calculate the likelihood ratio
  double calcLhRatio(double s, int & nb1, int & nb2);
  //! calculate the power
  void calcPower();
  //! calculate minimum N(obs) that will reject s=0
  void calcNMin();
  //! reset all variables pertaining to the limit calculation
  void resetCalcLimit();
  //! checks if the limits are OK
  bool limitsOK();
  //@}

  /*! @name Output */
  //@{
  //! set the print style - not very elaborate at the moment - can be 0 or 1
  void setPrintLimitStyle( int style ) { m_printLimStyle = style; }
  //! print the result of the limit calculation
  void printLimit(bool doTitle=false);
  //! print setup
  void printSetup();
  //! print failure message
  void printFailureMsg();
  //@}

  /*! @name Accessor functions */
  //@{
  const double getCL()                  const { return m_cl; }
  const double getObservedSignal()      const { return m_measurement.getSignal(); }
  const double getTrueSignal()          const { return m_measurement.getTrueSignal(); }
  const bool   getMethod()              const { return m_method; }
  const bool   usesMBT()                const { return (m_method==RL_MBT); }
  const bool   usesFHC2()               const { return (m_method==RL_FHC2); }

  const MeasPoisEB & getMeasurement()   const { return m_measurement; }
  MeasPoisEB &       getMeasurement()         { return m_measurement; }
  const int          getNObserved()     const { return m_measurement.getObsVal(); }

  const double       getEffObs()        const { return m_measurement.getEffObs(); }
  const double       getEffPdfMean()    const { return m_measurement.getEffPdfMean(); }
  const double       getEffPdfSigma()   const { return m_measurement.getEffPdfSigma(); }
  const PDF::DISTYPE getEffPdfDist()    const { return m_measurement.getEffPdfDist(); }

  const double       getBkgObs()        const { return m_measurement.getBkgObs(); }
  const double       getBkgPdfMean()    const { return m_measurement.getBkgPdfMean(); }
  const double       getBkgPdfSigma()   const { return m_measurement.getBkgPdfSigma(); }
  const PDF::DISTYPE getBkgPdfDist()    const { return m_measurement.getBkgPdfDist(); }

  const double       getEffPdfBkgCorr() const { return m_measurement.getBEcorr(); }

  const bool         useCoverage()      const { return m_coverage; }
  const bool         truthCovered()     const { return m_coversTruth; }

  const int            getVerbose()       const { return m_verbose; }

  const Range<double> *getHypTest()       const { return &m_hypTest; }
  //
  const double getEffIntMin() const { return m_measurement.getEff()->getIntXmin(); }
  const double getEffIntMax() const { return m_measurement.getEff()->getIntXmax(); }
  const double getEffIntN()   const { return m_measurement.getEff()->getIntN(); }
  const double getBkgIntMin() const { return m_measurement.getBkg()->getIntXmin(); }
  const double getBkgIntMax() const { return m_measurement.getBkg()->getIntXmax(); }
  const double getBkgIntN()   const { return m_measurement.getBkg()->getIntN(); }
  const double getEffIntNorm() const { return m_measurement.getEff()->getIntegral(); }
  const double getBkgIntNorm() const { return m_measurement.getBkg()->getIntegral(); }
  const double getIntNorm() const { return m_measurement.getNuisanceIntNorm(); }
  //
  const double  getLsbest(int n) const;
  const int     getNBeltUsed() const { return m_nBeltUsed; }
  const int     getNBeltMinUsed() const { return m_nBeltMinUsed; }
  const int     getNBeltMaxUsed() const { return m_nBeltMaxUsed; }
  const bool    isValidBestMu() const  { return m_validBestMu; }
  //
  const double  getBestMuStep() const { return m_bestMuStep; }
  const int     getBestMuNmax() const { return m_bestMuNmax; }
  const std::vector<double> & getBestMuProb() const { return m_bestMuProb; }
  const std::vector<double> & getBestMu() const { return m_bestMu; }
  const std::vector<double> & getMuProb() const { return m_muProb; }
  const std::vector<double> & getLhRatio() const { return m_lhRatio; }
  const double getMinMuProb() const { return m_minMuProb; }
  const double getMuProb(int n) const { if ((n>m_nBeltMaxUsed)||(n<m_nBeltMinUsed)) return 0.0; return m_muProb[n];}
  //
  const double getBSThreshold() const { return m_thresholdBS; }
  const double getAlphaThreshold() const { return m_thresholdAlpha; }
  const double getSumProb() const    { return m_sumProb; }
  const double getLowerLimit() const { return m_lowerLimit; }
  const double getUpperLimit() const { return m_upperLimit; }
  const double getLowerLimitNorm() const { return m_lowerLimitNorm; }
  const double getUpperLimitNorm() const { return m_upperLimitNorm; }
  const double getRejS0P()  const { return m_rejs0P; }
  const int    getRejS0N1() const { return m_rejs0N1; }
  const int    getRejS0N2() const { return m_rejs0N2; }

  const std::string & getInputFile() const { return m_inputFile; }
  const int getInputFileLines() const { return m_inputFileLines; }

  //@}

  /*! @name Some tools */
  //@{
  const bool isFullyCorrelated() const { return false; }
  const bool isNotCorrelated()   const { return true;  }
  const bool normOK(double p)    const { return (fabs(p-1.0)<m_normMaxDiff); }
  //@}

private:

  const PDF::PoisTab   *m_poisson;
  const PDF::Gauss     *m_gauss;
  const PDF::Gauss2D   *m_gauss2d;
  const PDF::LogNormal *m_logNorm;
  //
  int    m_verbose;
  int    m_printLimStyle;
  //
  // CL, confidence limit
  double m_cl;

  // RL method
  RLMETHOD m_method;

  // True signal - used in coverage studies
  bool   m_coverage;
  bool   m_coversTruth;
  // Measurement
  MeasPoisEB m_measurement;
  // correlation between eff and bkg [-1.0..1.0]
  double  m_beCorr;
  ////////////////////////////////////////////////////
  //
  // Test range for belt construction (calcBelt()), power calculation (calcPower()) and construction (calcConstruct())
  Range<double>  m_hypTest;
  //
  // Arrays of best fit and limits
  //
  // POLE
  int     m_nBelt;        // beltsize used for explicit construction of conf. belt (calcBelt() etc)
  int     m_nBeltUsed;    // dynamic beltsize determined in calcLhRatio() - default = max(2,N(obs))
  int     m_nBeltMinUsed; // the minimum nBelt used for the calculation
  int     m_nBeltMaxUsed; // the maximum nBelt used for the calculation
  int     m_nBeltMinLast; // the minimum nBelt used in previous call to calcLhRatio()
  //  std::vector<int> m_nBeltList; // list of suggested nBelt - filled in constructor
  //
  bool    m_validBestMu;//
  double  m_bestMuStep;       // step size in search for s_best (LHR)
  int     m_bestMuNmax;   // maximum N in search for s_best (will locally nodify dmus)
  std::vector<double> m_bestMuProb; // prob. of best mu=e*s+b, index == (N observed)
  std::vector<double> m_bestMu;     // best mu=e*s+b
  std::vector<double> m_muProb;     // prob for mu
  std::vector<double> m_lhRatio;    // likelihood ratio
  double m_minMuProb;  // minimum probability accepted
  //
  double m_thresholdBS; // binary search threshold
  double m_thresholdAlpha; // threshold for accepting a CL in calcLimit(s)
  double m_sumProb;    // sum of probs for conf.belt construction - set by calcLimit()
  double m_prevSumProb;// ditto from previous scan - idem
  double m_scanBeltNorm; // sum(p) for all n used in belt at the current s - idem
  bool   m_lowerLimitFound; // true if lower limit is found
  bool   m_upperLimitFound; // true if an upper limit is found
  double m_lowerLimit; // lowerlimit obtained from ordering (4)
  double m_upperLimit; // upperlimit
  double m_lowerLimitNorm; // sum of the probabilities in the belt given by the lower limit
  double m_upperLimitNorm; // dito upper limit (useful to check whether nbelt was enough or not)
  //
  double m_maxNorm;  // max probability sum (should be very near 1)
  double m_normMaxDiff; // max(norm-1.0) allowed before giving a warning
  double m_rejs0P;  // probability for belt at s=0
  int    m_rejs0N1; // min N at s=0
  int    m_rejs0N2; // ditto max N

  double m_scaleLimit; // a factor to scale the resulting limit (in printLimit() )

  std::string m_inputFile; // input file with data
  int         m_inputFileLines; // number of lines to read; if < 1 => read ALL lines
  //
};
/*!
  Calculates the likelihood of the best fit signal to the given number of observations.
  For RLMETHOD::RL_MBT it will use the result from findBestMu().
  For RLMETHOD::RL_MBT it will use Poisson(n,g) where g = (n>bkg ? n : bkg ).
 */
inline const double Pole::getLsbest(int n) const {
  double rval = 0.0;
  if (usesMBT()) {
    double g;
    if (n>m_measurement.getBkgObs()) {
      g = static_cast<double>(n);
    } else {
      g = m_measurement.getBkgObs();
    }
    rval = m_poisson->getVal(n,g);
  } else {
    rval = m_bestMuProb[n];
  }
  return rval;
}

#endif

