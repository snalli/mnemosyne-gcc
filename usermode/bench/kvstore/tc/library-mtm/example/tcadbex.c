#include <tcutil.h>
#include <tcadb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

int main(int argc, char **argv){
  TCADB *adb;
  char *key, *value;

  /* create the object */
  adb = tcadbnew();

  /* open the database */
  if(!tcadbopen(adb, "casket.tch")){
    fprintf(stderr, "open error\n");
  }

  /* store records */
  if(!tcadbput2(adb, "foo", "hop") ||
     !tcadbput2(adb, "bar", "step") ||
     !tcadbput2(adb, "baz", "jump")){
    fprintf(stderr, "put error\n");
  }

  /* retrieve records */
  value = tcadbget2(adb, "foo");
  if(value){
    printf("%s\n", value);
    free(value);
  } else {
    fprintf(stderr, "get error\n");
  }

  /* traverse records */
  tcadbiterinit(adb);
  while((key = tcadbiternext2(adb)) != NULL){
    value = tcadbget2(adb, key);
    if(value){
      printf("%s:%s\n", key, value);
      free(value);
    }
    free(key);
  }

  /* close the database */
  if(!tcadbclose(adb)){
    fprintf(stderr, "close error\n");
  }

  /* delete the object */
  tcadbdel(adb);

  return 0;
}
