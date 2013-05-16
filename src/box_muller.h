#ifndef BOX_MULLER_H
#define BOX_MULLER_H
struct box_muller_genrator
{
	int seed;
	int use_last;
	double sd;
	double mean;
	double last_value;
};
typedef struct box_muller_genrator box_muller_generator_t;

box_muller_generator_t* init_box_muller_generator(int seed,	double sd,double mean);

double box_muller(box_muller_generator_t* gen);
#endif