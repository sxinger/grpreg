#include <math.h>
#include <string.h>
#include "Rinternals.h"
#include "R_ext/Rdynload.h"
#include <R.h>
#include <R_ext/Applic.h>
int checkConvergence(double *beta, double *beta_old, double eps, int l, int J);
double crossprod(double *x, double *y, int n, int j);
double sum(double *x, int n);
double norm(double *x, int p);
double S(double z, double l);
double F(double z, double l1, double l2, double gamma);
double Fs(double z, double l1, double l2, double gamma);
double MCP(double theta, double l, double a);
double dMCP(double theta, double l, double a);
void cleanupB(double *a, double *r, int *e, double *eta);

// Group descent update -- binomial
void gd_binomial(double *b, double *x, double *r, double *eta, int g, int *K1, int n, int l, int p, char *penalty, double lam1, double lam2, double gamma, double *df, double *a) {

  // Calculate z
  int K = K1[g+1] - K1[g];
  double *z = Calloc(K, double);
  for (int j=K1[g]; j<K1[g+1]; j++) z[j-K1[g]] = crossprod(x, r, n, j)/n + a[j];
  double z_norm = norm(z,K);
  double v = 0.25;

  // Update b
  double len;
  if (strcmp(penalty, "grLasso")==0) len = S(v * z_norm, lam1) / (v * (1 + lam2));
  if (strcmp(penalty, "grMCP")==0) len = F(v * z_norm, lam1, lam2, gamma) / v;
  if (strcmp(penalty, "grSCAD")==0) len = Fs(v * z_norm, lam1, lam2, gamma) / v;
  if (len != 0 | a[K1[g]] != 0) {
    // If necessary, update b and r
    for (int j=K1[g]; j<K1[g+1]; j++) {
      b[l*p+j] = len * z[j-K1[g]] / z_norm;
      double shift = b[l*p+j]-a[j];
      for (int i=0; i<n; i++) {
	double si = shift*x[j*n+i];
	r[i] -= si;
	eta[i] += si;
      }
    }
  }

  // Update df
  if (len > 0) df[l] = df[l] + K * len / z_norm;
  Free(z);
}

void grPathFit_binomial(double *b0, double *b, int *iter, double *df, double *Dev, double *x, double *y, int *n_, int *p_, char **penalty_, int *J_, int *K1, int *K0_, double *lam1, double *lam2, int *L_, double *eps_, int *max_iter_, double *gamma_, double *group_multiplier, int *dfmax_, int *gmax_, int *warn_, int *user_)
{
  int n=n_[0]; int p=p_[0]; char *penalty=penalty_[0]; int J=J_[0]; int K0=K0_[0]; int L=L_[0]; int max_iter=max_iter_[0]; double eps=eps_[0]; double gamma=gamma_[0]; int dfmax=dfmax_[0]; int gmax=gmax_[0]; int warn=warn_[0]; int user=user_[0];
  double a0=0;
  double *r = Calloc(n, double);
  double *eta = Calloc(n, double);
  double *a = Calloc(p, double);
  for (int j=0; j<p; j++) a[j] = 0;
  int *e = Calloc(J, int);
  for (int g=0; g<J; g++) e[g] = 0;
  int lstart, violations;

  // Initialization
  double ybar = sum(y, n)/n;
  a0 = b0[0] = log(ybar/(1-ybar));
  double nullDev = 0;
  for (int i=0; i<n; i++) nullDev += - y[i]*log(ybar) - (1-y[i])*log(1-ybar);
  for (int i=0; i<n; i++) eta[i] = a0;

  // If lam[0]=lam_max, skip lam[0] -- closed form sol'n available
  if (user) {
    lstart = 0;
  } else {
    lstart = 1;
    Dev[0] = nullDev;
  }

  // Path
  double pi;
  for (int l=lstart; l<L; l++) {
    if (l != 0) {
      a0 = b0[l-1];
      for (int j=0; j<p; j++) a[j] = b[(l-1)*p+j];

      // Check dfmax, gmax
      int ng = 0;
      int nv = 0;
      for (int g=0; g<J; g++) {
	if (a[K1[g]] != 0) {
	  ng++;
	  nv = nv + (K1[g+1]-K1[g]);
	}
      }
      if (ng > gmax | nv > dfmax) {
	for (int ll=l; ll<L; ll++) iter[ll] = NA_INTEGER;
	cleanupB(a, r, e, eta); 
	return;
      }
    }

    while (iter[l] < max_iter) {
      while (iter[l] < max_iter) {
	int converged = 0;
	iter[l]++;

	// Approximate L
	Dev[l] = 0;
	for (int i=0; i<n; i++) {
	  if (eta[i] > 10) {
	    pi = 1;
	  } else if (eta[i] < -10) {
	    pi = 0;
	  } else {
	    pi = exp(eta[i])/(1+exp(eta[i]));
	  }
	  r[i] = (y[i] - pi) / 0.25;
	  Dev[l] += - y[i]*log(pi) - (1-y[i])*log(1-pi);
	}

	// Check for saturation
	if (Dev[l]/nullDev < .01) {
	  if (warn) warning("Model saturated; exiting...");
	  for (int ll=l; ll<L; ll++) iter[ll] = NA_INTEGER;
	  cleanupB(a, r, e, eta);
	  return;
	}

	// Update intercept
	double shift = sum(r, n)/n;
	b0[l] = shift + a0;
	for (int i=0; i<n; i++) {
	  r[i] -= shift;
	  eta[i] += shift;
	}
	df[l] = 1;

	// Update unpenalized covariates
	for (int j=0; j<K0; j++) {
	  shift = crossprod(x, r, n, j)/n;
	  b[l*p+j] = shift + a[j];
	  for (int i=0; i<n; i++) {
	    double si = shift * x[n*j+i];
	    r[i] -= si;
	    eta[i] += si;
	  }
	  df[l] = df[l] + 1;
	}

	// Update penalized groups
	for (int g=0; g<J; g++) {
	  if (e[g]) gd_binomial(b, x, r, eta, g, K1, n, l, p, penalty, lam1[l]*group_multiplier[g], lam2[l], gamma, df, a);
	}

	// Check convergence
	if (checkConvergence(b, a, eps, l, p)) {
	  converged  = 1;
	  break;
	}
	a0 = b0[l];
	for (int j=0; j<p; j++) a[j] = b[l*p+j];
      }

      // Scan for violations
      violations = 0;
      for (int g=0; g<J; g++) {
	if (e[g]==0) {
	  gd_binomial(b, x, r, eta, g, K1, n, l, p, penalty, lam1[l]*group_multiplier[g], lam2[l], gamma, df, a);
	  if (b[l*p+K1[g]] != 0) {
	    e[g] = 1;
	    violations++;
	  }
	}
      }

      if (violations==0) break;
      a0 = b0[l];
      for (int j=0; j<p; j++) a[j] = b[l*p+j];
    }
  }
  cleanupB(a, r, e, eta);
  return;
}