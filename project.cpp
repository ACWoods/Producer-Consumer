/******************************************************************
 * Program : FINAL PROJECT
 *
 * Programmer: ALLEN WOODS, MELISSA NIERLE
 *
 * Due Date: APRIL 29 2014
 *
 * CMSC 312, SPRING 2014      Instructor: MENG YU
 *
 * Pledge: I HAVE NEITHER GIVEN NOR RECEIVED UNAUTHORIZED AID ON THIS PROGRAM
 *
 * Description: SIMULATES THE CUSTOMER-TELLER PROBLEM USING THREADS 
 *	AND PRODUCER/CONSUMER FUNCTIONS
 *
 * Input: NONE
 *
 * Output: PRINTED STATEMENTS DESCRIBING PROGRAM OUTPUT
 *
 ******************************************************************/

/***General Comments***/
/*Initially, printf() statements were being used for debugging.  However, incorrect variable values were being reported.  All printf()
/*statements have been replaced with cout() statements.
/***End of General Comments***/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <iostream>

#define MAX_QUEUE_SIZE 20
#define NO_OF_TELLERS 3

using namespace std;

pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER, qcount_mutex = PTHREAD_MUTEX_INITIALIZER, avail_tell_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition1 = PTHREAD_COND_INITIALIZER, condition2 = PTHREAD_COND_INITIALIZER, condition3 = PTHREAD_COND_INITIALIZER;

int q_count = 0, available_tellers = NO_OF_TELLERS, sched_cust_count = 0;
int customer_queue[MAX_QUEUE_SIZE] = {0}, counters[NO_OF_TELLERS] = {0}, sched_customer = 0;

void *customer_generator(void *)	/*Produces customer queue elements*/
{
	int produce = 0, m = 0;
	while(1)
	{
		/*Locking mutex and waiting for signal to release.  No function can touch the customer queue until it locks qcount_mutex.*/
		pthread_mutex_lock(&qcount_mutex);	/*Acquires lock on mutex; blocks thread if mutex is already locked*/	
		
		pthread_mutex_lock(&cout_mutex);	/*Locks cout_mutex to prevent other threads from distorting screen output*/
		
		cout.flush();
		cout<<endl<<q_count<<" CUSTOMERS are waiting in the queue."<<endl;
		pthread_mutex_unlock(&cout_mutex);
		pthread_mutex_unlock(&qcount_mutex);
		pthread_cond_signal(&condition2);


		pthread_mutex_lock(&qcount_mutex);
		if(q_count == MAX_QUEUE_SIZE)	/*Full buffer*/
		{
			sleep(1);	/*Sleeps for one second so the program reports the no. of customers in the queue at a minimum 1 second rate*/
			pthread_cond_wait(&condition1, &qcount_mutex);	/*Blocks thread until signaled on variable condition, then retakes control of mutex*/
		}	/*end if*/
			
		else if(q_count == 0 || (q_count < MAX_QUEUE_SIZE && q_count != 0))	/*Partially full buffer*/
		{
			pthread_cond_signal(&condition1);

			sleep(3);	/*Customer generator makes decision on producing customers every 3 sec*/
			srand(time(NULL));
			produce = rand() % 5 + 1;

			if(produce > 3)	/*This block will allow the generator to produce a customer 'some of the time' after waking*/
			{
				pthread_cond_signal(&condition1); /*Signal scheduler() that a customer has been produced*/
				sleep(1);
				pthread_mutex_lock(&qcount_mutex);
				customer_queue[q_count] = 1;
				q_count++;
				pthread_mutex_lock(&cout_mutex);
				cout.flush();
				cout<<endl<<"A new CUSTOMER has entered the queue!"<<endl;
				pthread_mutex_unlock(&cout_mutex);
			}	/*end if*/
			
			if(q_count - 1 == 0)	/*Empty buffer*/
				pthread_cond_signal(&condition1);	/*Signal to scheduler() to wake up since the buffer will no longer be empty. Has no effect if no threads are blocked on condition1.*/

			else
				pthread_mutex_unlock(&qcount_mutex);	/*Unlocks the mutex after producing a customer*/
		}	/*end else if*/		
	}	/*end while*/
}	/*end customer_generator()*/


void *scheduler(void *)	/*Consumes customer queue elements / Produces teller counter elements*/
{
	int consume = 0, customeradded = 0, i = 0, k = 0;
	while(1)
	{
		/*Locking mutex and waiting for signal to release*/
		pthread_mutex_lock(&qcount_mutex);
		pthread_mutex_lock(&avail_tell_mutex);

		if(q_count == 0)	/*Empty buffer*/
			pthread_cond_wait(&condition1, &qcount_mutex);	/*Wait here until the customer generator places a customer in the queue*/
			
		if(q_count == MAX_QUEUE_SIZE || (q_count < MAX_QUEUE_SIZE && q_count != 0))	/*Full or partially full buffer*/
		{
			pthread_mutex_unlock(&qcount_mutex);

			if(available_tellers == 0)	/*No teller counter is available*/
				pthread_cond_wait(&condition2, &avail_tell_mutex);	/*Wait here until a teller signals that it is available*/
			
			if(available_tellers > 0 && available_tellers <= NO_OF_TELLERS)	/*At least one teller counter is available*/
			{
				pthread_mutex_lock(&qcount_mutex);
				sched_customer = customer_queue[q_count - 1];	/*Scheduler receives customer*/
				customer_queue[q_count - 1] = 0;
				q_count--;
					
				for(i = 0; i < NO_OF_TELLERS; i++)
				{
					customeradded = 0;
							
					if(counters[i] == 0)
					{
						counters[i] = sched_customer;	/*Customer is sent to the first empty teller counter*/
						sched_customer = 0;
						customeradded++;
						break;
					}	/*end if*/
							
					else if(counters[i] == 1)
						continue;
						
				}	/*end for*/
	
						
				if(customeradded == 0 && available_tellers != 1)	/*Error checks available tellers and empty counters*/
				{
					sleep(1);
						if(customeradded == 0 && available_tellers != 1)
						{
							cout<<"ERROR: Number of available tellers differs from number of empty counters in scheduler().  Thread will now exit."<<endl;
							cout<<"available_tellers = "<<available_tellers<<", counters[0] = "<<counters[0]<<" counters[1] = "<<counters[1]<<", counters[2] = "<<counters[2]<<endl;
							exit(0);
						}
				}	/*end if*/
					
				pthread_mutex_lock(&cout_mutex);
				cout.flush();
				cout<<endl<<"A CUSTOMER in the queue is being serviced by TELLER "<<i<<"!"<<endl;
				pthread_mutex_unlock(&cout_mutex);
				available_tellers--;
				pthread_mutex_unlock(&qcount_mutex);
				pthread_cond_signal(&condition3);	/*Signal to tellers() that a customer is ready to be serviced*/
			}	/*end if*/
					
			if(available_tellers > NO_OF_TELLERS || available_tellers < 0)	/*Error checks available_tellers variable if the variable is not in the range of 0 to NO_OF_TELLERS*/
			{
				cout<<"ERROR: Number of available tellers has taken an improper value in scheduler().  Thread will now exit."<<endl;
				cout<<"available_tellers = "<<available_tellers<<endl;
				exit(0);
			}	/*end else*/
				
			pthread_mutex_lock(&qcount_mutex);

			if((q_count + 1) == MAX_QUEUE_SIZE)	/*Full buffer*/
				pthread_cond_signal(&condition1);	/*Signal to customer_generator() to wake up since the buffer will no longer be full*/

			else
				pthread_mutex_unlock(&qcount_mutex);
		}	/*end if*/
	}	/*end while*/
}	/*end scheduler()*/


void *tellers(void *)	/*Consumes customer elements*/
{
	int consume = 0, i = 0, j = 0, m = 0;

	while(1)
	{
		pthread_mutex_lock(&avail_tell_mutex);

		if(available_tellers > NO_OF_TELLERS || available_tellers < 0)	/*Error checks available_tellers variable for improper values*/
		{
			cout<<"ERROR: Number of available tellers has taken an improper value in tellers().  Thread will now exit."<<endl;
			cout<<"available_tellers = "<<available_tellers<<endl;
			exit(0);
		}	/*end if*/
		
		if(available_tellers == NO_OF_TELLERS)	/*No teller is occupied with a customer*/
		{ 
			pthread_cond_signal(&condition2);	/*Should prevent deadlock when scheduler() waits for teller to become available*/
			pthread_mutex_lock(&avail_tell_mutex);
			pthread_cond_wait(&condition3, &avail_tell_mutex);	/*Wait here until the scheduler has a customer to send to a teller*/	
		}	/*end if*/


		for(m = 0; m < 100 ; m++)	/*This block will allow the tellers to consume multiple customers at a random rate*/
		{
			srand(time(NULL));
			consume = rand() % 5 + 1;

			if(available_tellers == 0 || (available_tellers > 0 && available_tellers < NO_OF_TELLERS))	/*All or some tellers are occupied with a customer*/
			{
				srand(time(NULL));	/*Initializes the random number generator to output random numbers each time rand() is called*/
				sleep(rand() % 5 + 2);	/*Consumes customers at a random rate between 2 to 5 sec)*/

				for(i = 0; i < NO_OF_TELLERS; i++)	/*Loop through the counters[] array for a customer to consume*/
				{
					j = rand() % 3;	  /*rand() will pick a number from 0 to 2 to represent one of the elements in the counters[] array*/
					if(counters[j] == 0)	/*If one of the counters is empty, move to another counter*/	
						continue;

					else
					{
						counters[j] = 0;	/*A random element in the counters[] array will be consumed*/
						available_tellers++;
						pthread_mutex_lock(&cout_mutex);
						cout.flush();
						cout<<endl<<"TELLER " <<j<<" has finished serving a CUSTOMER!  The number of available TELLERS "<<endl<<" has gone from "<<available_tellers - 1<< " to "<<available_tellers<<"."<<endl;
						pthread_mutex_unlock(&cout_mutex);
						pthread_mutex_unlock(&avail_tell_mutex);
						break;
					}	/*end else*/
				}	/*end for*/
			}	/*end if*/


			if(consume < 5)	/*This block will allow the tellers to consume multiple customers at a random rate*/
			{
				pthread_mutex_lock(&avail_tell_mutex);	
				pthread_cond_signal(&condition2); /*Signal scheduler() that a teller is available*/
				break;
			}	/*end if*/

			else
				continue;
		}	/*end for*/	
	}	/*end while*/
}	/*end tellers()*/



int main(void)
{	
	int thread_error1 = 0, thread_error2 = 0, thread_error3 = 0;
	
	pthread_t gen_thread, sched_thread, tell_thread;

	/*Creating threads to execute functions*/
	thread_error1 = pthread_create(&gen_thread, NULL, &customer_generator, NULL);
	thread_error2 = pthread_create(&tell_thread, NULL, &tellers, NULL);
	thread_error3 = pthread_create(&sched_thread, NULL, &scheduler, NULL);
	
	if(thread_error1)
		printf("Error creating thread. ErrNo: %d", thread_error1);
	if(thread_error2)
		printf("Error creating thread. ErrNo: %d", thread_error2);
	if(thread_error3)
		printf("Error creating thread. ErrNo: %d", thread_error3);

	/*Waiting until threads complete before main() resumes.*/
	/*Otherwise main() could exit before threads complete.*/	

	pthread_join(gen_thread, NULL);
	pthread_join(tell_thread, NULL);
	pthread_join(sched_thread, NULL);
}	/*end main*/
