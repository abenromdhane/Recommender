#ifndef SERVER_PARAMERTES_H
#define SERVER_PARAMERTES_H

#include "recommender.h"
#include "training_set.h"

struct server_parameters
{
	char* file_path;
	learning_model_t model;
	char* social_relations_file_path;
	size_t social_relations_number;
	size_t port;
};
typedef struct server_parameters server_parameters_t;


server_parameters_t* parse_arguments (int argc, char** argv);

training_set_t* server_extract_data(server_parameters_t* server_param);

#endif
