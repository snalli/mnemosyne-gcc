#include <tcutil.h>
#include <tctdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

int main(int argc, char **argv){
  TCTDB *tdb;
  int ecode, pksiz, i, rsiz;
  char pkbuf[256];
  const char *rbuf, *name;
  TCMAP *cols;
  TDBQRY *qry;
  TCLIST *res;

  /* create the object */
  tdb = tctdbnew();

  /* open the database */
  if(!tctdbopen(tdb, "casket.tct", TDBOWRITER | TDBOCREAT)){
    ecode = tctdbecode(tdb);
    fprintf(stderr, "open error: %s\n", tctdberrmsg(ecode));
  }

  /* store a record */
  pksiz = sprintf(pkbuf, "%ld", (long)tctdbgenuid(tdb));
  cols = tcmapnew3("name", "mikio", "age", "30", "lang", "ja,en,c", NULL);
  if(!tctdbput(tdb, pkbuf, pksiz, cols)){
    ecode = tctdbecode(tdb);
    fprintf(stderr, "put error: %s\n", tctdberrmsg(ecode));
  }
  tcmapdel(cols);

  /* store a record in a naive way */
  pksiz = sprintf(pkbuf, "12345");
  cols = tcmapnew();
  tcmapput2(cols, "name", "falcon");
  tcmapput2(cols, "age", "31");
  tcmapput2(cols, "lang", "ja");
  if(!tctdbput(tdb, pkbuf, pksiz, cols)){
    ecode = tctdbecode(tdb);
    fprintf(stderr, "put error: %s\n", tctdberrmsg(ecode));
  }
  tcmapdel(cols);

  /* store a record with a TSV string */
  if(!tctdbput3(tdb, "abcde", "name\tjoker\tage\t19\tlang\ten,es")){
    ecode = tctdbecode(tdb);
    fprintf(stderr, "put error: %s\n", tctdberrmsg(ecode));
  }

  /* search for records */
  qry = tctdbqrynew(tdb);
  tctdbqryaddcond(qry, "age", TDBQCNUMGE, "20");
  tctdbqryaddcond(qry, "lang", TDBQCSTROR, "ja,en");
  tctdbqrysetorder(qry, "name", TDBQOSTRASC);
  tctdbqrysetlimit(qry, 10, 0);
  res = tctdbqrysearch(qry);
  for(i = 0; i < tclistnum(res); i++){
    rbuf = tclistval(res, i, &rsiz);
    cols = tctdbget(tdb, rbuf, rsiz);
    if(cols){
      printf("%s", rbuf);
      tcmapiterinit(cols);
      while((name = tcmapiternext2(cols)) != NULL){
        printf("\t%s\t%s", name, tcmapget2(cols, name));
      }
      printf("\n");
      tcmapdel(cols);
    }
  }
  tclistdel(res);
  tctdbqrydel(qry);

  /* close the database */
  if(!tctdbclose(tdb)){
    ecode = tctdbecode(tdb);
    fprintf(stderr, "close error: %s\n", tctdberrmsg(ecode));
  }

  /* delete the object */
  tctdbdel(tdb);

  return 0;
}
