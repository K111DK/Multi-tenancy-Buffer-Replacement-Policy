#include"buffer.h"

double cost_func(double sla_date){
    //cout<<"te";
    //te();
    return sla_date*sla_date;
}



double cost_func2(double sla_date){
    if(sla_date<=0.1)
    {
        return sla_date*1.5;
    }
    if (sla_date*3.5-0.2>=1)
    {
        return 1;
    }
    return sla_date*3.5-0.2;
}

/*
void te()
{

}
*/