#pragma once
#include <string>
#include <vector>
#include <chrono>

#include <stdint.h>




///// progress bar
class Progress
{
public:
    int timer_mode;
    std::chrono::steady_clock::time_point start_time;
    double heure_actuelle ;
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
        start_time = std::chrono::steady_clock::now();

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
                auto now = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed = now - start_time;
                double seconds_elapsed = elapsed.count();
                
                double speed = done / seconds_elapsed;
                double rem = (todo-done) / speed;
                if(done>todo) rem=0;
                int min_e  = static_cast<int>(seconds_elapsed / 60) ;
                seconds_elapsed -= min_e*60;
                int min_r  = static_cast<int>(rem / 60) ;
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
            auto now = std::chrono::steady_clock::now();
            double elapsed_sec = std::chrono::duration_cast<std::chrono::duration<double>>(now - start_time).count();

            uint64_t total_done = 0;
            for (int ii = 0; ii < _nthreads; ++ii)
                total_done += done_threaded[ii];

            double speed = total_done / elapsed_sec;
            double remaining_sec = (todo > total_done) ? (todo - total_done) / speed : 0.0;

            int min_e = static_cast<int>(elapsed_sec / 60);
            double sec_e = elapsed_sec - min_e * 60;
            int min_r = static_cast<int>(remaining_sec / 60);
            double sec_r = remaining_sec - min_r * 60;

                    fprintf(stderr,"%c[%s]  %-5.3g%%   elapsed: %3i min %-2.0f sec   remaining: %3i min %-2.0f sec",13,
                        message.c_str(),
                        100*(double)total_done/todo,
                        min_e,sec_e,min_r,sec_r);
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