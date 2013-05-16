/* boxmuller.c           Implements the Polar form of the Box-Muller
                         Transformation

                      (c) Copyright 1994, Everett F. Carter Jr.
                          Permission is granted by the author to use
			  this software for any application provided this
			  copyright notice is preserved.

*/

#include <math.h>
#include <stdlib.h>
#include "box_muller.h"

box_muller_generator_t* init_box_muller_generator(int seed,	double sd,double mean)
{
	box_muller_generator_t* gen = malloc(sizeof(box_muller_generator_t));
	gen->mean = mean;
	gen->sd = sd;
	gen->seed = seed;
	gen->use_last = 0;
	srand(seed);
	return gen;
}


double box_muller(box_muller_generator_t* gen)	/* normal random variate generator */
{				        /* mean m, standard deviation s */
	double x1, x2, w, y1;

	if (gen->use_last)		        /* use value from previous call */
	{
		y1 = gen->last_value;
		gen->use_last = 0;
	}
	else
	{
		do {
			x1 = 2.0 * rand()/RAND_MAX - 1.0;
			x2 = 2.0 * rand()/RAND_MAX - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		y1 = x1 * w;
		gen->last_value = x2 * w;
		gen->use_last = 1;
	}

	return( gen->mean + y1 * gen->sd );
}



