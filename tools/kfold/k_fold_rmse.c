#include"k_fold_rmse.h"
#include "data_set.h"
#include "learned_factors.h"
#include<math.h>
#include<stdlib.h>
#include<stdio.h>
#include <stdarg.h>
#include  "rlog.h"
#include "../serialization/serialize_training_set.h"
#include "../serialization/redis_parameters.h"
#include "nearest_neighbors.h"
#include "neighborMF.h"
#include"sparse_matrix_hash.h"
#include "items_rated_by_user.h"
#include "rating_estimator.h"
#include "sparse_matrix.h"
#include "social_reg.h"
#include "model_parameters.h"

#define ABS(a) ((a)<0 ? -(a) : (a))
error_t RMSE_mean (k_fold_parameters_t k_fold_params)
{
	
	int index;
	error_t err, err_sum;
	learned_factors_t *learned;
	training_set_t* tset = NULL;
	training_set_t* validation_set = NULL;
	k_fold_params.model.parameters = k_fold_params.params;
	err_sum.MAE = 0;
	err_sum.RMSE = 0;
	if(k_fold_params.model.parameters.algoithm_type == SOCIAL)
	{
		k_fold_params.model.social_matrix = extract_social_realtions(k_fold_params.social_relations_file_path,
										k_fold_params.params.users_number,k_fold_params.social_relations_number);
	}else
	{
		k_fold_params.model.social_matrix = NULL;
	}
	for (index = 0; index < k_fold_params.K; index++)
	{
		extract_data (k_fold_params, &tset, &validation_set, index);
		
		compile_training_set (tset);
		learned = learn(tset,k_fold_params.model);
		err = RMSE (learned,validation_set,tset,k_fold_params);
		err_sum.RMSE += err.RMSE;
		err_sum.MAE += err.MAE;
		//free_learned_factors(learned);
		free_training_set (tset);
		free_training_set (validation_set);
	}
	if(k_fold_params.model.social_matrix)
	{
		free(k_fold_params.model.social_matrix);
	}
	err_sum.RMSE /= k_fold_params.K;
	err_sum.MAE /= k_fold_params.K;
	return (err_sum);
}



error_t RMSE (learned_factors_t* learned, training_set_t * _validation_set,
             training_set_t * tset,k_fold_parameters_t k_fold_params)
{
	unsigned int i;
	error_t e;
	double a;
	size_t s;
	size_t u;
	rating_estimator_parameters_t* estim_param=malloc(sizeof(rating_estimator_parameters_t));
	estim_param->lfactors=learned;
	estim_param->tset=tset;
	e.RMSE=0;
	e.MAE=0;
	for (s = 0; s < _validation_set->training_set_size; s++)
	{
		i = _validation_set->ratings->entries[s].row_i;
		u = _validation_set->ratings->entries[s].column_j;
		estim_param->item_index = i;
		estim_param->user_index = u;
		a = estimate_rating_from_factors(estim_param,k_fold_params.model);
		e.RMSE += pow (_validation_set->ratings->entries[s].value -
			a , 2) / ((double)_validation_set->training_set_size);	
		e.MAE += ABS(_validation_set->ratings->entries[s].value - a)/ ((double)_validation_set->training_set_size);
	}
	e.RMSE = sqrtf(e.RMSE);
	RLog ("RMSE = %f \n", e.RMSE );
	RLog ("MAE = %f \n", e.MAE );
	free(estim_param);
	return (e );
}
