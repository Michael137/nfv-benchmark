#include "poisson.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

static gsl_rng* poisson_rng_g = NULL;

void poisson_init(void)
{
	const gsl_rng_type * T;

	/* create a generator chosen by the
	environment variable GSL_RNG_TYPE */

	gsl_rng_env_setup();

	T = gsl_rng_default;
	poisson_rng_g = gsl_rng_alloc (T);
}

void poisson_fini(void)
{
	gsl_rng_free (poisson_rng_g);
}

unsigned int poisson(double mu)
{
	return gsl_ran_poisson (poisson_rng_g, mu);
}
