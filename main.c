
#include "ac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define MAXLEN 1502 
#define MAXLINE 10000
#define MAXPATTERNLEN 202

/*
*  Text Data Buffer
*/

unsigned char text[MAXLINE][MAXLEN];   
unsigned char pattern[MAXPATTERNLEN]; 
unsigned int overlap_len = 0;
unsigned int line = 0;
unsigned int *valid_len_array;

unsigned int        thread_num = 8;
pthread_t           *thread_array;
ACSM_STRUCT         *acsm;

void* SearchThread(void* args);

int main (int argc, char **argv) 
{
    int i,j;
    unsigned int total_len = 0;
    struct timeval begtime,endtime;
    FILE *sfd,*pfd;
    char sfilename[20] = "string";
    char pfilename[20] = "pattern";

//=============================================== 
    if (argc < 3)
    {
        fprintf (stderr,"Usage: acsmx stringfile patternfile thread_num...  -nocase\n");
        exit (0);
    }
    strcpy (sfilename, argv[1]);
    sfd = fopen(sfilename,"r");
    if(sfd == NULL)
    {
        fprintf(stderr,"Open file error!\n");
        exit(1);
    }

    strcpy(pfilename,argv[2]);
    pfd = fopen(pfilename,"r");
    if(sfd == NULL)
    {
        fprintf(stderr,"Open file error!\n");
        exit(1);
    }
    thread_num = atoi(argv[3]);

   	acsm = acsmNew (thread_num); 
   
//read patterns    
	i = 0;
    while(fgets(pattern,MAXPATTERNLEN,pfd))
    {
    	int len = strlen(pattern);
    	acsmAddPattern (acsm, pattern, len-1);
		if(len-1 > overlap_len) 
			overlap_len = len - 1;
		i++;
    }
    fclose(pfd);
    printf("\n\nread %d patterns\n\n===============================",i);
    /* Generate GtoTo Table and Fail Table */
    acsmCompile (acsm);
//========================================================= 

    /*read string*/
    for(i = 0;i < MAXLINE;i++)
    {
    	if(!fgets(text[i],MAXLEN,sfd))
    		break;
   		total_len += strlen(text[i]) - 1; //ignore the last char '\n'
    }
    line = i;
    fclose(sfd);
    printf("\n\nreading finished...\n=============================\n\n");
    printf("%d lines\t%dbytes",line,total_len);
    printf("\n\n=============================\n");
    
    gettimeofday(&begtime,0);
    //create multi_thread
    thread_array = calloc(thread_num,sizeof(pthread_t));
	valid_len_array = calloc(thread_num,sizeof(unsigned int));
    for(i = 0;i < thread_num; i++)
	{
		pthread_create(&thread_array[i], NULL, SearchThread, (void*)i);
    }
    //sleep(5);
//=========================================================== 
    int err;
    for(i = 0;i < thread_num;i++)
    {
        err = pthread_join(thread_array[i],NULL);
        if(err != 0)
        {
            printf("can not join with thread %d:%s\n", i,strerror(err));
        }
    }
    gettimeofday(&endtime,0);

    PrintSummary(acsm);
    acsmFree (acsm);

    printf ("\n### AC Match Finished ###\n");
    printf ("\nTime Cost: %lu us\n\n",(endtime.tv_sec - begtime.tv_sec)*1000000 + (endtime.tv_usec - begtime.tv_usec));
    printf ("\n====================================\n\n");
    printf ("Overlap Length:\t%d\n\n",overlap_len);
    printf ("Validation Stage Len:\n\n");
    for(i = 0;i < thread_num;i++)
        printf("rank%d\t%u\n",i,valid_len_array[i]);
    printf ("\n====================================\n\n");
    
    free(valid_len_array);
    return 0;
}

void* SearchThread(void* args)
{
	unsigned int rank = (unsigned int)args;
    int i,state;
    unsigned int index,index_end,len,vlen,linelen;
    int nthread;

    for(i = 0;i < line;i++)
	{
		state = 0;
		vlen = 0;
        len = 0;
        linelen = strlen(text[i]) - 1;
        nthread = thread_num + 1;
        while(len < overlap_len && nthread > 1)
        {
            nthread--;
            if(nthread < thread_num) printf("nthread %d\t len %d\t line %d \tlinelen %d\n",nthread,len,i,linelen);
            len = ((nthread-1)*overlap_len + linelen)/nthread;
        }
        if(rank >= nthread) 
            continue;

		index = (len-overlap_len) * rank;
		if (rank != nthread - 1)
			index_end = index + len;
		else 
			index_end = strlen(text[i]) - 1;
		for( ;index < index_end;index++)
		{
			acsmSearch(acsm,&text[i][index],&state,rank,PrintMatch);
			vlen++;
		}		
        valid_len_array[rank] += vlen;
	}
}
