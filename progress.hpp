#pragma once
#include <string>
#include <vector>
#include <stdint.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sys/timeb.h>   // for _ftime_s
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/time.h>
#endif


///// progress bar
class Progress
{
public:
    int timer_mode;
    struct timeval timestamp;
    double heure_debut, heure_actuelle ;
    std::string   message;

    uint64_t done;
    uint64_t todo;
    int subdiv ; // progress printed every 1/subdiv of total to do
    double partial;
    int _nthreads;
    std::vector<double > partial_threaded;
    std::vector<uint64_t > done_threaded;

    double steps ; //steps = todo/subidv

    void init(uint64_t ntasks, const char * msg,int nthreads =1)
    {
        _nthreads = nthreads;
        message = std::string(msg);
        gettimeofday(&timestamp, NULL);
        heure_debut = timestamp.tv_sec +(timestamp.tv_usec/1000000.0);

        //fprintf(stderr,"| %-*s |\n",98,msg);

        todo= ntasks;
        done = 0;
        partial =0;
        
        partial_threaded.resize(_nthreads);
        done_threaded.resize(_nthreads);
        
        for (int ii=0; ii<_nthreads;ii++) partial_threaded[ii]=0;
        for (int ii=0; ii<_nthreads;ii++) done_threaded[ii]=0;
        subdiv= 1000;
        steps = (double)todo / (double)subdiv;

        if(!timer_mode)
        {
                fprintf(stderr,"[");fflush(stderr);
        }
    }

    void finish()
    {
        set(todo);
            if(timer_mode)
            fprintf(stderr,"\n");
            else
            fprintf(stderr,"]\n");

        fflush(stderr);
        todo= 0;
        done = 0;
        partial =0;

    }
    void finish_threaded()// called by only one of the threads
    {
        done = 0;
        double rem = 0;
        for (int ii=0; ii<_nthreads;ii++) done += (done_threaded[ii] );
        for (int ii=0; ii<_nthreads;ii++) partial += (partial_threaded[ii] );

        finish();

    }
    void inc(uint64_t ntasks_done)
    {
        done += ntasks_done;
        partial += ntasks_done;


        while(partial >= steps)
        {
            if(timer_mode)
            {
                gettimeofday(&timestamp, NULL);
                heure_actuelle = timestamp.tv_sec +(timestamp.tv_usec/1000000.0);
                double elapsed = heure_actuelle - heure_debut;
                double speed = done / elapsed;
                double rem = (todo-done) / speed;
                if(done>todo) rem=0;
                int min_e  = (int)(elapsed / 60) ;
                elapsed -= min_e*60;
                int min_r  = (int)(rem / 60) ;
                rem -= min_r*60;

                fprintf(stderr,"%c[%s]  %-5.3g%%   elapsed: %3i min %-2.0f sec   remaining: %3i min %-2.0f sec",13,
                    message.c_str(),
                    100*(double)done/todo,
                    min_e,elapsed,min_r,rem);

            }
            else
            {
                    fprintf(stderr,"-");fflush(stderr);
            }
            partial -= steps;
        }


    }

    void inc(uint64_t ntasks_done, int tid) //threads collaborate to this same progress bar
    {
        partial_threaded[tid] += ntasks_done;
        done_threaded[tid] += ntasks_done;
        while(partial_threaded[tid] >= steps)
        {
            if(timer_mode)
            {
                struct timeval timet;
                double now;
                gettimeofday(&timet, NULL);
                now = timet.tv_sec +(timet.tv_usec/1000000.0);
                uint64_t total_done  = 0;
                for (int ii=0; ii<_nthreads;ii++) total_done += (done_threaded[ii] );
                double elapsed = now - heure_debut;
                double speed = total_done / elapsed;
                double rem = (todo-total_done) / speed;
                if(total_done > todo) rem =0;
                int min_e  =  (int)(elapsed / 60) ;
                elapsed -= min_e*60;
                int min_r  =  (int)(rem / 60) ;
                rem -= min_r*60;

                    fprintf(stderr,"%c[%s]  %-5.3g%%   elapsed: %3i min %-2.0f sec   remaining: %3i min %-2.0f sec",13,
                        message.c_str(),
                        100*(double)total_done/todo,
                        min_e,elapsed,min_r,rem);
            }
            else
            {
                    fprintf(stderr,"-");fflush(stderr);
            }
            partial_threaded[tid] -= steps;

        }

    }

    void set(uint64_t ntasks_done)
    {
        if(ntasks_done > done)
            inc(ntasks_done-done);
    }
    Progress () :     timer_mode(0) {}
    //include timer, to print ETA ?
};