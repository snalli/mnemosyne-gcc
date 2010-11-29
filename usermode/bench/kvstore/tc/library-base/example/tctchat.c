#include <tcutil.h>
#include <tctdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* function prototypes */
int main(int argc, char **argv);
static void proc(TCTMPL *tmpl, TCMPOOL *mpool);

/* main routine */
int main(int argc, char **argv){
  TCTMPL *tmpl = tctmplnew();
  tctmplload2(tmpl, "tctchat.tmpl");
  TCMPOOL *mpool = tcmpoolnew();
  proc(tmpl, mpool);
  tcmpooldel(mpool);
  tctmpldel(tmpl);
  return 0;
}

/* process each session */
static void proc(TCTMPL *tmpl, TCMPOOL *mpool){
  TCMAP *params = tcmpoolmapnew(mpool);
  char *query = getenv("QUERY_STRING");
  const char *rp = getenv("CONTENT_LENGTH");
  if(rp){
    int clen = tclmin(tcatoi(rp), 1024 * 1024);
    query = tcmpoolmalloc(mpool, clen + 1);
    if(fread(query, 1, clen, stdin) != clen) clen = 0;
    query[clen] = '\0';
  }
  if(query) tcwwwformdecode(query, params);
  const char *type = tcstrskipspc(tcmapget4(params, "type", ""));
  const char *author = tcstrskipspc(tcmapget4(params, "author", ""));
  const char *text = tcstrskipspc(tcmapget4(params, "text", ""));
  const char *search = tcstrskipspc(tcmapget4(params, "search", ""));
  int page = tcatoi(tcmapget4(params, "page", "1"));
  const char *dbpath = tctmplconf(tmpl, "dbpath");
  if(!dbpath) dbpath = "tctchat.tct";
  TCLIST *msgs = tcmpoollistnew(mpool);
  TCTDB *tdb = tcmpoolpush(mpool, tctdbnew(), (void (*)(void *))tctdbdel);
  rp = getenv("REQUEST_METHOD");
  if(rp && !strcmp(rp, "POST") && *author != '\0' && *text != '\0' &&
     strlen(author) <= 32 && strlen(text) <= 1024){
    if(!tctdbopen(tdb, dbpath, TDBOWRITER | TDBOCREAT))
      tclistprintf(msgs, "The database could not be opened (%s).", tctdberrmsg(tctdbecode(tdb)));
    tctdbsetindex(tdb, "", TDBITDECIMAL | TDBITKEEP);
    char pkbuf[64];
    int pksiz = sprintf(pkbuf, "%.0f", tctime() * 1000);
    TCMAP *cols = tcmpoolmapnew(mpool);
    tcmapput2(cols, "a", author);
    tcmapput2(cols, "t", text);
    tctdbtranbegin(tdb);
    if(!tctdbputkeep(tdb, pkbuf, pksiz, cols)) tclistprintf(msgs, "The message is ignored.");
    tctdbtrancommit(tdb);
  } else {
    if(!tctdbopen(tdb, dbpath, TDBOREADER))
      tclistprintf(msgs, "The database could not be opened (%s).", tctdberrmsg(tctdbecode(tdb)));
  }
  TCLIST *logs = tcmpoollistnew(mpool);
  TDBQRY *qry = tcmpoolpush(mpool, tctdbqrynew(tdb), (void (*)(void *))tctdbqrydel);
  if(*search != '\0') tctdbqryaddcond(qry, "t", TDBQCFTSEX, search);
  tctdbqrysetorder(qry, "", TDBQONUMDESC);
  tctdbqrysetlimit(qry, 16, page > 0 ? (page - 1) * 16 : 0);
  TCLIST *res = tcmpoolpushlist(mpool, tctdbqrysearch(qry));
  int rnum = tclistnum(res);
  for(int i = rnum - 1; i >= 0; i--){
    int pksiz;
    const char *pkbuf = tclistval(res, i, &pksiz);
    TCMAP *cols = tcmpoolpushmap(mpool, tctdbget(tdb, pkbuf, pksiz));
    if(cols){
      tcmapprintf(cols, "pk", "%s", pkbuf);
      char date[64];
      tcdatestrwww(tcatoi(pkbuf) / 1000, INT_MAX, date);
      tcmapput2(cols, "d", date);
      const char *astr = tcmapget4(cols, "a", "");
      tcmapprintf(cols, "c", "c%02u", tcgetcrc(astr, strlen(astr)) % 12 + 1);
      tclistpushmap(logs, cols);
    }
  }
  TCMAP *vars = tcmpoolmapnew(mpool);
  if(tclistnum(msgs) > 0) tcmapputlist(vars, "msgs", msgs);
  if(tclistnum(logs) > 0) tcmapputlist(vars, "logs", logs);
  tcmapprintf(vars, "author", "%s", author);
  tcmapprintf(vars, "search", "%s", search);
  if(page > 1) tcmapprintf(vars, "prev", "%d", page - 1);
  if(rnum >= 16 && tctdbrnum(tdb) > page * 16) tcmapprintf(vars, "next", "%d", page + 1);
  char *str = tcmpoolpushptr(mpool, tctmpldump(tmpl, vars));
  printf("Content-Type: %s\r\n", !strcmp(type, "xml") ? "application/xml" :
         !strcmp(type, "xhtml") ? "application/xhtml+xml" : "text/html; charset=UTF-8");
  printf("Cache-Control: no-cache\r\n");
  printf("\r\n");
  fwrite(str, 1, strlen(str), stdout);
}
