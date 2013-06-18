/*-
* Copyright (c) 2012 Hamrouni Ghassen.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

/*
 *
 * Recommender system using matrix factorization (MF)
 * Computing the product recommendation using latent factor models
 *
 */

#include "learned_factors.h"
#include "model_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include "utils.h"
#include "box_muller.h"
struct learned_factors*
init_learned_factors (struct model_parameters params)
{
	struct learned_factors* lfactors =
	    malloc (sizeof (struct learned_factors) );
	box_muller_generator_t* gen = init_box_muller_generator(1,0.1,0);

	if (!lfactors)
	{
		return NULL;
	}

	lfactors->x=NULL;
	lfactors->y = NULL;
	lfactors->ratings_average = 0;
	

	lfactors->item_factor_vectors = generate_random_matrix (params.items_number, params.dimensionality, gen);
	if(params.algoithm_type == NEIGHBOURS_MF)
	{
	lfactors->y = generate_random_matrix (params.items_number, params.dimensionality, gen);
	lfactors->x = generate_random_matrix (params.items_number, params.items_number, gen);
	}
	lfactors->user_factor_vectors = generate_random_matrix (params.users_number, params.dimensionality, gen);
	lfactors->user_bias = malloc (sizeof (double) *params.users_number);
	lfactors->item_bias = malloc (sizeof (double) *params.items_number);
	memset (lfactors->user_bias, 0.0, params.users_number*sizeof (double) );
	memset (lfactors->item_bias, 0.0, params.items_number*sizeof (double) );
	lfactors->R = NULL;
	lfactors->R_K = NULL;

	if (!lfactors->item_factor_vectors ||
	        !lfactors->user_factor_vectors ||
	        !lfactors->item_bias ||
	        !lfactors->user_bias)
	{
		return lfactors;
	}

	return lfactors;
}

/*
 * free_learned_factors:  delete the learned factors from memory
 */
void
free_learned_factors (learned_factors_t* lfactors)
{
	size_t i;

	if (!lfactors)
	{
		return;
	}

	for (i = 0; i < lfactors->items_number; i++)
	{
		free (lfactors->item_factor_vectors[i]);
		if(lfactors->x)
			free (lfactors->x[i]);
		if(lfactors->y)
			free (lfactors->y[i]);

	}
	free (lfactors->item_factor_vectors);
	if(lfactors->x)
		free (lfactors->x);
	if(lfactors->y)
		free (lfactors->y);

	for (i = 0; i < lfactors->users_number; i++)
	{
		free (lfactors->user_factor_vectors[i]);
	}
	free (lfactors->user_factor_vectors);
	free (lfactors->item_bias);
	free (lfactors->user_bias);
	if (lfactors->R)
	{
		for (i = 0; i < lfactors->users_number; i++)
		{
			free (lfactors->R[i].ratings_order);
		}
		free (lfactors->R);
	}
	if (lfactors->R)
	{
		for (i = 0; i < lfactors->items_number; i++)
		{
			free (lfactors->R_K[i].ratings_order);
		}
	}
	free (lfactors->R_K);
	free (lfactors);
}




struct learned_factors*
copy_learned_factors (struct learned_factors* factors)
{
	struct learned_factors* factors_copy =
	    malloc (sizeof (struct learned_factors) );
	size_t i;
	if (!factors)
	{
		return NULL;
	}
	factors_copy->users_number = factors->users_number;
	factors_copy->items_number = factors->items_number;
	factors_copy->dimensionality = factors->dimensionality;
	factors_copy->ratings_average = factors->ratings_average;
	factors_copy->x =NULL;
	factors_copy->y =NULL;
	factors_copy->user_factor_vectors = malloc (sizeof (double*) *factors->users_number);
	for (i = 0; i < factors->users_number; i++)
	{
		factors_copy->user_factor_vectors[i] = malloc (sizeof (double) * factors->dimensionality);
		memcpy (factors_copy->user_factor_vectors[i], factors->user_factor_vectors[i], factors->dimensionality * sizeof (double) );
	}
	if (factors->x)
	{
		factors_copy->x = malloc (sizeof (double*) *factors->items_number);
	}
	if (factors->y)
	{
		factors_copy->y = malloc (sizeof (double*) *factors->items_number);
	}
	factors_copy->item_factor_vectors = malloc (sizeof (double*) *factors->items_number);
	for (i = 0; i < factors->items_number; i++)
	{
		if (factors->x)
		{
			factors_copy->x[i] = malloc (sizeof (double) * factors->items_number);
			memcpy (factors_copy->x[i], factors->x[i], factors->items_number * sizeof (double) );
		}
		if (factors->y)
		{
			factors_copy->y[i] = malloc (sizeof (double) * factors->dimensionality);
			memcpy (factors_copy->y[i], factors->y[i], factors->dimensionality * sizeof (double) );
		}
		factors_copy->item_factor_vectors[i] = malloc (sizeof (double) * factors->dimensionality);
		memcpy (factors_copy->item_factor_vectors[i], factors->item_factor_vectors[i], factors->dimensionality * sizeof (double) );
	}

	
	factors_copy->user_bias = malloc (sizeof (double) *factors->users_number);
	factors_copy->item_bias = malloc (sizeof (double) *factors->items_number);
	memcpy (factors_copy->user_bias, factors->user_bias, factors->users_number * sizeof (double) );
	memcpy (factors_copy->item_bias, factors->item_bias, factors->items_number * sizeof (double) );
	factors_copy->R = NULL;
	factors_copy->R_K = NULL;

	if (!factors_copy->item_factor_vectors ||
	        !factors_copy->user_factor_vectors ||
	        !factors_copy->item_bias ||
	        !factors_copy->user_bias)
	{
		return NULL;
	}

	return factors_copy;
}
