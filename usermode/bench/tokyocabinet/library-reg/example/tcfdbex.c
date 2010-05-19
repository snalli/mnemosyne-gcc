#include <tcutil.h>
#include <tcfdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

int main(int argc, char **argv){
  TCFDB *fdb;
  int ecode;
  char *key, *value;

  /* create the object */
  fdb = tcfdbnew();

  /* open the database */
  if(!tcfdbopen(fdb, "casket.tcf", FDBOWRITER | FDBOCREAT)){
    ecode = tcfdbecode(fdb);
    fprintf(stderr, "open error: %s\n", tcfdberrmsg(ecode));
  }

  /* store records */
  if(!tcfdbput3(fdb, "1", "one") ||
     !tcfdbput3(fdb, "12", "twelve") ||
     !tcfdbput3(fdb, "144", "one forty four")){
    ecode = tcfdbecode(fdb);
    fprintf(stderr, "put error: %s\n", tcfdberrmsg(ecode));
  }

  /* retrieve records */
  value = tcfdbget3(fdb, "1");
  if(value){
    printf("%s\n", value);
    free(value);
  } else {
    ecode = tcfdbecode(fdb);
    fprintf(stderr, "get error: %s\n", tcfdberrmsg(ecode));
  }

  /* traverse records */
  tcfdbiterinit(fdb);
  while((key = tcfdbiternext3(fdb)) != NULL){
    value = tcfdbget3(fdb, key);
    if(value){
      printf("%s:%s\n", key, value);
      free(value);
    }
    free(key);
  }

  /* close the database */
  if(!tcfdbclose(fdb)){
    ecode = tcfdbecode(fdb);
    fprintf(stderr, "close error: %s\n", tcfdberrmsg(ecode));
  }

  /* delete the object */
  tcfdbdel(fdb);

  return 0;
}
