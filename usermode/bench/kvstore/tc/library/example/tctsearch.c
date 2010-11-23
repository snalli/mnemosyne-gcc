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
  tctmplload2(tmpl, "tctsearch.tmpl");
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
  if(query) tcwwwformdecode(query, params);
  char *expr = tcmpoolpushptr(mpool, tcstrdup(tcmapget4(params, "expr", "")));
  expr = tcstrsqzspc(tcstrutfnorm(expr, TCUNSPACE));
  int page = tclmax(tcatoi(tcmapget4(params, "page", "1")), 1);
  const char *dbpath = tctmplconf(tmpl, "dbpath");
  if(!dbpath) dbpath = "tctsearch.tct";
  const char *replace = tctmplconf(tmpl, "replace");
  const char *rbef = replace;
  const char *raft = rbef ? strchr(rbef, ' ') : NULL;
  if(raft){
    rbef = tcmpoolpushptr(mpool, tcmemdup(rbef, raft - rbef));
    raft++;
  }
  TCMAP *vars = tcmpoolmapnew(mpool);
  if(*expr != '\0'){
    TCTDB *tdb = tcmpoolpush(mpool, tctdbnew(), (void (*)(void *))tctdbdel);
    tctdbopen(tdb, dbpath, TDBOREADER);
    TDBQRY *qrys[2];
    for(int i = 0; i < 2; i++){
      qrys[i] = tcmpoolpush(mpool, tctdbqrynew(tdb), (void (*)(void *))tctdbqrydel);
      tctdbqrysetorder(qrys[i], "title", TDBQOSTRASC);
    }
    tctdbqryaddcond(qrys[0], "title", TDBQCFTSEX, expr);
    tctdbqryaddcond(qrys[1], "body", TDBQCFTSEX, expr);
    double stime = tctime();
    TCLIST *res = tcmpoolpushlist(mpool, tctdbmetasearch(qrys, 2, TDBMSUNION));
    int rnum = tclistnum(res);
    TCLIST *docs = tcmpoollistnew(mpool);
    for(int i = (page - 1) * 10; i < rnum && i < page * 10; i++){
      int pksiz;
      const char *pkbuf = tclistval(res, i, &pksiz);
      TCMAP *cols = tcmpoolpushmap(mpool, tctdbget(tdb, pkbuf, pksiz));
      if(cols){
        tcmapputkeep2(cols, "title", "(no title)");
        TCXSTR *snip = tcmpoolxstrnew(mpool);
        TCLIST *kwic = tctdbqrykwic(qrys[1], cols, NULL, 30, TCKWMUTAB | TCKWNOOVER | TCKWPULEAD);
        tcmpoolpushlist(mpool, kwic);
        for(int j = 0; j < tclistnum(kwic) && tcxstrsize(snip) < 400; j++){
          if(j > 0) tcxstrcat2(snip, " ... ");
          tcxstrcat2(snip, "<span>");
          tcxstrcat2(snip, tcmpoolpushptr(mpool, tcxmlescape(tclistval2(kwic, j))));
          tcxstrcat2(snip, "</span>");
        }
        tcxstrcat2(snip, " ...");
        char *hlstr = tcregexreplace(tcxstrptr(snip), "\t([^\t]*)\t", "<strong>\\1</strong>");
        tcmapput2(cols, "snippet", tcmpoolpushptr(mpool, hlstr));
        const char *url = tcmapget4(cols, "url", "/");
        url = tcmpoolpushptr(mpool, tcregexreplace(url, "*^[a-z]+://", ""));
        if(rbef) url = tcmpoolpushptr(mpool, tcregexreplace(url, rbef, raft ? raft : ""));
        if(url) tcmapprintf(cols, "url", "%s", url);
        tclistpushmap(docs, cols);
      }
    }
    tcmapprintf(vars, "etime", "%.3f", tctime() - stime);
    tcmapprintf(vars, "min", "%d", (page - 1) * 10 + 1);
    tcmapprintf(vars, "max", "%d", page * 10);
    tcmapprintf(vars, "expr", "%s", expr);
    tcmapprintf(vars, "hitnum", "%d", rnum);
    if(tclistnum(docs) > 0) tcmapputlist(vars, "docs", docs);
    if(page > 1) tcmapprintf(vars, "prev", "%d", page - 1);
    if(rnum > page * 10) tcmapprintf(vars, "next", "%d", page + 1);
  }
  char *str = tcmpoolpushptr(mpool, tctmpldump(tmpl, vars));
  printf("Content-Type: text/html; charset=UTF-8\r\n");
  printf("Cache-Control: no-cache\r\n");
  printf("\r\n");
  fwrite(str, 1, strlen(str), stdout);
}
