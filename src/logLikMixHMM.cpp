#include "seqHMM.h"

// [[Rcpp::export]]

NumericVector logLikMixHMM(const arma::mat& transition, NumericVector emissionArray,
  const arma::vec& init, IntegerVector obsArray, const arma::mat& coef, const arma::mat& X,
  const arma::ivec& numberOfStates, int threads) {
  
  IntegerVector eDims = emissionArray.attr("dim"); //m,p,r
  IntegerVector oDims = obsArray.attr("dim"); //k,n,r
  
  arma::cube emission(emissionArray.begin(), eDims[0], eDims[1], eDims[2], false);
  arma::icube obs(obsArray.begin(), oDims[0], oDims[1], oDims[2], false);
  
  unsigned int q = coef.n_rows;
  arma::mat weights = exp(X * coef).t();
  if (!weights.is_finite()) {
    warning(
      "Coefficients of covariates resulted non-finite cluster probabilities. Returning -Inf.");
    return wrap(-arma::math::inf());
    
  }
  weights.each_row() /= sum(weights, 0);
  
  NumericVector ll(obs.n_slices);
  
#pragma omp parallel for if(obs.n_slices >= threads) schedule(static) num_threads(threads) \
  default(none) shared(ll, obs, weights, init, emission, transition, numberOfStates)
    for (int k = 0; k < obs.n_slices; k++) {
      arma::vec alpha = init % reparma(weights.col(k), numberOfStates);
      
      for (unsigned int r = 0; r < obs.n_rows; r++) {
        alpha %= emission.slice(r).col(obs(r, 0, k));
      }
      
      double tmp = sum(alpha);
      ll(k) = log(tmp);
      alpha /= tmp;
      
      for (unsigned int t = 1; t < obs.n_cols; t++) {
        alpha = transition.t() * alpha;
        for (unsigned int r = 0; r < obs.n_rows; r++) {
          alpha %= emission.slice(r).col(obs(r, t, k));
        }
        
        tmp = sum(alpha);
        ll(k) += log(tmp);
        alpha /= tmp;
      }
    }
    return ll;
}
