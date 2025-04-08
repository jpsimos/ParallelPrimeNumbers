# ParallelPrimeNumbers
Generate prime numbers using all CPU cores. This tool demonstrates how to
leverage multi-core processors to perform work at the same time. Each thread has its own core.
Assuming your processor is 64 bit, each thread will begin calculating at
(ULONG_MAX / NumberOfCores) * CoreNumber and will find all prime numbers until
the begining number plus (ULONG_MAX / NumberOfCores) is reached. The processor
core's prime number generation rate gets lower as the begining number gets larger
so expect lower cpu cores to generate more primes during a given length of time.
Output is stored separately at the current directory in files named PRIMES_THREAD_CORENUM.TXT

You must define _GNU_SOURCE, in this case it is done during compilation:

gcc -D_GNU_SOURCE -O3 --std=gnu99 -o ParallelPrimeNumbers Main.c -lpthread

The example invocation below runs the tool for one hour and saves the primes 

timeout -s STOP 1h ./ParallelPrimeNumbers
