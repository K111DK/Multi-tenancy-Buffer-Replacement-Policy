//局部替换均使用LRU
#include"buffer.h"
#include<iostream>
#include<fstream>
#include<set>
#include<cstring>
#include<ctime>
#include<queue>
#include<cmath>

/*
#define FRAMESIZE 4096
#define DEFBUFSIZE 8192
#define MAXPAGES 150000
#define MAXTENANTS 10
#define refer_total 500000
*/
using namespace std;

string s;
int te_ref=0;
int tena_teref[MAXTENANTS+2];

struct page_timestamp
{
    int pageid;
    int timestamp;

};

struct LFU_ele{
    int page_id;
    int refer_num;
    int last_refer;
    bool operator < (const LFU_ele &A) const
    { 
        if (refer_num==A.refer_num)
        {
            return last_refer > A.last_refer; 
        }
        return refer_num > A.refer_num; 
    }//小顶堆
};

/*
class HList
{
    public:
        void Init(int max_size)
        {
            maxsize=max_size;
            Size=0;
            for(int i=1;i<=MAXPAGES;i++)
            {
                page_in_list[i]=0;
            }
            

        }
        void Push(int page_id,int timestamp)
        {
            pq.push({page_id,timestamp});
            page_in_list[page_id]=1;
            Size++;
        }
        void Pop()
        {
            page_timestamp te_pt=pq.front();
            while(page_in_list[te_pt.pageid]==0)
            {
                pq.pop();
                te_pt=pq.front();
            }
            page_in_list[te_pt.pageid]=0;
            pq.pop();
            Size--;
        }
        int Getsize()
        {
            return Size;
        }

    private:
        int Size;
        int maxsize;
        int page_in_list[MAXPAGES+2];
        queue<page_timestamp> pq;
       

};
*/
class Buffer
{
    public:
    void Init(int max_buffer,int max_queue,int vir_size[],double pri_level[])
    {
    
        maxbuffer=max_buffer;
        buffersize=0;
        for(int i=1;i<=MAXPAGES;i++)
        {
            page_in_buffer[i]=0;
        }
        maxsla_money=0;
        for(int i=1;i<=MAXTENANTS;i++)
        {
            sla_money[i]=pri_level[i];
            maxsla_money=max(maxsla_money,sla_money[i]);
            hits[i]=faults[i]=0;
            tena_buffersize[i]=0;
            vir_maxbuffersize[i]=vir_size[i];
            vir_buffersize[i]=0;
            vir_faults[i]=vir_hits[i]=0;
        }
        totalhits=0;
        totalfaults=0;
        //hlist.Init(max_queue);
        glolist_maxsize=max_queue;
        glolist_size=0;
        for(int i=1;i<=MAXPAGES;i++)
        {
            page_in_glolist[i]=0;
        }
        for(int i=1;i<=MAXTENANTS;i++)
        {
            zone_weights[i]=1.0/(MAXTENANTS*1.0);
        }

        for(int i=1;i<=MAXPAGES;i++)
        {
            last_refer[i]=0;
        }

        for(int i=1;i<=MAXPAGES;i++)
        {
            refer_num[i]=0;
        }
        

    }

    void glolist_push(int page_id,int timestamp)
    {
        if (glolist_size>=glolist_maxsize)
        {
            glolist_pop();
        }
        gloqueue.push({page_id,timestamp});
        page_in_glolist[page_id]=1;
        last_refer[page_id]=timestamp;
        glolist_size++;
    }
    void glolist_pop()
    {
        page_timestamp te_pt=gloqueue.front();
        while(!(page_in_glolist[te_pt.pageid]&&last_refer[te_pt.pageid]==te_pt.timestamp))
        {
            gloqueue.pop();
            te_pt=gloqueue.front();
        }
        page_in_glolist[te_pt.pageid]=0;
        gloqueue.pop();
        glolist_size--;
    }
    void glolist_erase(int pageid)
    {
        page_in_glolist[pageid]=0;
        glolist_size--;
        
    }
    int LRUevit(int zone_num)
    {
        
        
        page_timestamp page_to_evit=LRUqueue[zone_num].front();
        while(!(page_in_buffer[page_to_evit.pageid]&&page_to_evit.timestamp==last_refer[page_to_evit.pageid]))
        {
            LRUqueue[zone_num].pop();
            page_to_evit=LRUqueue[zone_num].front();
        }
        return page_to_evit.pageid;

    }

    int LFUevit(int zone_num)
    {
        LFU_ele page_to_evit=LFUqueue[zone_num].top();
        while(!(page_in_buffer[page_to_evit.page_id]&&page_to_evit.last_refer==last_refer[page_to_evit.page_id]))
        {
            LFUqueue[zone_num].pop();
            page_to_evit=LFUqueue[zone_num].top();
        }
        return page_to_evit.page_id;

    }


    int vir_getvicpage(int zone_num)
    {
        
        
           
        
        
        page_timestamp page_to_evit=virLRUqueue[zone_num].front();
        while(!(page_in_virbuffer[page_to_evit.pageid]&&page_to_evit.timestamp==vir_lastrefer[page_to_evit.pageid]))
        {
            virLRUqueue[zone_num].pop();
            page_to_evit=virLRUqueue[zone_num].front();
        }
        return page_to_evit.pageid;
    }
    void reset()
    {
        
        int onesize_num=0;
        double sum=0;
        for(int i=1;i<=MAXTENANTS;i++)
        {
            if ((tena_buffersize[i]<=maxbuffer/MAXTENANTS/MAXTENANTS) && (zone_weights[i]>(1.0/(MAXTENANTS*1.0))))
            {
                //cout<<tena_buffersize[i]<<" "<<zone_weights[i]<<endl;
                
                onesize_num++;
            }
            else
            {
                sum=sum+zone_weights[i];
            }
        }
        
        for(int i=1;i<=MAXTENANTS;i++)
        {
            if (tena_buffersize[i]<=maxbuffer/MAXTENANTS/MAXTENANTS && zone_weights[i]>1.0/(MAXTENANTS*1.0))
            {
                zone_weights[i]=1.0/(MAXTENANTS*1.0);
            }
            else
            {
                zone_weights[i]=zone_weights[i]*(1.0-1.0/(MAXTENANTS*1.0)*(onesize_num*1.0))/sum;

            }
        

        }
        


    }
        
    
    int getvicpage(int time_stamp,int tena_num)
    {
        int zone_num;
        if(faults[tena_num]<=vir_faults[tena_num])
        {
            zone_num=tena_num;
        }
       
       else
       {
        double random=double(rand()) / double(RAND_MAX);
        
        
            zone_num=0;
            while(zone_num==0)
            {
                for(int i=1;i<=MAXTENANTS;i++)
                {
                    random=random-zone_weights[i];
                    if (random<zero)
                    {
                        zone_num=i;
                        break;
                    }
                }
                random=double(rand()) / double(RAND_MAX);

            }
            
           
            //if(tena_buffersize[zone_num]<=1)
            if(tena_buffersize[zone_num]<=maxbuffer/MAXTENANTS/MAXTENANTS)
            {
                
                random=double(rand()) / double(RAND_MAX);
                
                
                double sum=0;
                for(int i=1;i<=MAXTENANTS;i++)
                {
                    if (tena_buffersize[i]<=maxbuffer/MAXTENANTS/MAXTENANTS)
                    {
                        //zone_weights[i]=0.0;
                        te_zone_weights[i]=0.0;
                    }
                    else
                    {
                        sum=sum+zone_weights[i];
                    }
                }
                for(int i=1;i<=MAXTENANTS;i++)
                {
                    if (tena_buffersize[i]>=maxbuffer/MAXTENANTS/MAXTENANTS+1)
                    {
                        //zone_weights[i]=zone_weights[i]/sum;
                        te_zone_weights[i]=zone_weights[i]/sum;
                    }
                
                }
                

                zone_num=0;
                while(zone_num==0 || tena_buffersize[zone_num]<=maxbuffer/MAXTENANTS/MAXTENANTS)
                {
                    for(int i=1;i<=MAXTENANTS;i++)
                    {
                        random=random-te_zone_weights[i];
                        if (random<zero)
                        {
                            zone_num=i;
                            break;
                        }
                    }
                    random=double(rand()) / double(RAND_MAX);

                }
                
                
                reset();
                
                
                

            }
       }
           
        
        
        
        int page_to_evit=LRUevit(zone_num);
        
        tena_buffersize[zone_num]--;
        return page_to_evit;
    }

    void updateweight(int page_id,int tena_num,double learning_rate)
    {
        
        if(page_in_glolist[page_id])
        {
            //double sla_rate=((max(vir_faults[tena_num],faults[tena_num])-vir_faults[tena_num])*1.0)/(vir_faults[tena_num]*1.0);
            double sla_rate=(max(0,faults[tena_num]-vir_faults[tena_num])*1.0)/((faults[tena_num]+hits[tena_num])*1.0);
            
            
            //zone_weights[tena_num]=zone_weights[tena_num]*pow(e,-1.0*(sla_money[tena_num]/maxsla_money)*sla_rate);

            zone_weights[tena_num]=max(zero,zone_weights[tena_num]*pow(e,-1.0*(sla_money[tena_num]/maxsla_money)*sla_rate));
            page_in_glolist[page_id]=0;
            glolist_size--;
            double sum=0;
            for(int i=1;i<=MAXTENANTS;i++)
            {
                sum=sum+zone_weights[i];
            }
            for(int i=1;i<=MAXTENANTS;i++)
            {
                zone_weights[i]=zone_weights[i]/sum;
            }
            
        }
    }

    void fixpage(int page_id,int tena_num,int time_stamp)
    {
        
        if (page_in_buffer[page_id])
        {
            //page在buffer中
            totalhits++;
            hits[tena_num]++;
            LRUqueue[tena_num].push({page_id,time_stamp});
            last_refer[page_id]=time_stamp;
            refer_num[page_id]++;
            LFUqueue[tena_num].push({page_id,refer_num[page_id],time_stamp});
            return;
        }
        
        if(buffersize<maxbuffer)
        {
            
            buffersize++;
            page_in_buffer[page_id]=1;
            //tena_buffersize[tena_num]++;
            
        }
        else
        {
            //需要替换
            updateweight(page_id,tena_num,learning_rate);

            int vic_page=getvicpage(time_stamp,tena_num);
            
            glolist_push(vic_page,time_stamp);
            page_in_buffer[vic_page]=0;
            page_in_buffer[page_id]=1;

        }
        tena_buffersize[tena_num]++;
        totalfaults++;
        faults[tena_num]++;
        LRUqueue[tena_num].push({page_id,time_stamp});
        last_refer[page_id]=time_stamp;
        refer_num[page_id]=1;
        LFUqueue[tena_num].push({page_id,refer_num[page_id],time_stamp});
        


    }
    void vir_fixpage(int page_id,int tena_num,int time_stamp)
    {

        if (page_in_virbuffer[page_id])
        {
            //page在buffer中
            
            vir_hits[tena_num]++;
            virLRUqueue[tena_num].push({page_id,time_stamp});
            vir_lastrefer[page_id]=time_stamp;
            
            return;
        }
        
        vir_faults[tena_num]++;
        if(vir_buffersize[tena_num]<vir_maxbuffersize[tena_num])
        {
            
            vir_buffersize[tena_num]++;
            page_in_virbuffer[page_id]=1;
            
        }
        else
        {
            //需要替换
            int vic_page=vir_getvicpage(tena_num);
           
            
            page_in_virbuffer[vic_page]=0;
            page_in_virbuffer[page_id]=1;

        }
        virLRUqueue[tena_num].push({page_id,time_stamp});
        vir_lastrefer[page_id]=time_stamp;
        


    }
    void Print()
    {
        cout<<c<<endl;
        cout<<"frac_buffer: "<<frac_buffer<<endl;
        cout<<"actual buffersize: "<<maxbuffer<<endl;

        

        //cout<<endl;

        //cout<<"total hits: "<<totalhits<<" total faults: "<<totalfaults<<" total HR: "<<(totalhits*1.0)/((totalhits+totalfaults)*1.0);
        
        for(int i=1;i<=MAXTENANTS;i++)
        {
            cout<<"tena_id: "<<i;
            //cout<<" hits: "<<hits[i]<<" faults: "<<faults[i]<<" HR: "<<(hits[i]*1.0)/((hits[i]+faults[i])*1.0);
            cout<<" HR: "<<(hits[i]*1.0)/((hits[i]+faults[i])*1.0);
            cout<<" base_size: "<<vir_maxbuffersize[i];
            //cout<<" base_hits: "<<vir_hits[i]<<" base_faults: "<<vir_faults[i]<<" HR: "<<(vir_hits[i]*1.0)/((vir_hits[i]+vir_faults[i])*1.0);
            cout<<" HR: "<<(vir_hits[i]*1.0)/((vir_hits[i]+vir_faults[i])*1.0);
            cout<<endl;
        }
        
        
        cout<<"tena_buffer: ";
        for(int i=1;i<=MAXTENANTS;i++)
        {
            cout<<tena_buffersize[i]<<" ";
        }
        cout<<endl;
        
        cout<<"money: ";
        for(int i=1;i<=MAXTENANTS;i++)
        {
            cout<<sla_money[i]<<" ";
        }
        cout<<endl;

        double cost=0;
        double cost2=0;
        double sla_rate;
        cout<<"sla_rate: ";
        for(int i=1;i<=MAXTENANTS;i++)
        {
            //sla_rate=((max(vir_faults[i],faults[i])-vir_faults[i])*1.0)/(vir_faults[i]*1.0);
            sla_rate=(max(0,faults[i]-vir_faults[i])*1.0)/((faults[i]+hits[i])*1.0);
            cout<<sla_rate<<" ";
            cost=cost+sla_money[i]*cost_func(sla_rate);
            cost2=cost2+sla_money[i]*cost_func2(sla_rate);
        }
        cout<<endl;
        cout<<"total HR: "<<endl<<(totalhits*1.0)/((totalhits+totalfaults)*1.0);
        
        cout<<endl<<cost;
        cout<<endl<<cost2;


        
    }

    private:
        int page_in_buffer[MAXPAGES+2];
        int buffersize;
        int tena_buffersize[MAXTENANTS+2];
        int maxbuffer;
        
        //HList hlist;
        int totalhits;
        int totalfaults;
        int hits[MAXTENANTS+2];
        int faults[MAXTENANTS+2];

        int glolist_size;
        int glolist_maxsize;
        int page_in_glolist[MAXPAGES+2];
        queue<page_timestamp> gloqueue;
        double zone_weights[MAXTENANTS+2];
        double te_zone_weights[MAXTENANTS+2];
        queue<page_timestamp> LRUqueue[MAXTENANTS+2];
        priority_queue<LFU_ele> LFUqueue[MAXTENANTS+2];
        int refer_num[MAXPAGES+2];

        int last_refer[MAXPAGES+2];

        int vir_maxbuffersize[MAXTENANTS+2];
        int vir_buffersize[MAXTENANTS+2];
        int page_in_virbuffer[MAXPAGES+2];
        int vir_hits[MAXTENANTS+2];
        int vir_faults[MAXTENANTS+2];
        int vir_lastrefer[MAXPAGES+2];
        queue<page_timestamp> virLRUqueue[MAXTENANTS+2];

        double sla_money[MAXTENANTS+2];
        double maxsla_money;

};
int vir_size[MAXTENANTS+2];
double pri_level[MAXTENANTS+2];
int tenant;
int page_refer;
Buffer bf;
int total_buffersize=0;

int main()
{
    //cout<<"h"<<endl;
    
    for(int i=1;i<=MAXTENANTS;i++)
    {
        vir_size[i]=vir_buffer_per_tena;
        total_buffersize=total_buffersize+vir_size[i];
        pri_level[i]=1;
        
        if (i%2==1)
        {
            pri_level[i]=4;
        }
        
    }
    
    
    
    bf.Init(int(total_buffersize*frac_buffer),int(total_buffersize*frac_buffer),vir_size,pri_level);
    
    
    ifstream myfile(c);
    //myfile>>tena[i];
	//myfile>>refer[i];
    while(myfile>>tenant)
	{
        
        te_ref++;
        
	    myfile>>page_refer;
        tena_teref[tenant]++;
        bf.fixpage(page_refer,tenant,tena_teref[tenant]);
        bf.vir_fixpage(page_refer,tenant,tena_teref[tenant]);
        
	}

    bf.Print();
    
   
    

}