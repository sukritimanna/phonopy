#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "phonoc_array.h"
#include "imag_self_energy.h"

#define THZTOEVPARKB 47.992398658977166
#define INVSQRT2PI 0.3989422804014327

static double get_imag_self_energy_at_band(const int num_band,
					   const double *fc3_normal_sqared,
					   const double fpoint,
					   const double *freqs0,
					   const double *freqs1,
					   const double sigma,
					   const double temperature);
static double get_imag_self_energy_at_band_0K(const int num_band,
					      const double *fc3_normal_sqared,
					      const double fpoint,
					      const double *freqs0,
					      const double *freqs1,
					      const double sigma);
static double gaussian(const double x, const double sigma);
static double occupation(const double x, const double t);
    
/* imag_self_energy[num_band0] */
/* fc3_normal_sqared[num_triplets, num_band0, num_band, num_band] */
void get_imag_self_energy(double *imag_self_energy,
			  const Darray *fc3_normal_sqared,
			  const double fpoint,
			  const double *frequencies,
			  const int *grid_point_triplets,
			  const int *triplet_weights,
			  const double sigma,
			  const double temperature,
			  const double unit_conversion_factor)
{
  int i, j, num_triplets, num_band0, num_band, gp1, gp2;
  double sum_g;

  num_triplets = fc3_normal_sqared->dims[0];
  num_band0 = fc3_normal_sqared->dims[1];
  num_band = fc3_normal_sqared->dims[2];

  for (i = 0; i < num_band0; i++) {
    sum_g = 0;
#pragma omp parallel for private(gp1, gp2) reduction(+:sum_g)
    for (j = 0; j < num_triplets; j++) {
      gp1 = grid_point_triplets[j * 3 + 1];
      gp2 = grid_point_triplets[j * 3 + 2];
      if (temperature > 0) {
	sum_g +=
	  get_imag_self_energy_at_band(num_band,
				       fc3_normal_sqared->data +
				       j * num_band0 * num_band * num_band +
				       i * num_band * num_band,
				       fpoint,
				       frequencies + gp1 * num_band,
				       frequencies + gp2 * num_band,
				       sigma,
				       temperature) *
	  triplet_weights[j] * unit_conversion_factor;
      } else {
	sum_g +=
	  get_imag_self_energy_at_band_0K(num_band,
					  fc3_normal_sqared->data +
					  j * num_band0 * num_band * num_band +
					  i * num_band * num_band,
					  fpoint,
					  frequencies + gp1 * num_band,
					  frequencies + gp2 * num_band,
					  sigma) *
	  triplet_weights[j] * unit_conversion_factor;
      }
    }
    imag_self_energy[i] = sum_g;
  }
}

static double get_imag_self_energy_at_band(const int num_band,
					   const double *fc3_normal_sqared,
					   const double fpoint,
					   const double *freqs0,
					   const double *freqs1,
					   const double sigma,
					   const double temperature)
{
  int i, j;
  double n2, n3, g1, g2, g3, sum_g;

  sum_g = 0;
  for (i = 0; i < num_band; i++) {
    if (freqs0[i] > 0) {
      n2 = occupation(freqs0[i], temperature);
      for (j = 0; j < num_band; j++) {
	if (freqs1[j] > 0) {
	  n3 = occupation(freqs1[j], temperature);
	  g1 = gaussian(fpoint - freqs0[i] - freqs1[j], sigma);
	  g2 = gaussian(fpoint + freqs0[i] - freqs1[j], sigma);
	  g3 = gaussian(fpoint - freqs0[i] + freqs1[j], sigma);
	  sum_g += ((n2 + n3 + 1) * g1 + (n2 - n3) * (g2 - g3)) *
	    fc3_normal_sqared[i * num_band + j];
	}
      }
    }
  }
  return sum_g;
}

static double get_imag_self_energy_at_band_0K(const int num_band,
					      const double *fc3_normal_sqared,
					      const double fpoint,
					      const double *freqs0,
					      const double *freqs1,
					      const double sigma)
{
  int i, j;
  double g1, sum_g;

  sum_g = 0;
  for (i = 0; i < num_band; i++) {
    if (freqs0[i] > 0) {
      for (j = 0; j < num_band; j++) {
	if (freqs1[j] > 0) {
	  g1 = gaussian(fpoint - freqs0[i] - freqs1[j], sigma);
	  sum_g += g1 * fc3_normal_sqared[i * num_band + j];
	}
      }
    }
  }
  return sum_g;
}

static double gaussian(const double x, const double sigma)
{
  return INVSQRT2PI / sigma * exp(-x * x / 2 / sigma / sigma);
}  

static double occupation(const double x, const double t)
{
  return 1.0 / (exp(THZTOEVPARKB * x / t) - 1);
}
