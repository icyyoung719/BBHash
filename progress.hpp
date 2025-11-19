#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

///// progress bar
class Progress
{
  public:
	int timer_mode{0};
	std::chrono::steady_clock::time_point start_time{};
	double heure_actuelle{0.0};
	std::string message;

	uint64_t done{0};
	uint64_t todo{0};
	int subdiv{1000}; // progress printed every 1/subdiv of total to do
	double partial{0.0};
	int _nthreads{1};
	std::vector<double> partial_threaded;
	std::vector<uint64_t> done_threaded;

	double steps{0.0}; // steps = todo/subidv

	Progress() = default;

	void init(uint64_t ntasks, const char* msg, int nthreads = 1)
	{
		_nthreads = nthreads;
		message = std::string(msg);
		start_time = std::chrono::steady_clock::now();

		todo = ntasks;
		done = 0;
		partial = 0;

		partial_threaded.assign(static_cast<size_t>(_nthreads), 0.0);
		done_threaded.assign(static_cast<size_t>(_nthreads), 0);

		subdiv = 1000;
		steps = (subdiv > 0) ? (static_cast<double>(todo) / static_cast<double>(subdiv)) : 1.0;

		if (!timer_mode)
		{
			std::fprintf(stderr, "[");
			std::fflush(stderr);
		}
	}

	void finish()
	{
		set(todo);
		if (timer_mode)
			std::fprintf(stderr, "\n");
		else
			std::fprintf(stderr, "]\n");

		std::fflush(stderr);
		todo = 0;
		done = 0;
		partial = 0;
	}
	void finish_threaded() // called by only one of the threads
	{
		done = 0;
		partial = 0;
		for (int ii = 0; ii < _nthreads; ii++)
			done += done_threaded[ii];
		for (int ii = 0; ii < _nthreads; ii++)
			partial += partial_threaded[ii];

		finish();
	}
	void inc(uint64_t ntasks_done)
	{
		done += ntasks_done;
		partial += ntasks_done;

		while (partial >= steps && steps > 0.0)
		{
			if (timer_mode)
			{
				auto now = std::chrono::steady_clock::now();
				std::chrono::duration<double> elapsed = now - start_time;
				double seconds_elapsed = elapsed.count();

				double speed = (seconds_elapsed > 0.0) ? (static_cast<double>(done) / seconds_elapsed) : 0.0;
				double rem = 0.0;
				if (speed > 0.0)
				{
					if (todo > done)
						rem = static_cast<double>(todo - done) / speed;
					else
						rem = 0.0;
				}
				else
				{
					rem = 0.0;
				}
				if (done > todo)
					rem = 0;
				int min_e = static_cast<int>(seconds_elapsed / 60);
				double sec_e = seconds_elapsed - min_e * 60;
				int min_r = static_cast<int>(rem / 60);
				double sec_r = rem - min_r * 60;

				std::fprintf(stderr, "%c[%s]  %-5.3g%%   elapsed: %3i min %-2.0f sec   remaining: %3i min %-2.0f sec", 13,
				             message.c_str(),
				             100 * (static_cast<double>(done) / todo),
				             min_e, sec_e, min_r, sec_r);
			}
			else
			{
				std::fprintf(stderr, "-");
				std::fflush(stderr);
			}
			partial -= steps;
		}
	}

	void inc(uint64_t ntasks_done, int tid) // threads collaborate to this same progress bar
	{
		if (tid < 0 || tid >= _nthreads)
			return;
		partial_threaded[tid] += ntasks_done;
		done_threaded[tid] += ntasks_done;
		while (partial_threaded[tid] >= steps && steps > 0.0)
		{
			if (timer_mode)
			{
				auto now = std::chrono::steady_clock::now();
				double elapsed_sec = std::chrono::duration_cast<std::chrono::duration<double>>(now - start_time).count();

				uint64_t total_done = 0;
				for (int ii = 0; ii < _nthreads; ++ii)
					total_done += done_threaded[ii];

				double speed = (elapsed_sec > 0.0) ? (static_cast<double>(total_done) / elapsed_sec) : 0.0;
				double remaining_sec = (speed > 0.0 && todo > total_done) ? (static_cast<double>(todo - total_done) / speed) : 0.0;

				int min_e = static_cast<int>(elapsed_sec / 60);
				double sec_e = elapsed_sec - min_e * 60;
				int min_r = static_cast<int>(remaining_sec / 60);
				double sec_r = remaining_sec - min_r * 60;

				std::fprintf(stderr, "%c[%s]  %-5.3g%%   elapsed: %3i min %-2.0f sec   remaining: %3i min %-2.0f sec", 13,
				             message.c_str(),
				             100 * (static_cast<double>(total_done) / todo),
				             min_e, sec_e, min_r, sec_r);
			}
			else
			{
				std::fprintf(stderr, "-");
				std::fflush(stderr);
			}
			partial_threaded[tid] -= steps;
		}
	}

	void set(uint64_t ntasks_done)
	{
		if (ntasks_done > done)
			inc(ntasks_done - done);
	}
	// include timer, to print ETA ?
};