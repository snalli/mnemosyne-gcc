#include <tcutil.h>
#include <tcbdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

int main(int argc, char **argv){
  TCBDB *bdb;
  BDBCUR *cur;
  int ecode;
  char *key, *value;

  /* create the object */
  bdb = tcbdbnew();

  /* open the database */
  if(!tcbdbopen(bdb, "casket.tcb", BDBOWRITER | BDBOCREAT)){
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
  }

  /* store records */
  if(!tcbdbput2(bdb, "foo", "hop") ||
     !tcbdbput2(bdb, "bar", "step") ||
     !tcbdbput2(bdb, "baz", "jump")){
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
  }

  /* retrieve records */
  value = tcbdbget2(bdb, "foo");
  if(value){
    printf("%s\n", value);
    free(value);
  } else {
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "get error: %s\n", tcbdberrmsg(ecode));
  }

  /* traverse records */
  cur = tcbdbcurnew(bdb);
  tcbdbcurfirst(cur);
  while((key = tcbdbcurkey2(cur)) != NULL){
    value = tcbdbcurval2(cur);
    if(value){
      printf("%s:%s\n", key, value);
      free(value);
    }
    free(key);
    tcbdbcurnext(cur);
  }
  tcbdbcurdel(cur);

  /* close the database */
  if(!tcbdbclose(bdb)){
    ecode = tcbdbecode(bdb);
    fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
  }

  /* delete the object */
  tcbdbdel(bdb);

  return 0;
}
