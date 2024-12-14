
//#include<iostream>
#ifndef buffer_H
#define buffer_H

#define MAXPAGES 240000
#define MAXTENANTS 4

double cost_func(double sla_date);
double cost_func2(double sla_date);
//void te();


const char c[15]="data14.txt";
const double e=2.7182818;
const double learning_rate=0.45;
const double frac_buffer=0.5;
//const double frac_buffer=0.0625;
const int vir_buffer_per_tena=1024;
const double zero=0.00000001;
//const double pri_level[MAXTENANTS+2]={4,1,4,1,4,1,4,1,4,1,4,1};
//const int vir_size[MAXTENANTS+2]={vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena,vir_buffer_per_tena};


#endif