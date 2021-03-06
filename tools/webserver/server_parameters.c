#include "server_parameters.h"
#include "matrix_factorization.h"
#include "matrix_factorization_bias.h"
#include "neighborMF.h"
#include "social_reg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
server_parameters_t* parse_arguments (int argc, char** argv)
{
	server_parameters_t* arguments_params = malloc(sizeof(server_parameters_t));
	size_t file_path_length = 0;
	int bin_width_ratio;
	if (argc < 15)
	{
		printf ("Incorrect arguments \n");
		printf ("usage: webserver 8000 100000 943 1682 u.data 30 30 0.055 0.0095 0.02 0.001 bias \n kfold -h for help \n");
		if (strcmp (argv[1], "-h") )
		{
			printf ("webserver port ratings_number users_number items_number file_path \n");
			printf ("port : server connection port\n");
			printf ("ratings_number : Number of ratings in the sample \n");
			printf ("users_number : number of users in the sample \n");
			printf ("items_number : number of items in the sample \n");
			printf ("file_path : the path of the samples file \n");
			printf ("dimentionality : dimentionality \n");
			printf ("iteration_number : the desired iteration's number \n");
			printf ("lambda : lambda \n");
			printf ("step : step \n");
			printf ("lambda_bias : lambda_bias \n");
			printf ("step_bias : step_bias \n");
			printf ("projection family number :  \n");
			printf ("bin width ratio \n");
			printf ("type : basic or bias \n");
		}
		system ("pause");
		return NULL;
	}

	arguments_params->port = atoi (argv[1]);
	arguments_params->model.parameters.training_set_size = atoi (argv[2]);

	if (arguments_params->port < 2)
	{
		printf ("port must be greater or equal than 2 \n ");
		system ("pause");
		return NULL;
	}
	arguments_params->model.parameters.users_number = atoi (argv[3]);
	if (arguments_params->model.parameters.users_number < 1)
	{
		printf ("users_number must be a positif integer \n ");
		system ("pause");
		return NULL;
	}
	arguments_params->model.parameters.items_number = atoi (argv[4]);
	if (arguments_params->model.parameters.items_number < 1)
	{
		printf ("items_number must be a positif integer \n ");
		system ("pause");
		return NULL;
	}

	file_path_length = strlen (argv[5]) + 1;

	arguments_params->file_path = (char*) malloc (file_path_length);
	strcpy (arguments_params->file_path, argv[5]);


	arguments_params->model.parameters.dimensionality = atoi (argv[6]);
	if (arguments_params->model.parameters.dimensionality < 1)
	{
		printf ("dimensionality must be a positif integer \n ");
		system ("pause");
		return NULL;
	}
	
	arguments_params->model.parameters.iteration_number = atoi (argv[7]);
	if (arguments_params->model.parameters.iteration_number < 1)
	{
		printf ("iteration_number must be a positif integer \n ");
		system ("pause");
		return NULL;
	}
	

	arguments_params->model.parameters.lambda = (float) atof (argv[8]);
	

	arguments_params->model.parameters.step = (float) atof (argv[9]);

	arguments_params->model.parameters.lambda_bias = (float) atof (argv[10]);
	

	arguments_params->model.parameters.step_bias = (float) atof (argv[11]);

	arguments_params->model.parameters.proj_family_size = (int) atoi (argv[12]);

	bin_width_ratio = (int) atoi (argv[13]);

	if (strcmp (argv[14], "basic") == 0)
	{
		printf ("basic \n");
		arguments_params->model.learning_algorithm  = learn_basic_mf;
		arguments_params->model.rating_estimator = estimate_rating_basic_mf;
		arguments_params->model.parameters.algoithm_type = 0;
	}
	else if (strcmp (argv[14], "bias") == 0)
	{
		printf ("bias \n");
		arguments_params->model.learning_algorithm = learn_mf_bias;
		arguments_params->model.rating_estimator   = estimate_rating_mf_bias;
		arguments_params->model.update_algorithm = update_learning_with_training_set;
		arguments_params->model.parameters.algoithm_type = BIAS;
	}
	else if (strcmp (argv[14], "MFneighbors") == 0)
	{
		printf ("MFneighbors \n");
		arguments_params->model.learning_algorithm = learn_mf_neighbor;
		arguments_params->model.rating_estimator   = estimate_rating_mf_neighbor;
		arguments_params->model.update_algorithm = update_learning_with_training_set_neighborMF;
		arguments_params->model.parameters.algoithm_type = NEIGHBOURS_MF;

	}else if (strcmp (argv[14], "social") == 0)
	{
		printf ("social \n");
		arguments_params->model.learning_algorithm = learn_social;
		arguments_params->model.rating_estimator   = estimate_rating_social;
		arguments_params->model.parameters.algoithm_type = SOCIAL;
		if(argc!=18)
			return NULL;
		arguments_params->social_relations_file_path = malloc(strlen(argv[15]+1));
		strcpy(arguments_params->social_relations_file_path,argv[15]);
		arguments_params->social_relations_number = atoi(argv[16]);
		arguments_params->model.parameters.betha=atoi(argv[17]);
	}
	else
	{
		printf ("type must be bias or basic");
		return NULL;
	}
	arguments_params->model.parameters.bin_width = (int)(arguments_params->model.parameters.items_number * bin_width_ratio / 100.0);
	
	arguments_params->model.parameters.seed = 4578;
	return arguments_params;

}


training_set_t* server_extract_data(server_parameters_t* server_param)
{
	training_set_t* training_set = init_training_set(server_param->model.parameters);
	FILE * file = fopen (server_param->file_path, "r");
	size_t i,j,k;
	double m;
	assert (file);

	if (!file)
	{
		return NULL;
	}
	while (!feof (file) )
	{
		if (fscanf (file, "%u %u %lf %u", &i, &j, &m,&k) < 4)
		{
			break;
		}
		set_known_rating (i-1  , j-1 , (float) m, training_set);
	}

	fclose (file);
	return training_set;
}
