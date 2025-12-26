#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include "bitvector.hpp"

using namespace boomphf;

static void BM_SetResetGet(benchmark::State& state)
{
    const uint64_t nbits = static_cast<uint64_t>(state.range(0));
    bitVector bv(nbits);

    for (auto _ : state)
    {
        // set every 3rd bit
        for (uint64_t i = 0; i < nbits; i += 3)
            bv.set(i);

        // random reads
        for (uint64_t i = 0; i < nbits; i += 7)
            benchmark::DoNotOptimize(bv.get(i));

        // reset some bits
        for (uint64_t i = 0; i < nbits; i += 5)
            bv.reset(i);
    }
}
BENCHMARK(BM_SetResetGet)->Arg(1<<10)->Arg(1<<16)->Arg(1<<20)->Arg(1<<24)->Unit(benchmark::kMillisecond);

static void BM_AtomicTestAndSet(benchmark::State& state)
{
    const uint64_t nbits = static_cast<uint64_t>(state.range(0));
    bitVector bv(nbits);

    for (auto _ : state)
    {
        for (uint64_t i = 0; i < nbits; ++i)
        {
            benchmark::DoNotOptimize(bv.atomic_test_and_set(i));
        }
    }
}
BENCHMARK(BM_AtomicTestAndSet)->Arg(1<<10)->Arg(1<<16)->Arg(1<<20)->Unit(benchmark::kMillisecond);

static void BM_BuildRanks(benchmark::State& state)
{
    const uint64_t nbits = static_cast<uint64_t>(state.range(0));
    bitVector bv(nbits);
    // set some bits to get non-trivial ranks
    for (uint64_t i = 0; i < nbits; i += 3)
        bv.set(i);

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(bv.build_ranks(0));
    }
}
BENCHMARK(BM_BuildRanks)->Arg(1<<10)->Arg(1<<16)->Arg(1<<20)->Unit(benchmark::kMillisecond);

static void BM_RankQueries(benchmark::State& state)
{
    const uint64_t nbits = static_cast<uint64_t>(state.range(0));
    bitVector bv(nbits);
    for (uint64_t i = 0; i < nbits; i += 3)
        bv.set(i);
    [[maybe_unused]] auto ignored = bv.build_ranks(0);

    for (auto _ : state)
    {
        for (uint64_t i = 0; i < nbits; i += 13)
            benchmark::DoNotOptimize(bv.rank(i));
    }
}
BENCHMARK(BM_RankQueries)->Arg(1<<10)->Arg(1<<16)->Arg(1<<20)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
