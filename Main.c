/*
 * Main.c
 *
 *  Created on: Mar 30, 2025
 *      Author: jacob a psimos
 *
 *  Description: use individual CPU cores to generate prime numbers and store them in individual records.
 */

#include <err.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

// Structures
typedef struct ThreadArguments
{
	unsigned int threadNum;
	unsigned int cpuCoreNum;
	unsigned long beginCalculationsAt;
	unsigned long endCalculationsAt;
	char outputFileName[FILENAME_MAX];
} ThreadArguments;

// Globals
volatile int receivedQuitSignal;

// Prototypes
void handleQuitSignal(int sig);
void* PrimeNumberGenerator(void* arg);
int IsPrimeNumber(unsigned long num);

int main(int argc, char *argv[])
{
	int result;
	size_t iter;
	struct sigaction sigquitAction;
	pthread_t* threads;
	pthread_attr_t threadAttribs;
	struct ThreadArguments* threadArguments;
	cpu_set_t cpuSet;
	ssize_t numberOfCpuCores;

	result = EXIT_SUCCESS;

	receivedQuitSignal = 0;

	sigquitAction.sa_handler = handleQuitSignal;
	sigemptyset(&sigquitAction.sa_mask);

	if(sigaction(SIGQUIT, &sigquitAction, NULL) == -1)
	{
		err(EXIT_FAILURE, "Could not setup SIGQUIT.");
	}

	// Get the number of CPU cores and create threads that run on each of them
	// where the begin and end positions on the number line are given. The work
	// load is not proportional with respect to number of prime numbers generated
	// per core and gets lower as the core number grows.

	numberOfCpuCores = sysconf(_SC_NPROCESSORS_ONLN);

	if(EINVAL == numberOfCpuCores)
	{
		err(EXIT_FAILURE, "Can't enumerate number of CPU cores.");
	}

	threads = (pthread_t*)calloc(numberOfCpuCores, sizeof(pthread_t));
	threadArguments = (struct ThreadArguments*)calloc(numberOfCpuCores, sizeof(struct ThreadArguments));

	if(NULL == threads || NULL == threadArguments)
	{
		if(NULL != threads)
		{
			free(threads);
		}

		if(NULL != threadArguments)
		{
			free(threadArguments);
		}

		err(EXIT_FAILURE, "Memory allocation error.");
	}

	for(iter = 0; iter < numberOfCpuCores; iter++)
	{
		CPU_ZERO(&cpuSet);
		CPU_SET(iter, &cpuSet);

		threadArguments[iter].threadNum = iter + 1;
		threadArguments[iter].cpuCoreNum = iter + 1;
		threadArguments[iter].beginCalculationsAt = (ULONG_MAX / numberOfCpuCores) * iter;
		threadArguments[iter].endCalculationsAt = threadArguments[iter].beginCalculationsAt + (ULONG_MAX / numberOfCpuCores);
		snprintf(threadArguments[iter].outputFileName, FILENAME_MAX,
				"%s/PRIMES_THREAD_%lu.TXT", get_current_dir_name(), iter);

		result = pthread_attr_init(&threadAttribs);
		result |= pthread_attr_setstacksize(&threadAttribs, PTHREAD_STACK_MIN * 8);
		result |= pthread_attr_setaffinity_np(&threadAttribs, sizeof(cpuSet), &cpuSet);
		result |= pthread_create(&threads[iter], &threadAttribs, PrimeNumberGenerator, &threadArguments[iter]);
		result |= pthread_attr_destroy(&threadAttribs);

		if(result)
		{
			fprintf(stderr, "Thread creation result = 0x%x.\n", result);

			while(--iter >= 0)
			{
				result = pthread_cancel(threads[iter]);

				if(result)
				{
					fprintf(stderr, "Thread cancellation error: pthread_cancel = %d.\n", result);
				}
			}

			result = EXIT_FAILURE;
			break;
		}
	}

	if(EXIT_FAILURE != result)
	{
		for(iter = 0; iter < numberOfCpuCores && threads[iter] != 0; iter++)
		{
			result = pthread_join(threads[iter], NULL);

			if(result)
			{
				fprintf(stderr, "Thread join error: pthread_join = %d.\n", result);
			}
		}
	}

	free(threads);
	free(threadArguments);

	return result;
}

void handleQuitSignal(int sig)
{
	receivedQuitSignal = 1;
}

void* PrimeNumberGenerator(void* arg)
{
	struct ThreadArguments* threadArgs = (struct ThreadArguments*)arg;
	FILE* outputFile = fopen(threadArgs->outputFileName, "w");

	if(outputFile)
	{
		for(unsigned long num = threadArgs->beginCalculationsAt; num < threadArgs->endCalculationsAt && !receivedQuitSignal; num++)
		{
			if(IsPrimeNumber(num))
			{
				fprintf(outputFile, "%lu\n", num);
			}
		}

		fclose(outputFile);
	}
	else
	{
		fprintf(stderr, "Could not open %s on thread %u.\n",
				threadArgs->outputFileName, threadArgs->threadNum);
	}

	return NULL;
}

int IsPrimeNumber(unsigned long num)
{
	for(unsigned long denom = 2; denom < num; denom++)
	{
		if(num % denom == 0)
		{
			return 0;
		}
	}
	return 1;
}

