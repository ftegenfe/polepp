//
// Issues:: Number of points in the integral construction -> check! setBkgInt
//
//
//
// This code contains classes for calculating the coverage of a CL
// belt algorithm.
//
#include <iostream>
#include <fstream>
#include <iomanip>

#include "Pole.h"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

inline char *yesNo(bool var) {
  static char yes[4] = {'Y','e','s',char(0)};
  static char no[3] = {'N','o',char(0)};
  return (var ? &yes[0]:&no[0]);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/*!
  Main constructor.
 */
Pole::Pole() {
  m_nUppLim = 10;
  m_normMaxDiff = 0.001;

  m_effDist = DIST_GAUS;
  m_bkgDist = DIST_GAUS;
  //
  m_stepMin = 0.001;
  //
  m_dmus = 0.01;
  //
  m_nObserved = -1;
  //
  m_intNdef = 20;    // default number of points (when step<=0)
  m_effIntScale = 5.0;
  m_bkgIntScale = 5.0;
  //
  m_hypTest.setRange(0.0,35.0,0.01);
  //
  m_validInt  = false;
  m_nInt      = 0;
  m_weightInt = 0;
  m_effInt    = 0;
  m_bkgInt    = 0;
  //
  m_validBestMu = false;
  m_nBelt       = 0;
  m_nBeltMax    = 0;
  m_suggestBelt = true;
  m_muProb      = 0;
  m_bestMuProb  = 0;
  m_bestMu      = 0;
  m_lhRatio     = 0;
  //
  // init list of suggested nBelt
  //
//   m_nBeltList.push_back(18); //0
//   m_nBeltList.push_back(20);
//   m_nBeltList.push_back(20);
//   m_nBeltList.push_back(22);
//   m_nBeltList.push_back(22);
//   m_nBeltList.push_back(23); //5
//   m_nBeltList.push_back(25);
//   m_nBeltList.push_back(32);
//   m_nBeltList.push_back(38);
//   m_nBeltList.push_back(38);
//   m_nBeltList.push_back(40); //10
  //
  m_useNLR = false;
}

Pole::~Pole() {
  // delete arrays
  if (m_weightInt)  delete [] m_weightInt;
  if (m_effInt)     delete [] m_effInt;
  if (m_bkgInt)     delete [] m_bkgInt;
  //
  if (m_bestMuProb) delete [] m_bestMuProb;
  if (m_muProb)     delete [] m_muProb;
  if (m_bestMu)     delete [] m_bestMu;
  if (m_lhRatio)    delete [] m_lhRatio;
}

void Pole::setEffMeas(double mean,double sigma, DISTYPE dist) {
  m_effMeas   = mean;
  m_effSigma  = sigma;
  m_effDist   = dist;
  m_validInt    = false;
  m_validBestMu = false;
}

void Pole::setBkgMeas(double mean,double sigma, DISTYPE dist) {
  m_bkgMeas   = mean;
  m_bkgSigma  = sigma;
  m_bkgDist   = dist;
  m_validInt    = false;
  m_validBestMu = false;
}
bool Pole::checkEffBkgDists() {
  bool change=false;
  // If ONLY one is DIST_GAUSCORR, make bkgDist == effDist
  if ( ((m_effDist == DIST_GAUSCORR) && (m_bkgDist != DIST_GAUSCORR)) ||
       ((m_effDist != DIST_GAUSCORR) && (m_bkgDist == DIST_GAUSCORR)) ) {
    m_bkgDist = m_effDist;
    change = true;
  }
  if (m_effDist != DIST_GAUSCORR) {
    m_beCorr = 0;
    change = true;
  }
  //
  if (m_effDist==DIST_NONE) {
    m_effSigma = 0.0;
    change = true;
  }
  if (m_bkgDist==DIST_NONE) {
    m_bkgSigma = 0.0;
    change = true;
  }
  return change;
}
//
// Calculates the range for integration.
//
void Pole::setInt(double & low, double & high, double & step, double scale, double mean, double sigma, DISTYPE dist) {
  //
  double dx;
  double nsigma;
  double nmean;
  //
  if (dist==DIST_NONE) {
    low  = mean;
    high = mean;
    step = 1.0;
  } else {
    switch (dist) {
    case DIST_GAUSCORR:
    case DIST_GAUS:
      low  = mean - scale*sigma;
      high = mean + scale*sigma;
      break;
    case DIST_FLAT:
      dx=sigma*1.73205081; // == sqrt(12)*0.5; ignore scale - always use full range
      low  = mean-dx;
      high = mean+dx;
      break;
    case DIST_LOGN:
      nmean  = m_gauss.getLNMean(mean,sigma);
      nsigma = m_gauss.getLNSigma(mean,sigma);
      low  = exp(nmean - scale*nsigma);
      high = exp(nmean + scale*nsigma);
      break;
    default: // ERROR STATE
      break;
    }
    if (low<0) { // lower limit below 0 - shift the range
      high-=low;
      low=0;
    }
    if (step<=0.0) step = (high-low)/m_intNdef;     // default N pts
  }
}

void Pole::setEffInt(double scale,double step) {
  m_validInt = false;
  m_validBestMu = false;
  //
  double low,high;
  if (isFullyCorrelated() && (m_effDist == DIST_GAUSCORR)) { // Fully correlated - integrate only bkg with sigma/sqrt(2)
    low = m_effMeas;
    high = m_effMeas;
    step = 1.0;
  } else {
    if (step<=0) step = m_effRangeInt.step();
    if (scale<=0) {
      scale = m_effIntScale;
    } else {
      m_effIntScale = scale;
    }
    setInt(low,high,step,scale,m_effMeas,m_effSigma,m_effDist);
  }
  //
  m_effRangeInt.setRange(low,high,step);
}

void Pole::setBkgInt(double scale,double step) {
  m_validInt = false;
  m_validBestMu = false;
  //
  double low,high;
  if (step<=0)  step  = m_bkgRangeInt.step();
  if (scale<=0) {
    scale = m_bkgIntScale;
  } else {
    m_bkgIntScale = scale;
  }
  setInt(low,high,step,scale,m_bkgMeas,m_bkgSigma,m_bkgDist);
  //
  m_bkgRangeInt.setRange(low,high,step);
}

void Pole::setTestHyp(double low, double high, double step) {
  if (high>low) {
    if (step<=0) step=(high-low)/1000.0;
  } else {
    low = high;
    step = 1.0;
  }
  m_hypTest.setRange(low,high,step);
}
//
// MAYBE REMOVE THIS // HERE
bool Pole::checkParams() {
  bool rval=true;
  std::cout << "<<Pole::CheckParams() is disabled - Manually make sure that the integration limits are ok>>" << std::endl;
  return rval;
  /////////////////////////////////////////
  // check efficiency distribution
  // check background distribution
  // check true signal range
  // check mu_test range
  // check efficiency integration
  std::cout << "Checking efficiency integration - ";
  double dsLow, dsHigh;
  dsLow  = (m_effMeas - m_effRangeInt.min())/m_effSigma;
  dsHigh = (m_effRangeInt.max()  - m_effMeas)/m_effSigma;
  if ( (m_effDist!=DIST_NONE) && ( ((dsLow<4.0)&&(m_effRangeInt.min()>0)) ||
                                   (dsHigh<4.0) ) ) {
    std::cout << "Not OK" << std::endl;
    std::cout << "  Efficiency range for integration does not cover 4 sigma around the true efficiency." << std::endl;
    std::cout << "  Change range or efficiency distribution." << std::endl;
    rval = false;
  } else {
    std::cout << "OK" << std::endl;
  }
  // check background integration
  std::cout << "Checking background integration - ";
  dsLow  = (m_bkgMeas - m_bkgRangeInt.min())/m_bkgSigma;
  dsHigh = (m_bkgRangeInt.max()  - m_bkgMeas)/m_bkgSigma;
  if ( (m_bkgDist!=DIST_NONE) && ( ((dsLow<4.0)&&(m_bkgRangeInt.min()>0)) ||
			    (dsHigh<4.0) ) ) {
    std::cout << "Not OK" << std::endl;
    std::cout << "  Background range for integration does not cover 4 sigma around the true background." << std::endl;
    std::cout << "  Change range or background distribution." << std::endl;
    rval = false;
  } else {
    std::cout << "OK" << std::endl;
  }
  return rval;
}

//

void Pole::initPoisson(int nlambda, int nn, double lmbmax) {
  m_poisson.init(nlambda, nn, lmbmax);
}

void Pole::initGauss(int ndata, double mumax) {
  m_gauss.init(ndata, mumax);
}

//
// Must be called after setEffInt, setBkgInt and setTestHyp
//
void Pole::initIntArrays() {
  m_nInt = m_effRangeInt.n()*m_bkgRangeInt.n();
  if ((m_weightInt==0) || (m_nInt>m_nIntMax)) {
    m_nIntMax = m_nInt*2; // make a safe margin
    if (m_weightInt) { // arrays needs resizing
      delete [] m_weightInt;
      delete [] m_effInt;
      delete [] m_bkgInt;
    }
    m_weightInt = new double[m_nIntMax];
    m_effInt    = new double[m_nIntMax];
    m_bkgInt    = new double[m_nIntMax];
  }
}

// int Pole::suggestBelt() {
//   int rval=50; // default value
//   int nbelt = static_cast<int>(m_nBeltList.size());
//   if (m_nObserved>=0) {
//     if (m_nObserved<nbelt) {
//       rval = m_nBeltList[m_nObserved];
//     } else {
//       rval = m_nObserved*4;
//     }
//   }
//   return rval;
// }


int Pole::suggestBelt() {
  int rval=50;
  if (m_nObserved>=0) {
    rval = BeltEstimator::getBeltMin(m_nObserved, m_bkgMeas) + 5;
    if (rval<20) rval=20; // becomes less reliable for small n
  }
  if (m_verbose>1) std::cout << "Pole::suggestBelt() : Belt range = " << m_nBelt << std::endl;
  return rval;
}

void Pole::initBeltArrays() {
  if (m_suggestBelt) m_nBelt = suggestBelt();
  //
  if ((m_muProb==0) || (m_nBelt>m_nBeltMax)) {
    m_nBeltMax = m_nBelt*2;
    if (m_muProb) { // arrays needs resizing
      delete [] m_muProb;
      delete [] m_bestMuProb;
      delete [] m_bestMu;
      delete [] m_lhRatio;
    }
    m_muProb     = new double[m_nBeltMax];
    m_bestMuProb = new double[m_nBeltMax];
    m_bestMu     = new double[m_nBeltMax];
    m_lhRatio    = new double[m_nBeltMax];
  }
}

void Pole::initIntegral() {
  if (m_validInt) return;
  //
  static bool firstEffWarn=true;
  static bool firstBkgWarn=true;
  int i,j;
  double effs;
  double bkgs;
  double eff_prob, bkg_prob, norm_prob;
  double sum_eff_prob=0;
  double sum_bkg_prob=0;
  int count=0;
  norm_prob = 1.0;
  bool full2d = false;
  /////////////////////////////////
  double sdetC=0, detC=0, seff=0, sbkg=0, vceff=0;
  double effMean,effSigma;
  double bkgMean,bkgSigma;
  if (m_effDist==DIST_LOGN) {
    effMean  = m_gauss.getLNMean(m_effMeas,m_effSigma);
    effSigma = m_gauss.getLNSigma(m_effMeas,m_effSigma);
  } else {
    effMean  = m_effMeas;
    effSigma = m_effSigma;
  }
  if (m_bkgDist==DIST_LOGN) {
    bkgMean  = m_gauss.getLNMean(m_bkgMeas,m_bkgSigma);
    bkgSigma = m_gauss.getLNSigma(m_bkgMeas,m_bkgSigma);
  } else {
    bkgMean  = m_bkgMeas;
    bkgSigma = m_bkgSigma;
  }
  if (m_effDist==DIST_GAUSCORR) {
    if (isFullyCorrelated() || isNotCorrelated()) {
      full2d=false;
    } else {
      full2d=true;
      detC = m_gauss.getDetC(m_effSigma,m_bkgSigma,m_beCorr);
      sdetC = sqrt(detC);
      vceff = m_gauss.getVeffCorr(detC,m_effSigma,m_bkgSigma,m_beCorr);
      seff  = sqrt(m_gauss.getVeff(detC,m_bkgSigma)); // effective sigma for efficiency v1eff = detC/(s2*s2)
      sbkg  = sqrt(m_gauss.getVeff(detC,m_effSigma)); // dito for background            v2eff = detC/(s1*s1)
      if (m_verbose>2) {
	std::cout << "DetC = " << detC << std::endl;
	std::cout << "sDetC = " << sdetC << std::endl;
	std::cout << "Vceff = " << vceff << std::endl;
	std::cout << "seff = " << seff << std::endl;
	std::cout << "sbkg = " << sbkg << std::endl;
	std::cout << "corr = " << m_beCorr << std::endl;
      }
    }
  }
  //////////////////////////////////
  //
  bool dosumbkg = false;
  double dedb;
  double de=1.0;
  double db=1.0;
  if (m_effRangeInt.n() != 1) de=m_effRangeInt.step();
  if (m_bkgRangeInt.n() != 1) db=m_bkgRangeInt.step();
  dedb = de*db;

//   if ((m_effRangeInt.n() != 1) && (m_bkgRangeInt.n() != 1)) { // both eff and bkg vary
//     dedb = m_bkgRangeInt.step()*m_effRangeInt.step();
//   } else {
//     if ((m_effRangeInt.n() != 1) && (m_bkgRangeInt.n() == 1)) { // bkg const
//       dedb = m_effRangeInt.step();
//     } else {
//       if ((m_effRangeInt.n() == 1) && (m_bkgRangeInt.n() != 1)) {  // eff const
// 	dedb = m_bkgRangeInt.step();
//       }
//     }
//   }
  if (m_verbose>1) {
    std::cout << "InitMatrix: eff dist (mu,s)= " << m_effMeas << "\t"
	      << m_effSigma << std::endl;
    std::cout << "InitMatrix: bkg dist (mu,s)= " << m_bkgMeas << "\t"
	      << m_bkgSigma << std::endl;
    std::cout << "InitMatrix: dedb           = " << dedb << std::endl;
  }
  for (i=0;i<m_effRangeInt.n();i++) { // Loop over all efficiency points
    effs =  i*m_effRangeInt.step() + m_effRangeInt.min();
    if ( m_effRangeInt.n() == 1 ) {
      eff_prob = 1.0;
    } else {
      switch(m_effDist) {
      case DIST_GAUS:
	eff_prob = m_gauss.getVal(effs,m_effMeas,m_effSigma);
	//	std::cout << "GAUSSEFF: " << effs << " " << m_effMeas << " " << m_effSigma << " -> " << eff_prob << std::endl;
	break;
      case DIST_LOGN:
	eff_prob = m_gauss.getValLogN(effs,effMean,effSigma);
	break;
      case DIST_GAUSCORR:
	if (isNotCorrelated()) {
	  eff_prob = m_gauss.getVal(effs,m_effMeas,m_effSigma);
	} else {
	  eff_prob = 1.0; // will be set in the bkg loop
	}
	break;
      case DIST_FLAT:
	if (m_effSigma>0) {
	  eff_prob = 1.0/(m_effSigma*3.46410161513775);
	} else {
	  eff_prob = 1.0;
	}
	break;
      case DIST_NONE:
	eff_prob = 1.0;
	break;
      default:
	eff_prob = 1.0;
	break;
      }
    }
    sum_eff_prob += eff_prob;
    //
    for (j=0;j<m_bkgRangeInt.n();j++) { // Loop over all background points
      bkgs =  j*m_bkgRangeInt.step() + m_bkgRangeInt.min();
      if ( m_bkgRangeInt.n() == 1 ) {
	bkg_prob = 1.0;
	dosumbkg = (i==0);
      } else {
	switch(m_bkgDist) {
	case DIST_GAUS:
	  bkg_prob = m_gauss.getVal(bkgs,m_bkgMeas,m_bkgSigma);
	  dosumbkg = (i==0);
	  //	  std::cout << "GAUSSBKG: " << bkgs << " " << m_bkgMeas << " " << m_bkgSigma << " -> " << bkg_prob << std::endl;
	  break;
	case DIST_LOGN:
	  bkg_prob = m_gauss.getValLogN(bkgs,bkgMean,bkgSigma);
	  dosumbkg = (i==0);
	  break;
	case DIST_GAUSCORR:
	  if (isFullyCorrelated()) {
	    bkg_prob = m_gauss.getVal(bkgs,m_bkgMeas,m_bkgSigma/sqrt(2.0));
	    dosumbkg = (i==0);
	  } else {
	    if (isNotCorrelated()) {
	      bkg_prob = m_gauss.getVal(bkgs,m_bkgMeas,m_bkgSigma);
	      dosumbkg = (i==0);
	    } else {
	      bkg_prob = m_gauss.getVal2D(effs,m_effMeas,bkgs,m_bkgMeas,sdetC,seff,sbkg,vceff);
	      dosumbkg = true;
	    }
	  }
	  break;
	case DIST_FLAT:
	  if (m_bkgSigma>0) {
	    bkg_prob = 1.0/(m_bkgSigma*3.46410161513775);
	  } else {
	    bkg_prob = 1.0;
	  }
	  dosumbkg = (i==0);
	  break;
	case DIST_NONE:
	  bkg_prob = 1.0;
	  dosumbkg = (i==0);
	  break;
	default:
	  bkg_prob = 1.0;
	  dosumbkg = (i==0);
	  break;
	}
      }
      if (dosumbkg) sum_bkg_prob += bkg_prob; // only for first loop
      norm_prob = eff_prob*bkg_prob*dedb;
      //
      // Fill the arrays (integral)
      //
      if( (m_effRangeInt.n() == 1) && (m_bkgRangeInt.n() == 1) ) {
	m_bkgInt[count]    = m_bkgMeas;
	m_effInt[count]    = m_effMeas;
	m_weightInt[count] = 1.0;
      } else {
	m_bkgInt[count]    = bkgs;
	m_effInt[count]    = effs;
	m_weightInt[count] = norm_prob;
      }
      //
      //      if (m_verbose>1) std::cout << "Matrix: (" << i << ", " <<  j << ") -> (" << m_effInt[count] << ", " << m_bkgInt[count] << ") " << m_weightInt[count] << "\n";
      //
      count++;
      if (count>m_nInt) {
	std::cout << "WARNING: Array index overflow. count = " << count
		  << ", n_e = " << m_effRangeInt.n()
		  << ", n_b = " << m_bkgRangeInt.n() << std::endl;
      }
    }
  }
  double norm_bkg = sum_bkg_prob*db;
  double norm_eff = sum_eff_prob*de;
  if (full2d) {
    norm_bkg *= de;  // if the distribution is a 2d gauss, the sum is over all area elements
    norm_eff  = 1.0; // per construction
  }
  double norm = norm_bkg*norm_eff;
  //
  for (i=0; i<count; i++) {
    m_weightInt[i] = m_weightInt[i]/norm;
  }
  if (firstEffWarn && (!full2d) && (!normOK(norm_eff))) {
    std::cout << std::endl;
    std::cout << "WARNING: normalisation of EFF is off: " << norm_eff << std::endl;
    std::cout << "         This might be OK if the intention is to use a bounded pdf." << std::endl;
    std::cout << "         The pdf is reweighted such that it is normalised." << std::endl;
    std::cout << "--- MESSAGE IS NOT REPEATED! ---" << std::endl;
    std::cout << std::endl;
    firstEffWarn = false;
  }
  if (firstBkgWarn && (!normOK(norm_bkg))) {
    std::cout << std::endl;
    std::cout << "WARNING: normalisation of BKG is off: " << norm_bkg << std::endl;
    std::cout << "         This might be OK if the intention is to use a bounded pdf." << std::endl;
    std::cout << "         The pdf is reweighted such that it is normalised." << std::endl;
    std::cout << "--- MESSAGE IS NOT REPEATED! ---" << std::endl;
    std::cout << std::endl;
    firstBkgWarn = false;
  }
  if (m_verbose>1) {
    std::cout << "First 10 from double integral (bkg,eff,w):" << std::endl;
    std::cout << "Eff: " << m_effRangeInt.n()   << "\t" << m_effRangeInt.step()
	      << "\t"    << m_effRangeInt.min() << "\t" << m_effRangeInt.max() << std::endl;
    std::cout << "Bkg: " << m_bkgRangeInt.n()   << "\t" << m_bkgRangeInt.step()
	      << "\t"    << m_bkgRangeInt.min() << "\t" << m_bkgRangeInt.max() << std::endl;
    for (i=0; i<10; i++) {
      std::cout << m_bkgInt[i] << "\t" << m_effInt[i] << "\t"
		<< m_weightInt[i] << std::endl;
    }
  }
  m_validInt = true;
}



void Pole::findBestMu(int n) {
  // finds the best fit (mu=s) for a given n. Fills m_bestMu[n]
  // and m_bestMuProb[n].
  int i;
  double mu_test,lh_test,mu_best,mu_s_max,mu_s_min;
  //,dmu_s;
  double lh_max = 0.0;
  //
  mu_best = 0;
  if(n<m_bkgMeas) {
    m_bestMu[n] = 0; // best mu is 0
    m_bestMuProb[n] = calcProb(n,0);
  } else {
    mu_s_max = double(n)-m_bkgMeas;
    //    mu_s_max = (double(n) - m_bkgRangeInt.min())/m_effRangeInt.min();
    //    mu_s_min = (double(n) - m_bkgRangeInt.max())/m_effRangeInt.max();
    mu_s_min = (double(n) - m_bkgRangeInt.max())/m_effRangeInt.max();
    if(mu_s_min<0) {mu_s_min = 0.0;}
    //    dmu_s = 0.01; // HARDCODED:: Change!
    int ntst = 1+int((mu_s_max-mu_s_min)/m_dmus);
    //// TEMPORARY CODE - REMOVE /////
    //    ntst = 1000;
    //    m_dmus = (mu_s_max-mu_s_min)/double(ntst);
    //////////////////////////////////
    if (m_verbose>1) std::cout << "FindBestMu range: " << " I " << m_bkgRangeInt.max() << " " << m_effRangeInt.max() << " "
			       << n << " " << m_bkgMeas << " " << ntst << " [" << mu_s_min << "," << mu_s_max << "] => ";
    for (i=0;i<ntst;i++) {
      mu_test = mu_s_min + i*m_dmus;
      lh_test = calcProb(n,mu_test);
      if(lh_test > lh_max) {
	lh_max = lh_test;
	mu_best = mu_test;
      }
    }  
    if (m_verbose>1) std::cout <<"s_best = " << mu_best << ", LH = " << lh_max << std::endl;
    m_bestMu[n] = mu_best; m_bestMuProb[n] = lh_max;  
  }
}

void Pole::findAllBestMu() {
  if (m_validBestMu) return;
// fills m_bestMuProb and m_bestMu (L(s_best + b)[n])
  for (int n=0; n<m_nBelt; n++) {
    findBestMu(n);
  }
  if (m_verbose>1) {
    std::cout << "First 10 from best fit (mean,prob):" << std::endl;
    for (int i=0; i<10; i++) {
      std::cout << m_bestMu[i] << "\t" << m_bestMuProb[i] << std::endl;
    }
  }
  m_validBestMu = true;
}

double Pole::calcLhRatio() {
  double norm_p = 0;
  if (m_useNLR) { // use method by Gary Hill
    double pbf;
    for (int n=0; n<m_nBelt; n++) {
      m_muProb[n] =  calcProb(n, s);
      if (n>m_bkgMeas) {
	pbf = static_cast<double>(n);
      } else {
	pbf = 0;
      }
      pbf = m_poisson.getVal(n,pbf);
      m_lhRatio[n]  = m_muProb[n]/pbf;
      norm_p += m_muProb[n]; // needs to be renormalised
    }
  } else {
    for (int n=0; n<m_nBelt; n++) {
      m_muProb[n] =  calcProb(n, s);
      m_lhRatio[n]  = m_muProb[n]/m_bestMuProb[n];
      norm_p += m_muProb[n]; // needs to be renormalised
    }
  }
}

double Pole::calcLimit(double s) {
  int k,i;
  //
  double norm_p = 0;
  m_sumProb = 0;
  //
  //  std::cout << "calcLimit for " << s << std::endl;
  if (m_useNLR) { // use method by Gary Hill
    double pbf;
    for (int n=0; n<m_nBelt; n++) {
      m_muProb[n] =  calcProb(n, s);
      if (n>m_bkgMeas) {
	pbf = static_cast<double>(n);
      } else {
	pbf = 0;
      }
      pbf = m_poisson.getVal(n,pbf);
      m_lhRatio[n]  = m_muProb[n]/pbf;
      norm_p += m_muProb[n]; // check norm
    }
  } else {
    for (int n=0; n<m_nBelt; n++) {
      m_muProb[n] =  calcProb(n, s);
      m_lhRatio[n]  = m_muProb[n]/m_bestMuProb[n];
      norm_p += m_muProb[n];
    }
  }
  if (norm_p>m_maxNorm) m_maxNorm=norm_p;
  //
  k = m_nObserved;
  if (k>=m_nBelt) {
    k=m_nBelt; // WARNING::
    std::cout << "WARNING:: n_observed is larger than the maximum n used for R(n,s)!!" << std::endl;
    std::cout << "          -> increase nbelt such that it is more than n_obs = " << m_nObserved << std::endl;
  }
  // Calculate the probability for all n and the given s.
  // The Feldman-Cousins method dictates that for each n a
  // likelihood ratio (R) is calculated. The n's are ranked according
  // to this ratio. Values of n are included starting with that giving
  // the highest R and continuing with decreasing R until the total probability
  // matches the searched CL.
  // Below, the loop sums the probabilities for a given s and for all n with R>R0.
  // R0 is the likelihood ratio for n_observed.
  i=0;
  bool done=false;
  while (!done) {
    //  for(i=0;i<m_nBelt;i++) {
    //    m_muProb[i] = m_muProb[i]/norm_p; DO NOT NORMALISE
    //    for(k=0;k<m_nBelt;k++) {
    if(i != k) { 
      if(m_lhRatio[i] > m_lhRatio[k])  {
	m_sumProb  +=  m_muProb[i];
	//	std::cout << "s= " << s << "   i:k " << i << ":" << k << "    RL(i:k) = " << m_lhRatio[i] << ":" << m_lhRatio[k]
	//		  << "   prob = " << m_sumProb << std::endl;
      }
      //    }
    }
    i++;
    done = ((i==m_nBelt) || m_sumProb>m_cl);
  }
  //
  // Check if limit is reached.
  // For a given n_observed, we should have
  //   Sum(p) > cl for s < s_low
  //   Sum(p) < cl for s_low < s < s_upp
  //   Sum(p) > cl for s_upp < s
  //
  if (m_sumProb<m_cl) {
    if (m_foundLower) {
      m_upperLimit = s;
      m_upperLimitNorm = norm_p;
      //      m_foundUpper = true;
    } else {
      m_lowerLimit = s;
      m_lowerLimitNorm = norm_p;
      m_foundLower = true;
      m_foundUpper = false;
    }
  } else {
    if (m_foundLower) {
      m_foundUpper = true;
    }
  }
  return norm_p;
}

bool Pole::limitsOK() {
  bool rval=false;
  if (m_foundLower && m_foundUpper) {
    rval = (normOK(m_lowerLimitNorm) && normOK(m_upperLimitNorm));
  }
  return rval;
}

//*********************************************************************//
//*********************************************************************//
//*********************************************************************//

void Pole::calcConstruct(double s) {
  int i;
  //
  double norm_p = 0;
  double p;
  std::vector<double> muProb;
  std::vector<double> lhRatio;
  //
  //  std::cout << "calcLimit for " << s << std::endl;
  if (m_useNLR) { // use method by Gary Hill
    double pbf;
    for (int n=0; n<m_nBelt; n++) {
      p =  calcProb(n, s);
      if (n>m_bkgMeas) {
	pbf = static_cast<double>(n);
      } else {
	pbf = 0;
      }
      pbf = m_poisson.getVal(n,pbf);
      lhRatio.push_back(p/pbf);
      muProb.push_back(p);
      norm_p += p; // needs to be renormalised

    }
  } else {
    for (int n=0; n<m_nBelt; n++) {
      p =  calcProb(n, s);
      lhRatio.push_back(p/m_bestMuProb[n]);
      muProb.push_back(p);
      norm_p += p; // needs to be renormalised
    }
  }
  for (i=0; i<m_nBelt; i++) {
    muProb[i] = muProb[i]/norm_p;
  }
  //
  for (i=0; i<m_nBelt; i++) {
    std::cout << "CONSTRUCT: " << s << "\t" << i << "\t" << lhRatio[i] << "\t" << muProb[i] << std::endl;
  }
}

// NOTE: Simple sorting - should be put somewhere else...

void sort_index(std::vector<double> & input, std::vector<int> & index, bool reverse=false) {
  int ndata = input.size();
  if (ndata<=0) return;
  //
  int i;
  std::list< std::pair<double,int> > dl;
  //
  for (i=0; i<ndata; i++) {
    dl.push_back(std::pair<double,int>(input[i],i));
  }
  dl.sort();
  //
  if (!reverse) {
    std::list< std::pair<double,int> >::iterator dliter;
    for (dliter = dl.begin(); dliter != dl.end(); dliter++) {
      index.push_back(dliter->second);
    }
  } else {
    std::list< std::pair<double,int> >::reverse_iterator dliter;
    for (dliter = dl.rbegin(); dliter != dl.rend(); dliter++) {
      index.push_back(dliter->second);
    }
  }
}

double Pole::calcBelt(double s, int & n1, int & n2) {
  int i;
  //
  double norm_p = 0;
  double sumProb = 0;
  double p;
  std::vector<double> muProb;
  std::vector<double> lhRatio;
  std::vector<int> index;
  //
  //  std::cout << "calcLimit for " << s << std::endl;
  if (m_useNLR) { // use method by Gary Hill
    double pbf;
    for (int n=0; n<m_nBelt; n++) {
      p =  calcProb(n, s);
      if (n>m_bkgMeas) {
	pbf = static_cast<double>(n);
      } else {
	pbf = 0;
      }
      pbf = m_poisson.getVal(n,pbf);
      lhRatio.push_back(p/pbf);
      muProb.push_back(p);
      norm_p += p; // needs to be renormalised

    }
  } else {
    for (int n=0; n<m_nBelt; n++) {
      p =  calcProb(n, s);
      lhRatio.push_back(p/m_bestMuProb[n]);
      muProb.push_back(p);
      norm_p += p; // needs to be renormalised
    }
  }
  sort_index(lhRatio,index,true); // reverse sort
  //
  // Calculate the probability for all n and the given s.
  // The Feldman-Cousins method dictates that for each n a
  // likelihood ratio (R) is calculated. The n's are ranked according
  // to this ratio. Values of n are included starting with that giving
  // the highest R and continuing with decreasing R until the total probability
  // matches the searched CL.
  // Below, the loop sums the probabilities for a given s and for all n with R>R0.
  // R0 is the likelihood ratio for n_observed.
  for (i=0; i<m_nBelt; i++) {
    muProb[i] = muProb[i]/norm_p;
  }
  i=0;
  bool done=false;
  int nmin=-1;
  int nmax=-1;
  int n;

  while (!done) {
    n = index[i];
    p = muProb[i];
    sumProb +=p;
    if ((n<nmin)||(nmin<0)) nmin=n;
    if ((n>nmax)||(nmax<0)) nmax=n;
    //
    i++;
    done = ((i==m_nBelt) || sumProb>m_cl);
  }
  if ((nmin<0) || ((nmin==0)&&(nmax==0))) {
    nmin=0;
    nmax=1;
    sumProb=1.0;
  }
  n1 = nmin;
  n2 = nmax;
  std::cout << "CONFBELT: " << s << "\t" << n1 << "\t" << n2 << "\t" << sumProb << "\t" << lhRatio[n1] << "\t" << lhRatio[n2] << "\t" << norm_p << std::endl;
  return sumProb;
}

//*********************************************************************//
//*********************************************************************//
//*********************************************************************//

void Pole::findConstruct() {
  double mu_test;
  int i = 0;
  bool done = (i==m_hypTest.n());
  //
  while (!done) {
    mu_test = m_hypTest.min() + i*m_hypTest.step();
    calcConstruct(mu_test);
    i++;
    done = (i==m_hypTest.n()); // must loop over all hypothesis
  }
}

void Pole::findBelt() {
  double mu_test;
  int i = 0;
  bool done = (i==m_hypTest.n());
  //
  int n1,n2;
  while (!done) {
    mu_test = m_hypTest.min() + i*m_hypTest.step();
    calcBelt(mu_test,n1,n2);
    i++;
    done = (i==m_hypTest.n()); // must loop over all hypothesis
  }
}

bool Pole::findLimits() {
  m_maxNorm = -1.0;
  m_foundLower = false;
  m_foundUpper = false;
  m_lowerLimit = 0;
  m_upperLimit = 0;
  //
  if (m_nObserved>=m_nBelt) return false;
  double mu_test;
  int i = 0;
  bool done = (i==m_hypTest.n());
  bool nuppOK = false;
  int nupp=0;
  //
  //
  double p;
  //  int n1,n2;

  while (!done) {
    mu_test = m_hypTest.min() + i*m_hypTest.step();
    p=calcLimit(mu_test);
    //    calcBelt(mu_test,n1,n2);
    if (m_verbose>2) std::cout << "findLimits(): " << mu_test << " norm = " << p << " sump = " << m_sumProb << std::endl;
    i++;
    if (m_foundUpper) { // sample a few afterwards just to make sure we get the last one
      nupp++;
      nuppOK = ((nupp>=m_nUppLim) || (m_nUppLim<1));
    }
    done = ((i==m_hypTest.n()) || // Done if last hypothesis reached
	    (nuppOK)           || //
	    (!normOK(p)));        // normalisation not OK
  }
  if (m_verbose>1) {
    if (limitsOK()) {
      std::cout << "LIMITS(N,e,b,l,u): ";
      coutFixed(4,m_nObserved);  std::cout << "\t";
      coutFixed(4,m_effMeas);    std::cout << "\t";
      coutFixed(4,m_bkgMeas);    std::cout << "\t";
      coutFixed(4,m_lowerLimit); std::cout << "\t";
      coutFixed(4,m_upperLimit); std::cout << std::endl;
    } else {
      std::cout << "Limits NOT OK!" << std::endl;
    }
  }
  return limitsOK();
}

bool Pole::findCoverageLimits() {
  //
  // Same as findLimits() but it will stop the scan as soon it is clear
  // that the true s (m_sTrueMean) is inside or outside the range.
  //
  m_maxNorm = -1.0;
  m_foundLower = false;
  m_foundUpper = false;
  m_lowerLimit = 0;
  m_upperLimit = 0;
  //
  if (m_nObserved>=m_nBelt) return false;
  //
  double mu_test;
  int i = 0;
  bool done = (i==m_hypTest.n());
  bool decided = false;
  bool nuppOK = false;
  int  nupp=0;
  //
  double p;
  while (!done) {
    mu_test = m_hypTest.min() + i*m_hypTest.step();
    p=calcLimit(mu_test);
    if (m_verbose>2) std::cout << "findCoverageLimits():"
			       << " s_test = " << mu_test
			       << " norm = " << p
			       << " sump = " << m_sumProb << std::endl;
    i++;
    if ((!m_foundLower) && (mu_test>m_sTrue)) { // for sure outside
      decided = true;
      m_lowerLimit = mu_test;     // create a fake set of limits
      m_upperLimit = mu_test+0.1;
      m_lowerLimitNorm = p; // not really needed
      m_upperLimitNorm = p; // not exactly true - only the lower limit norm is relevant
      m_foundLower = true;
      m_foundUpper = true;
    } else if (m_foundUpper) {
      if (m_upperLimit>m_sTrue) { // for sure inside
	decided = limitsOK(); // if norm is not OK then this is not sure
      } else { // m_sTrue is above - wait a bit before deciding
	nupp++;
	nuppOK = ((nupp>=m_nUppLim) || (m_nUppLim<1));
	if (nuppOK) decided = limitsOK();
      }
    }
    done = ((decided) ||
	    (i==m_hypTest.n()));
  }
  if (m_verbose>1) {
    if (limitsOK()) {
      std::cout << "COVLIMITS(N,s,e,b,l,u): ";
      coutFixed(4,m_nObserved);  std::cout << "\t";
      coutFixed(4,m_sTrue);      std::cout << "\t";
      coutFixed(4,m_effMeas);    std::cout << "\t";
      coutFixed(4,m_bkgMeas);    std::cout << "\t";
      coutFixed(4,m_lowerLimit); std::cout << "\t";
      coutFixed(4,m_upperLimit); std::cout << std::endl;;
    } else {
      std::cout << "Limits NOT OK!" << std::endl;
    }
  }
  return decided;
}

bool Pole::analyseExperiment() {
  bool rval=false;
  if (m_verbose>0) std::cout << "Initialise arrays" << std::endl;
  initIntArrays();
  initBeltArrays();
  if (m_verbose>0) std::cout << "Constructing integral" << std::endl;
  initIntegral();  // generates predefined parts of double integral ( eq.7)
  if (!m_useNLR) {
    if (m_verbose>0) std::cout << "Finding s_best" << std::endl;
    findAllBestMu(); // loops
  }
  if (m_verbose>0) std::cout << "Calculating limit" << std::endl;
  if (m_coverage) {
    rval=findCoverageLimits();
  } else {
    rval=findLimits();
  }
  return rval;
}

void Pole::printLimit(bool doTitle) {
  if (doTitle) {
    std::cout << " Nobs  \t  Eff   \t Bkg" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
  }
  coutFixed(4,m_nObserved); std::cout << "\t";
  coutFixed(6,m_effMeas); std::cout << "\t";
  coutFixed(6,m_effSigma); std::cout << "\t";
  coutFixed(6,m_bkgMeas); std::cout << "\t";
  coutFixed(6,m_bkgSigma); std::cout << "\t";
  std::cout << "[ ";
  coutFixed(2,m_lowerLimit); std::cout << ", ";
  coutFixed(2,m_upperLimit); std::cout << " ]";
  std::cout << std::endl;
}

void Pole::printSetup() {
  std::cout << "\n";
  std::cout << "================ P O L E ==================\n";
  std::cout << " Confidence level   : " << m_cl << std::endl;
  std::cout << " N observed         : " << m_nObserved << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Coverage friendly  : " << yesNo(m_coverage) << std::endl;
  std::cout << " True signal        : " << m_sTrue << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Efficiency meas    : " << m_effMeas << std::endl;
  std::cout << " Efficiency sigma   : " << m_effSigma << std::endl;
  std::cout << " Efficiency dist    : " << distTypeStr(m_effDist) << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Background meas    : " << m_bkgMeas << std::endl;
  std::cout << " Background sigma   : " << m_bkgSigma << std::endl;
  std::cout << " Background dist    : " << distTypeStr(m_bkgDist) << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Bkg-Eff correlation: " << m_beCorr << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Int. eff. min      : " << m_effRangeInt.min() << std::endl;
  std::cout << " Int. eff. max      : " << m_effRangeInt.max() << std::endl;
  std::cout << " Int. eff. N pts    : " << m_effRangeInt.n() << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Int. bkg. min      : " << m_bkgRangeInt.min() << std::endl;
  std::cout << " Int. bkg. max      : " << m_bkgRangeInt.max() << std::endl;
  std::cout << " Int. bkg. N pts    : " << m_bkgRangeInt.n() << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Test hyp. min      : " << m_hypTest.min() << std::endl;
  std::cout << " Test hyp. max      : " << m_hypTest.max() << std::endl;
  std::cout << " Test hyp. N pts    : " << m_hypTest.n() << std::endl;
  std::cout << "----------------------------------------------\n";
  if (m_suggestBelt) {
    std::cout << " Belt N max         : variable" << std::endl;
  } else {
    std::cout << " Belt N max         : " << m_nBelt << std::endl;
  }
  std::cout << " Step mu_best       : " << m_dmus << std::endl;
  std::cout << "----------------------------------------------\n";
  std::cout << " Use NLR            : " << yesNo(m_useNLR) << std::endl;
  std::cout << "==============================================\n";
  //
}