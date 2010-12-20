#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include "paxos.h"
#include "driver.h"

#include "jsl_log.h"


int
main(int argc, char *argv[])
{
  int count = 0;

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  srandom(getpid());

  if(argc < 3){
    fprintf(stderr, "Usage: %s [master:]port [me:]port\n", argv[0]);
    exit(1);
  }

  char *count_env = getenv("RPC_COUNT");
  if(count_env != NULL){
    count = atoi(count_env);
  }

  jsl_set_debug(1);
  driver mydriver(argv[1], argv[2]);
  if (argc>3) {
    for (int i=3; i< argc; i++) {
      mydriver.add_member(argv[i]);
	}
  }	  

  if (strcmp(argv[1], argv[2])==0) {
	  for (int i=0; i<30000; i++) { 
		  mydriver.stress();
	  }	  
  } else {
  	while(1)
    	sleep(1000);
  }

}
