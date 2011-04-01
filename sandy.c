#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <curses.h>

/* Defines */
#ifndef PIPESIZ /* This is POSIX magic */
#define PIPESIZ 4096
#endif /*PIPESIZ*/

#ifndef PATHSIZ /* Max path length */
#define PATHSIZ 1024
#endif /*PATHSIZ*/

#define LENGTH(x)     (sizeof x / sizeof x[0])
#define LINSIZ        128
#define ISWORDBRK(ch) (ch==' ' || ch=='\t' || ch=='\0')
#define UTF8LEN(ch)   ((unsigned char)ch>=0xFC ? 6 : \
			((unsigned char)ch>=0xF8 ? 5 : \
			((unsigned char)ch>=0xF0 ? 4 : \
			((unsigned char)ch>=0xE0 ? 3 : \
			((unsigned char)ch>=0xC0 ? 2 : 1)))))
#define ISASCII(ch)   ((unsigned char)ch < 0x80)
#define ISCTRL(ch)    (((unsigned char)ch < 0x20) || (ch == 0x7F))
#define ISFILL(ch)    (isutf8 && !ISASCII(ch) && (unsigned char)ch<=0xBF)
#define VLEN(ch,col)  (ch==0x09 ? tabstop-(col%tabstop) : (ISCTRL(ch) ? 2: (ISFILL(ch) ? 0: 1)))
#define FIXNEXT(pos)  while(isutf8 && ISFILL(pos.l->c[pos.o]) && ++pos.o < pos.l->len)
#define FIXPREV(pos)  while(isutf8 && ISFILL(pos.l->c[pos.o]) && --pos.o > 0)
#define LINES2        (lines - (titlewin==NULL?0:1))

/* Typedefs */
typedef struct Line Line;
struct Line {
	char *c;
	size_t len;
	size_t vlen;
	int mul;
	bool dirty;
	Line *next;
	Line *prev;
};

typedef struct {
	Line *l;
	int o;
} Filepos;

typedef union {
	int i;
	const void *v;
	Filepos (*m)(Filepos);
} Arg;

typedef struct {
	int  keyv[6];
	bool (*test[4])(void);
	void (*func)(const Arg *arg);
	const Arg arg;
} Key;

typedef struct {
	regex_t *re;
	const char *re_text;
	bool (*test[2])(void);
	void (*func)(const Arg *arg);
} Command;

#define SYN_COLORS 8
typedef struct {
	char    *name;
	regex_t *file_re;
	char    *file_re_text;
	regex_t *re[SYN_COLORS];
	char    *re_text[SYN_COLORS];
} Syntax;

typedef struct Undo Undo;
struct Undo {
	char flags;
	unsigned long startl, endl;
	int starto, endo;
	char *str;
	Undo *prev;
};

/* ENUMS */
/* Colors */
enum { DefFG, CurFG, SelFG, SpcFG, CtrlFG, Syn0FG, Syn1FG, Syn2FG, Syn3FG, Syn4FG, Syn5FG, Syn6FG, Syn7FG, LastFG, };
enum { DefBG, CurBG, SelBG, /* Warning: BGs MUST have a matching FG */     LastBG, };

/* f_extsel arg->i */
enum { ExtDefault, ExtWord, ExtLines, ExtAll, };

/* Environment variables */
enum { EnvFind, EnvPipe, EnvLine, EnvOffset, EnvFile, EnvSyntax, EnvFifo, EnvLast, };

enum { /* To use in statusflags */
	S_Running    = 1,
	S_Readonly   = 1<<1,
	S_InsEsc     = 1<<2,
	S_CaseIns    = 1<<3,
	S_Modified   = 1<<4,
	S_Selecting  = 1<<5,
	S_DirtyScr   = 1<<6,
	S_DirtyDown  = 1<<7,
	S_NeedResize = 1<<8,
};

enum { /* To use in Undo.flags */
	UndoIns  = 1,
	UndoMore = 1<<1,
	RedoMore = 1<<2,
};

/* Constants */
static const char *envs[EnvLast] = {
	[EnvFind]   = "SANDY_FIND",
	[EnvPipe]   = "SANDY_PIPE",
	[EnvLine]   = "SANDY_LINE",
	[EnvOffset] = "SANDY_OFFSET",
	[EnvFile]   = "SANDY_FILE",
	[EnvSyntax] = "SANDY_SYNTAX",
	[EnvFifo]   = "SANDY_FIFO",
};

/* Variables */
static Line     *fstline;
static Line     *lstline;
static Line     *scrline; /* First line seen in window */
static Filepos   fsel;    /* Selection mark on file */
static Filepos   fcur;    /* Insert position on file */
static Filepos   fmrk = { NULL, 0 }; /* Mark */
static Syntax   *syntx = NULL;
static int       sel_re = 0; /* We keep 2 REs so regexec does not segfault */
static regex_t  *find_res[2];
static char     *fifopath = NULL;
static char     *filename = NULL;
static char     *title    = NULL;
static char     *tmptitle = NULL;
static char     *tsl_str  = NULL;
static char     *fsl_str  = NULL;
static WINDOW   *titlewin = NULL;
static WINDOW   *textwin  = NULL;
static Undo     *undos;
static Undo     *redos;
static int       textattrs[LastFG][LastBG];
static int       savestep=0;
static int       fifofd;
static int       statusflags=S_Running;
static int       cols, lines; /* To use instead of COLS and LINES */
static mmask_t   defmmask=ALL_MOUSE_EVENTS;

/* Functions */
/* f_* functions can be linked to an action or keybinding */
static void f_center(const Arg*);
static void f_delete(const Arg*);
static void f_extsel(const Arg*);
static void f_findbw(const Arg*);
static void f_findfw(const Arg*);
static void f_insert(const Arg*);
static void f_line(const Arg*);
static void f_mark(const Arg*);
static void f_move(const Arg*);
static void f_offset(const Arg*);
static void f_pipe(const Arg*);
static void f_pipelines(const Arg*);
static void f_pipero(const Arg*);
static void f_save(const Arg*);
static void f_select(const Arg*);
static void f_spawn(const Arg*);
static void f_syntax(const Arg *arg);
static void f_title(const Arg *arg);
static void f_toggle(const Arg *arg);
static void f_undo(const Arg*);

/* i_* funcions are called from inside the main code only */
static Filepos        i_addtext(char*, Filepos);
static void           i_addundo(bool, Filepos, Filepos, char*);
static void           i_advpos(Filepos *pos, int o);
static void           i_calcvlen(Line *l);
static void           i_cleanup(int);
static void           i_deltext(Filepos, Filepos);
static void           i_die(char *str);
static void           i_dirtyrange(Line*, Line*);
static void           i_edit(void);
static void           i_find(bool);
static char          *i_gettext(Filepos, Filepos);
static void           i_killundos(Undo**);
static Line          *i_lineat(unsigned long);
static unsigned long  i_lineno(Line*);
static void           i_mouse(void);
static void           i_pipetext(const char*);
static void           i_readfifo(void);
static void           i_readfile(char*);
static void           i_resize(void);
static Filepos        i_scrtofpos(int, int);
static bool           i_setfindterm(char*);
static void           i_setup(void);
static void           i_sigwinch(int);
static void           i_sortpos(Filepos*, Filepos*);
static char          *i_strdup(const char*);
static void           i_termwininit(void);
static void           i_update(void);
static void           i_usage(void);
static bool           i_writefile(void);

/* t_* functions to know whether to process an action or keybinding */
static bool t_bol(void);
static bool t_eol(void);
static bool t_mod(void);
static bool t_rw(void);
static bool t_redo(void);
static bool t_sel(void);
static bool t_undo(void);

/* m_ functions represent a cursor movement and can be passed in an Arg */
static Filepos m_bof(Filepos);
static Filepos m_bol(Filepos);
static Filepos m_eof(Filepos);
static Filepos m_eol(Filepos);
static Filepos m_nextchar(Filepos);
static Filepos m_prevchar(Filepos);
static Filepos m_nextword(Filepos);
static Filepos m_prevword(Filepos);
static Filepos m_nextline(Filepos);
static Filepos m_prevline(Filepos);
static Filepos m_nextscr(Filepos);
static Filepos m_prevscr(Filepos);
static Filepos m_stay(Filepos);
static Filepos m_tomark(Filepos);
static Filepos m_tosel(Filepos);

#include "config.h"

void
f_center(const Arg *arg) {
	int i=LINES2/2;
	
	scrline=fcur.l;
	while((i -= 1 + scrline->vlen/cols) > 0 && scrline->prev) scrline=scrline->prev;
	statusflags|=S_DirtyScr;
}

void /* Your responsibility: call only if t_rw() */
f_delete(const Arg *arg) {
	char *s;
	Filepos pos0=fcur, pos1=arg->m(fcur);

#ifdef HOOK_PRE_DELETE
	HOOK_PRE_DELETE;
#endif
	i_sortpos(&pos0, &pos1);
	s=i_gettext(pos0, pos1);
	i_addundo(FALSE, pos0, pos1, s);
	i_deltext(pos0, pos1);
	fcur=fsel=pos0;
	statusflags|=S_Modified;
	statusflags&=~S_Selecting;
}

void
f_extsel(const Arg *arg) {
	i_sortpos(&fsel, &fcur);
	switch(arg->i) {
	case ExtWord:
		if(fsel.o > 0 && !ISWORDBRK(fsel.l->c[fsel.o-1])) fsel=m_prevword(fsel);
		if(!ISWORDBRK(fcur.l->c[fcur.o])) fcur=m_nextword(fcur);
	break;
	case ExtLines:
		fsel.o=0, fcur.o=fcur.l->len;
	break;
	case ExtAll:
		fsel.l=fstline, fcur.l=lstline;
		f_extsel(&(const Arg){.i = ExtLines});
		statusflags|=S_DirtyScr;
	break;
	case ExtDefault:
	default:
		if(statusflags & S_Selecting) {
			if(fsel.o == 0 && fcur.o == fcur.l->len) f_extsel(&(const Arg){.i = ExtAll});
			else f_extsel(&(const Arg){.i = ExtLines});
		} else {
			if(!t_sel()) f_extsel(&(const Arg){.i = ExtWord});
		}
	}
	statusflags|=S_Selecting;
}

void
f_findbw(const Arg *arg) {
	if(i_setfindterm((char*)arg->v)) i_find(FALSE);
}

void
f_findfw(const Arg *arg) {
	if(i_setfindterm((char*)arg->v)) i_find(TRUE);
}

void /* Your responsibility: call only if t_rw() */
f_insert(const Arg *arg) {
	bool killsel;

	if((killsel=t_sel())) {
		f_delete(&(const Arg) { .m = m_tosel });
		undos->flags^=RedoMore;
	}
	fsel=i_addtext((char*)arg->v, fcur);
	i_addundo(TRUE, fcur, fsel, i_strdup(arg->v));
	if(killsel) undos->flags^=UndoMore;
	fcur=fsel;
	statusflags|=S_Modified;
	statusflags&=~S_Selecting;
}

void
f_line(const Arg *arg) {
	long int l;
	
	l=atoi(arg->v);
	fcur.l=i_lineat(l);
	if(fcur.o>fcur.l->len) fcur.o=fcur.l->len;
	FIXNEXT(fcur);
	if(! (statusflags & S_Selecting)) fsel=fcur;
}

void
f_mark(const Arg *arg) {
	fmrk=fcur;
}

void
f_move(const Arg *arg) {
	fcur=arg->m(fcur);
	if(! (statusflags & S_Selecting)) fsel=fcur;
}

void
f_offset(const Arg *arg) {
	fcur.o=atoi(arg->v);
	if(fcur.o>fcur.l->len) fcur.o=fcur.l->len;
	FIXNEXT(fcur);
	if(! (statusflags & S_Selecting)) fsel=fcur;
}

void /* Your responsibility: call only if t_rw() */
f_pipe(const Arg *arg) {
	i_pipetext(arg->v);
	statusflags|=S_Modified;
}

void /* Your responsibility: call only if t_rw() */
f_pipelines(const Arg *arg) {
	f_extsel(&(const Arg){ .i = ExtLines });
	i_pipetext(arg->v);
	statusflags|=S_Modified;
}

void
f_pipero(const Arg *arg) {
	char oldsf=statusflags;

	statusflags|=S_Readonly;
	i_pipetext(arg->v);
	statusflags=oldsf&(~S_Selecting);
}

void /* Your responsibility: call only if t_mod() */
f_save(const Arg *arg) {
	Undo *u;

	statusflags&=~S_Selecting;
	if(arg && arg->v) {
		free(filename);
		filename=i_strdup(arg->v);
		setenv(envs[EnvFile], filename, 1);
	} else if(filename==NULL) { 
		unsetenv(envs[EnvFile]);
#ifdef HOOK_SAVE_NO_FILE
		HOOK_SAVE_NO_FILE;
#endif
		return;
	}

	if(i_writefile()) {
		statusflags^=S_Modified;
		for(savestep=0,u=undos; u; u=u->prev, savestep++);
	}
}

void
f_select(const Arg *arg) {
	fsel=fcur;
	fcur=arg->m(fcur);
	if(t_sel())
		statusflags|=S_Selecting;
	else
		statusflags&=~S_Selecting;
}

void 
f_spawn(const Arg *arg) {
	int pid=-1;
	statusflags&=~S_Selecting;
	reset_shell_mode();
	if((pid=fork()) == 0) {
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "sandy: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(0);
	} else if(pid>0) waitpid(pid, NULL, 0);
	reset_prog_mode();
	if(titlewin) redrawwin(titlewin);
	redrawwin(textwin);
}

void
f_syntax(const Arg *arg) {
	int i, j;

	statusflags&=~S_Selecting;
	for(i=0; i<LENGTH(syntaxes); i++)
		if((arg && arg->v) ? !strcmp(arg->v, syntaxes[i].name)
				   : !regexec(syntaxes[i].file_re, filename, 1, NULL, 0)) {
			for(j=0; j<SYN_COLORS; j++) {
				if(syntx && syntx->re[j]) regfree(syntx->re[j]);
				if(regcomp(syntaxes[i].re[j], syntaxes[i].re_text[j], REG_EXTENDED|REG_NEWLINE)) i_die("Faulty regex.\n");
			}
			syntx=&syntaxes[i];
			setenv(envs[EnvSyntax], syntx->name, 1);
			return;;
		}
	for(i=0; i<SYN_COLORS; i++)
		if(syntx && syntx->re[i]) regfree(syntx->re[i]);
	syntx=NULL;
	setenv(envs[EnvSyntax], "none", 1);
}

void
f_title(const Arg *arg) {
	tmptitle=(char*)arg->v;
	statusflags&=~S_Selecting;
}

void /* Careful with this one! */
f_toggle(const Arg *arg) {
	statusflags^=(char)arg->i;
 
        /* Specific operations for some toggles */
	switch(arg->i) {
	case S_CaseIns: /* Re-compile regex with/without REG_ICASE */
		i_setfindterm(getenv(envs[EnvFind]));
	break;
	}
}

void /* Your responsibility: call only if t_undo() / t_redo() */
f_undo(const Arg *arg) {
	Filepos start, end;
	const bool r=(arg->i < 0);
	Undo *u;
	int   n;

	statusflags&=~S_Selecting;
	u=(r?redos:undos);
	fsel.o=u->starto, fsel.l=i_lineat(u->startl);
	fcur=fsel;
	while(u) {
		start.o=u->starto, start.l=i_lineat(u->startl);
		end.o=u->endo,     end.l=i_lineat(u->endl);
		if(r ^ (u->flags & UndoIns)) {
                        i_sortpos(&start, &end);
                        i_deltext(start, end);
                        fcur=fsel=start;
                } else
                        fcur=i_addtext(u->str, fcur);
                if(r)
                        redos=u->prev, u->prev=undos, undos=u;
                else
                        undos=u->prev, u->prev=redos, redos=u;
                if (!(u->flags & (r?RedoMore:UndoMore))) break;
		u=(r?redos:undos);
	}

	for(n=0, u=undos; u; u=u->prev, n++);
	if(n==savestep) /* True if we saved at this undo point */
		statusflags^=S_Modified;
	else
		statusflags|=S_Modified;
}

void
i_addundo(bool ins, Filepos start, Filepos end, char *s) {
	Undo *u;

	if(redos) i_killundos(&redos);
	if ((u=(Undo*)calloc(1, sizeof(Undo))) == NULL)
		i_die("Can't malloc.\n");
	u->flags  = (ins?UndoIns:0);
	u->startl = i_lineno(start.l);
	u->endl   = i_lineno(end.l);
	u->starto = start.o;
	u->endo   = end.o;
	u->str    = s;
	u->prev   = undos;
	undos     = u;
}

Filepos
i_addtext(char *buf, Filepos pos){
	size_t i=0, il=0;
	char c;
	Line *l=pos.l, *lnew=NULL;
	int   o=pos.o, extralines;
	Filepos f;

	extralines=l->vlen/cols;
	for(c=buf[0]; c!='\0'; c=buf[++i]){
		if(c=='\n' || c=='\r') { /* New line */
			if(((lnew=(Line*)malloc(sizeof(Line))) == NULL) ||
				((lnew->c=calloc(1, LINSIZ)) == NULL))
				i_die("Can't malloc.\n");
			lnew->c[0]='\0';
			lnew->dirty=l->dirty=TRUE;
			lnew->len = lnew->vlen = 0; lnew->mul = 1;
			lnew->next = l->next; lnew->prev = l;
			if(l->next) l->next->prev=lnew;
			else lstline=lnew;
			l->next = lnew; l = lnew;
			if(o+il<l->prev->len) { /* \n in the middle of a line */
				f.l=l; f.o=0;
				i_addtext(&(l->prev->c[o+il]), f);
				l->prev->len=o+il; l->prev->c[o+il]='\0';
			}
			i_calcvlen(l->prev);
			o=il=0;
		} else { /* Regular char */
			if(2+(l->len) >= LINSIZ*(l->mul))
				l->c = (char*)realloc(l->c,  LINSIZ*(++(l->mul)));
			memmove(l->c+il+o+1, l->c+il+o, (1 + l->len - (il+o)));
			l->c[il+o]=c;
			l->dirty=TRUE;
			if(il+o >= (l->len)++) l->c[il+o+1]='\0';
			il++;
		}
	}
	i_calcvlen(l);
	f.l=l; f.o=il+o;
	if(lnew!=NULL || extralines != pos.l->vlen/cols) statusflags|=S_DirtyDown;
	return f;
}

void
i_advpos(Filepos *pos, int o) {
	int toeol;

	toeol=pos->l->len - pos->o;
	if(o<=toeol)
		o+=pos->o;
	else while(o>toeol && pos->l->next) {
		pos->l = pos->l->next;
		o-=(1+toeol);
		toeol=pos->l->len;
	}
	pos->o=o;
	FIXNEXT((*pos)); /* This should not be needed here */
}

void
i_calcvlen(Line *l) {
	int i;

	l->vlen=0;
	for(i=0; i<l->len; i++)
		l->vlen+=VLEN(l->c[i], l->vlen);
}

void
i_cleanup(int sig) {
	int i;

	i_killundos(&undos);
	i_killundos(&redos);
	close(fifofd);
	unlink(fifopath);
	free(fifopath);
	free(filename);
	free(title);
	for(i=0; i<LENGTH(cmds); i++)
		regfree(cmds[i].re);
	for(i=0; i<LENGTH(syntaxes); i++)
		regfree(syntaxes[i].file_re);
	if(syntx) for(i=0; i<SYN_COLORS; i++)
			regfree(syntx->re[i]);
	regfree(find_res[0]);
	regfree(find_res[1]);
	endwin();
	exit(sig>0?1:0);
}

void
i_die(char *str) {
	fputs(str, stderr);
	exit(1);
}

void
i_dirtyrange(Line *l0, Line *l1) {
	Filepos pos0, pos1;
	pos0.l=l0, pos1.l=l1, pos0.o=pos1.o=0;
	i_sortpos(&pos0, &pos1);
	for(;pos0.l;pos0.l=pos0.l->next) pos0.l->dirty=TRUE;
}

void /* pos0 and pos1 MUST be in order, fcur and fsel integrity is NOT assured after deletion */
i_deltext(Filepos pos0, Filepos pos1) {
	Line *ldel=NULL;
	int extralines=0;

	if(pos0.l==pos1.l) {
		extralines=(pos0.l->vlen)/cols;
		memmove(pos0.l->c+pos0.o, pos0.l->c+pos1.o, (pos0.l->len - pos1.o));
		pos0.l->dirty=TRUE;
		pos0.l->len-=(pos1.o-pos0.o);
		pos0.l->c[pos0.l->len]='\0';
		i_calcvlen(pos0.l);
	} else {
		pos0.l->len=pos0.o; pos0.l->c[pos0.l->len]='\0';
		/* i_calcvlen is unneeded here, because we call i_addtext later */
		while(pos1.l!=ldel) {
			if(pos1.l==pos0.l->next)
				i_addtext(&(pos0.l->next->c[pos1.o]), pos0);
			if(pos0.l->next->next) pos0.l->next->next->prev=pos0.l;
			ldel=pos0.l->next;
			pos0.l->next=pos0.l->next->next;
			if(scrline == ldel) scrline=ldel->prev;
			if(lstline == ldel) lstline=ldel->prev;
			free(ldel->c);
			free(ldel);
		}
	}
	if(ldel!=NULL || extralines != pos0.l->vlen/cols) statusflags|=S_DirtyDown;
}

void
i_edit(void) {
	int ch, i;
	char c[7];
	fd_set fds;
	Filepos oldsel, oldcur;

	oldsel.l=oldcur.l=fstline;
	oldsel.o=oldcur.o=0;

	edit_top:
	while(statusflags & S_Running) {
		if(fsel.l != oldsel.l) i_dirtyrange(oldsel.l, fsel.l);
		else if(fsel.o != oldsel.o) fsel.l->dirty=TRUE;
		if(fcur.l != oldcur.l) i_dirtyrange(oldcur.l, fcur.l);
		else if(fcur.o != oldcur.o) fcur.l->dirty=TRUE;
		oldsel=fsel, oldcur=fcur;
		i_update();

#ifdef HOOK_SELECT_ALL
		if(fsel.l != fcur.l || fsel.o != fcur.o) {
			HOOK_SELECT_ALL;
		}
#endif

		FD_ZERO(&fds); FD_SET(0, &fds); FD_SET(fifofd, &fds);
		signal(SIGWINCH, i_sigwinch);
		if(select(FD_SETSIZE, &fds, NULL, NULL, NULL) == -1 && errno==EINTR) {
			signal(SIGWINCH, SIG_IGN);
			continue;
		}
		signal(SIGWINCH, SIG_IGN);
		if(FD_ISSET(fifofd, &fds)) i_readfifo();
		if(!FD_ISSET(0,     &fds)) continue;
		if((ch=wgetch(textwin)) == ERR) {
			tmptitle="ERR";
			continue;
		}

		/* NCurses special chars are processed first to avoid UTF-8 collision */
		if(ch>=KEY_MIN) {
			/* These are not really chars */
			if(ch==KEY_MOUSE) {
				i_mouse();
			} else for(i=0; i<LENGTH(curskeys); i++) {
				if(ch == curskeys[i].keyv[0] /* NCurses special chars come as a single 'int' */
					&& ( curskeys[i].test[0] == NULL || curskeys[i].test[0]() )
					&& ( curskeys[i].test[1] == NULL || curskeys[i].test[1]() )
					&& ( curskeys[i].test[2] == NULL || curskeys[i].test[2]() )
					&& ( curskeys[i].test[3] == NULL || curskeys[i].test[3]() ) ) {
					curskeys[i].func(&(curskeys[i].arg));
					break;
				}
			}
			//tmptitle=keyname(ch);
			goto edit_top;
		}

		/* Mundane characters are processed later */
		c[0]=(char)ch;
		if(c[0]==0x1B || (isutf8 && !ISASCII(c[0]))) {
			/* Multi-byte char or escape sequence */
			wtimeout(textwin, 1);
			for(i=1; i<(c[0]==0x1B?6:UTF8LEN(c[0])); i++)
				if((c[i]=wgetch(textwin)) == ERR) break;
			for(;i<7;i++)
				c[i]=0x00;
			wtimeout(textwin, 0);
		} else c[1]=c[2]=c[3]=c[4]=c[5]=c[6]=0x00;

		if(!(statusflags&S_InsEsc) && ISCTRL(c[0])) {
			for(i=0; i<LENGTH(stdkeys); i++) {
				if(c[0] == stdkeys[i].keyv[0] && c[1] == stdkeys[i].keyv[1]
				&& c[2] == stdkeys[i].keyv[2] && c[3] == stdkeys[i].keyv[3]
				&& c[4] == stdkeys[i].keyv[4] && c[5] == stdkeys[i].keyv[5]
					&& ( stdkeys[i].test[0] == NULL || stdkeys[i].test[0]() )
					&& ( stdkeys[i].test[1] == NULL || stdkeys[i].test[1]() )
					&& ( stdkeys[i].test[2] == NULL || stdkeys[i].test[2]() )
					&& ( stdkeys[i].test[3] == NULL || stdkeys[i].test[3]() ) ) {
					stdkeys[i].func(&(stdkeys[i].arg));
					break;
				}
			}
			goto edit_top;
		}
		statusflags&=~(S_InsEsc);
		f_insert(&(const Arg){ .v = c });
	}
}

void
i_find(bool fw) {
	char *s;
	int wp, _so, _eo, status;
	Filepos start, end;
	regmatch_t result[1];

	start.l=fstline; start.o=0;
	end.l=lstline;   end.o=lstline->len;
	i_sortpos(&fsel, &fcur);

	for (wp=0; wp<2 ; free(s), wp++) {
		if (wp)      s = i_gettext(start, end);
		else if (fw) s = i_gettext(fcur,  end);
		else         s = i_gettext(start, fsel);

		if ((status=regexec(find_res[sel_re], s, 1, result, (fw?(fcur.o==0 ? 0 : REG_NOTBOL):
							fsel.o==fsel.l->len?0:REG_NOTEOL))) == 0) {
			if(wp || !fw)
				fcur=start;
			fsel=fcur;
			_so=result[0].rm_so;
			_eo=result[0].rm_eo;
			if (!fw) {
				char *subs;
				subs = &s[_eo];
				while (!regexec(find_res[sel_re], subs, 1, result, REG_NOTBOL) && result[0].rm_eo) {
					/* This is blatantly over-simplified: do not try to match an
					   empty string backwards as it will match the first hit on the file. */
					_so=_eo+result[0].rm_so;
					_eo+=result[0].rm_eo;
					subs = &s[_eo];
				}
			}
			i_advpos(&fsel, _so);
			i_advpos(&fcur, _eo);
			f_mark(NULL);
			wp++;
		}
	}
}

char* /* pos0 and pos1 MUST be in order; you MUST free the returned string after use */
i_gettext(Filepos pos0, Filepos pos1) {
	Line *l;
	unsigned long long i=1;
	char *buf;

	for(l=pos0.l; l!=pos1.l->next; l=l->next)
		i+=1 + (l==pos1.l?pos1.o:l->len) - (l==pos0.l?pos0.o:0);
	buf=calloc(1,i);
	for(l=pos0.l, i=0; l!=pos1.l->next; l=l->next) {
		memcpy(buf+i, l->c+(l==pos0.l?pos0.o:0), (l==pos1.l?pos1.o:l->len) - (l==pos0.l?pos0.o:0));
		i+=(l==pos1.l?pos1.o:l->len) - (l==pos0.l?pos0.o:0);
		if(l!=pos1.l) buf[i++]='\n';
	}
	return buf;
}

void
i_killundos(Undo **list) {
	Undo *u;

	for(; *list;) {
		u=(*list)->prev;
		if((*list)->str) free((*list)->str);
		free(*list); *list=u;
	}
}

Line*
i_lineat(unsigned long il) {
	unsigned long i;
	Line *l;
	for(i=1, l=fstline; i!=il && l && l->next; i++) l=l->next;
	return l;
}

unsigned long
i_lineno(Line *l0) {
	unsigned long i;
	Line *l;
	for(i=1, l=fstline; l!=l0; l=l->next) i++;
	return i;
}

void
i_mouse(void) {
	static MEVENT ev;

	if(getmouse(&ev) == ERR) return;
	if(!wmouse_trafo(textwin, &ev.y, &ev.x, FALSE)) return;
	if(ev.bstate & (REPORT_MOUSE_POSITION|BUTTON1_RELEASED)) {
		fcur=i_scrtofpos(ev.x, ev.y); /* Select text */
		if(ev.bstate & BUTTON1_RELEASED) mousemask(defmmask,NULL);
#ifdef HOOK_SELECT_MOUSE
		HOOK_SELECT_MOUSE;
#endif
	} else if(ev.bstate & (BUTTON1_CLICKED|BUTTON1_PRESSED)) {
		fsel=fcur=i_scrtofpos(ev.x, ev.y); /* Move cursor */
		if(ev.bstate & BUTTON1_PRESSED) mousemask(defmmask|REPORT_MOUSE_POSITION,NULL);
	} else if(ev.bstate & (BUTTON1_DOUBLE_CLICKED)) {
		fsel=fcur=i_scrtofpos(ev.x, ev.y); /* Select word */
		f_extsel(&(const Arg){.i = ExtWord});
	} else if(ev.bstate & (BUTTON1_TRIPLE_CLICKED)) {
		fsel=fcur=i_scrtofpos(ev.x, ev.y); /* Select line */
		f_extsel(&(const Arg){.i = ExtLines});
	}
}

void
i_pipetext(const char *cmd) {
	struct timeval tv;
	char *s = NULL;
	int pin[2], pout[2], pid=-1, nr=1, nw, written, iw=0, closed=0, exstatus;
	char *buf = NULL;
	Filepos auxp;
	fd_set fdI, fdO;

	statusflags&=~S_Selecting;
	if(!cmd || cmd[0] == '\0') return;
	setenv(envs[EnvPipe], cmd, 1);
	if (pipe(pin) == -1) return;
	if (pipe(pout) == -1) {
		close(pin[0]); close(pin[1]);
		return;
	}

	i_sortpos(&fsel, &fcur);

	/* Things I will undo or free at the end of this function */
	s = i_gettext(fsel, fcur);

	fflush(stdout);
	if((pid = fork()) == 0) {
		dup2(pin[0], 0);
		dup2(pout[1], 1);
		close(pin[0]);  close(pin[1]);
		close(pout[0]); close(pout[1]);
		/* TODO: close stderr? redirect? update screen?? */
		execl("/bin/sh", "sh", "-c", cmd, NULL); /* I actually like it with sh so I can input pipes et al. */
		fprintf(stderr, "sandy: execl sh -c %s", cmd);
		perror(" failed");
		exit(0);
	}

	if (pid > 0) {
		close(pin[0]);
		close(pout[1]);
		if (t_rw()) {
			i_addundo(FALSE, fsel, fcur, i_strdup(s));
			undos->flags^=RedoMore;
			i_deltext(fsel, fcur);
			fcur=fsel;
		}
		fcntl(pin[1],  F_SETFL, O_NONBLOCK);
		fcntl(pout[0], F_SETFL, O_NONBLOCK);
		buf  = calloc(1, PIPESIZ+1);
		FD_ZERO(&fdO); FD_SET(pin[1] , &fdO);
		FD_ZERO(&fdI); FD_SET(pout[0], &fdI);
		tv.tv_sec = 5; tv.tv_usec = 0; nw=s?strlen(s):0;
		while (select(FD_SETSIZE, &fdI, &fdO, NULL, &tv) && (nw > 0 || nr > 0)) {
			fflush(NULL);
			if (FD_ISSET(pout[0], &fdI) && nr>0) {
				nr = read(pout[0], buf, PIPESIZ);
				if (nr>=0) buf[nr]='\0';
				else break; /* ...not seen it yet */
				if (nr && t_rw()) {
					auxp=i_addtext(buf, fcur);
					i_addundo(TRUE, fcur, auxp, i_strdup(buf));
					undos->flags^=RedoMore|UndoMore;
					fcur=auxp;
				}
			} else  if (nr>0) FD_SET(pout[0], &fdI);
				else FD_ZERO(&fdI);
			if (FD_ISSET(pin[1] , &fdO) && nw>0) {
				written=write(pin[1], &(s[iw]), (nw<PIPESIZ?nw:PIPESIZ));
				if (written < 0) break; /* broken pipe? */
				iw+=(nw<PIPESIZ?nw:PIPESIZ);
				nw-=written;
			} else  if (nw>0) FD_SET(pin[1], &fdO);
				else {
					if(!closed++)
						close(pin[1]);
					FD_ZERO(&fdO);
				}
		}
		if (t_rw())
			undos->flags^=RedoMore;
		free(buf);
		if(!closed) close(pin[1]);
		waitpid(pid, &exstatus, 0); /* We don't want to close the pipe too soon */
		close(pout[0]);
	} else {
		close(pin[0]);  close(pin[1]);
		close(pout[0]); close(pout[1]);
	}

	/* Things I want back to normal */
	if (s) free(s);
}

void
i_readfifo(void) {
	char *buf, *tofree;
	regmatch_t result[2];
	int i;

	if((buf=tofree=calloc(1, PIPESIZ+1)) == NULL) i_die("Can't malloc.\n");
	i=read(fifofd, buf, PIPESIZ);
	buf[i]='\0';
	buf=strtok(buf, "\n");
	while(buf != NULL) {
		for(i=0; i<LENGTH(cmds); i++)
			if(!regexec(cmds[i].re, buf, 2, result, 0)
				&& (cmds[i].test[0] == NULL || cmds[i].test[0]())
				&& (cmds[i].test[1] == NULL || cmds[i].test[1]())) {
				cmds[i].func(&(const Arg){ .v = (buf+result[1].rm_so)});
				break;
			}
		buf=strtok(NULL, "\n");
	}
	free(tofree);

	/* Kludge: we close and reopen to circumvent a bug? 
	           if we don't do this, fifofd seems to be always ready to select()
		   also, I was unable to mark fifofd as blocking after opening */
	close(fifofd);
	if((fifofd = open(fifopath, O_RDONLY | O_NONBLOCK)) == -1) i_die("Can't open FIFO for reading.\n");;
}

void
i_readfile(char *fname) {
	int fd;
	ssize_t n;
	char *buf = NULL;

	if(!strcmp(fname, "-")) {
		fd=0;
		reset_shell_mode();
	} else {
		filename=i_strdup(fname);
		setenv(envs[EnvFile], filename, 1);
		f_syntax(NULL); /* If we are here, try to guess a syntax */
		if((fd=open(filename, O_RDONLY)) == -1){
			tmptitle="WARNING! Can't read file!!!";
			return;
		}
	}

	if((buf=calloc(1, BUFSIZ+1)) == NULL) i_die("Can't malloc.\n");
	while((n=read(fd, buf, BUFSIZ)) > 0) {
		buf[n]='\0';
		fcur=i_addtext(buf, fcur);
	}
	if(fd > 0) close(fd);
	else reset_prog_mode();
	free(buf);
	fcur.l=fstline;
	fcur.o=0;
	fsel=fcur;
}

void /* handle term resize, ugly */
i_resize(void) {
	const char *tty;
	int fd, result;
	struct winsize ws;

	if((tty=ttyname(0)) == NULL) return;
	fd = open(tty, O_RDWR);
	if(fd == -1) return;
	result = ioctl(fd, TIOCGWINSZ, &ws);
	close(fd);
	if(result<0) return;
	if(cols==ws.ws_col && lines ==ws.ws_row) return;
	cols=ws.ws_col;
	lines=ws.ws_row;
	endwin();
	doupdate();
	i_termwininit();
	statusflags|=S_DirtyScr;
}

Filepos
i_scrtofpos(int x, int y) {
	Filepos pos;
	Line *l;
	int irow, ixrow, ivchar, extralines=0;

	pos.l=lstline;
	pos.o=pos.l->len;
	for(l=scrline, irow=0; l && irow<LINES2; l=l->next, irow+=(extralines+1)) {
		extralines=(l->vlen / cols);
		for(ixrow=ivchar=0;ixrow<=extralines && (irow+ixrow)<LINES2;ixrow++)
			if(irow+ixrow == y) {
				pos.l=l;
				pos.o=0;
				while(x>(ivchar%cols) || (ivchar/cols)<ixrow) {
					ivchar+=VLEN(l->c[pos.o], ivchar%cols);
					pos.o++;
				}
				if(pos.o>pos.l->len) pos.o=pos.l->len;
				break;
			}
	}
	return pos;
}

bool
i_setfindterm(char *find_term) { /* Return TRUE if find term is a valid RE or NULL */
	statusflags&=~S_Selecting;
	if(find_term) { /* Modify find term; use NULL to repeat search */
		if(!regcomp(find_res[sel_re^1], find_term, REG_EXTENDED|REG_NEWLINE|(statusflags&S_CaseIns?REG_ICASE:0))) {
			sel_re^=1;
			setenv(envs[EnvFind], find_term, 1);
			return TRUE;
		} else return FALSE;
	} else return TRUE;
}

void
i_setup(void){
	int i, j;
	Line *l=NULL;

	/* Signal handling, default */
	signal(SIGWINCH, SIG_IGN);
	signal(SIGINT,   i_cleanup);
	signal(SIGTERM,  i_cleanup);

	/* Some allocs */
	title   =calloc(1, BUFSIZ);
	fifopath=calloc(1, PATHSIZ);
	if (((find_res[0] = (regex_t*)calloc(1, sizeof (regex_t))) == NULL)
			|| (find_res[1] = (regex_t*)calloc(1, sizeof (regex_t))) == NULL)
		i_die("Can't malloc.\n");

	for(i=0; i<LENGTH(cmds); i++) {
		if((cmds[i].re=(regex_t*)calloc(1, sizeof (regex_t))) == NULL) i_die("Can't malloc.\n");
		if(regcomp(cmds[i].re, cmds[i].re_text, REG_EXTENDED|REG_ICASE|REG_NEWLINE)) i_die("Faulty regex.\n");
	}

	for(i=0; i<LENGTH(syntaxes); i++) {
		if((syntaxes[i].file_re=(regex_t*)calloc(1, sizeof (regex_t))) == NULL) i_die("Can't malloc.\n");
		if(regcomp(syntaxes[i].file_re, syntaxes[i].file_re_text, REG_EXTENDED|REG_NOSUB|REG_ICASE|REG_NEWLINE)) i_die("Faulty regex.\n");
		for(j=0; j<SYN_COLORS; j++)
			if((syntaxes[i].re[j]=(regex_t*)calloc(1, sizeof (regex_t))) == NULL) i_die("Can't malloc.\n");
	}

	snprintf(fifopath, PATHSIZ, "%s%d", fifobase, getpid());
	if(mkfifo(fifopath, (S_IRUSR|S_IWUSR)) !=0) i_die("FIFO already exists.\n");
	if((fifofd = open(fifopath, O_RDONLY | O_NONBLOCK)) == -1) i_die("Can't open FIFO for reading.\n");;
	setenv(envs[EnvFifo], fifopath, 1);
	regcomp(find_res[0], "", 0);
	regcomp(find_res[1], "", 0);

	initscr();
	if(has_colors()) {
		start_color();
		use_default_colors();
		for(i=0; i<LastFG; i++)
			for(j=0; j<LastBG; j++) {
				init_pair((i*LastBG)+j, fgcolors[i], bgcolors[j]);
				textattrs[i][j] = COLOR_PAIR((i*LastBG)+j) | colorattrs[i];
			}
	} else {
		for(i=0; i<LastFG; i++)
			for(j=0; j<LastBG; j++)
				textattrs[i][j] = bwattrs[i];
	}
	lines=LINES;
	cols=COLS;
	i_termwininit();

	/* Init line structure */
	if(((l=(Line*)malloc(sizeof(Line))) == NULL) ||
		((l->c=calloc(1, LINSIZ)) == NULL))
		i_die("Can't malloc.\n");
	l->c[0]='\0';
	l->dirty=FALSE;
	l->len = l->vlen = 0; l->mul = 1;
	l->next = NULL; l->prev = NULL;
	fstline=lstline=scrline=fcur.l=l;
	fcur.o=0;
	fsel=fcur;
}

void
i_sigwinch(int unused) {
	statusflags|=S_NeedResize;
}

void
i_sortpos(Filepos *pos0, Filepos *pos1) {
	Filepos p;

	for(p.l=fstline;p.l;p.l=p.l->next) {
		if(p.l==pos0->l || p.l==pos1->l) {
			if((p.l==pos0->l && (p.l==pos1->l && pos1->o < pos0->o)) ||
				(p.l==pos1->l && p.l!=pos0->l))
				p=*pos0, *pos0=*pos1, *pos1=p;
			break;
		}
	}
}

char* /* you MUST free the returned string after use */
i_strdup(const char *src) {
	char *dst;
	int i;

	i=1+strlen(src);
	if((dst=malloc(i)) == NULL)
		i_die("Can't malloc.\n");
	return memcpy(dst, src, i);
}

void
i_termwininit(void) {
	raw();
	noecho();
	nl();
	if(textwin) delwin(textwin);
	if(tigetflag("hs") > 0) {
		tsl_str=tigetstr("tsl");
		fsl_str=tigetstr("fsl");
		textwin=newwin(lines,cols,0,0);
	} else {
		if(titlewin) delwin(titlewin);
		titlewin=newwin(1,cols,0,0);
		wattron(titlewin,A_REVERSE);
		textwin=newwin(lines-1,cols,1,0);
	}
	idlok(textwin, TRUE);
	keypad(textwin, TRUE);
	meta(textwin, TRUE);
	nodelay(textwin, FALSE);
	wtimeout(textwin, 0);
	curs_set(1);
	scrollok(textwin, FALSE);
	intrflush(NULL, TRUE); /* TODO: test this */
	mousemask(defmmask, NULL);
}

void /* This is where everything happens */
i_update(void) {
	static int iline, irow, ixrow, ichar, ivchar, i, ifg, ibg, extralines;
	static int cursor_r, cursor_c;
	static int lines3; /* How many lines fit on screen */
	static long int nscr, ncur, nlst; /* Line number for scrline, fcur.l and lstline */
	static bool selection;
	static regmatch_t match[SYN_COLORS][1];
	static Line *l;
	static char c[7], buf[16];

	/* Check if we need to resize */
	if(statusflags & S_NeedResize) i_resize();

	/* Check offset */
	offset_top:
	for(selection=FALSE, l=scrline->prev, iline=1; l; iline++, l=l->prev) {
		if(l==fsel.l) /* Selection starts before screen view */
			selection=!selection;
		if(l==fcur.l) { /* Can't have fcur.l before scrline */
			scrline=l;
			statusflags|=S_DirtyDown; /* TODO: wscrl() ?? */
			goto offset_top;
		}
	}
	for(irow=0, l=scrline; l; l=l->next, irow+=(extralines+1)) {
		extralines=l->vlen/cols;
		if(fcur.l==l) {
			while(irow+extralines>=LINES2 && scrline->next) {
				statusflags|=S_DirtyScr; /* TODO: wscrl() ?? */
				irow -= 1 + scrline->vlen/cols;
				if(scrline==fsel.l) selection=!selection; /* We just scrolled past the selection point */
				scrline=scrline->next;
				iline++;
			}
			break;
		}
	}
	nscr=iline;

	/* Actually update lines on screen */
	for(irow=lines3=0, l=scrline; irow<LINES2; irow+=(extralines+1), lines3++, iline++) {
		extralines=(l==NULL ? 0 : (l->vlen / cols) );
		if(fcur.l==l) {
			ncur=iline;
			/* Update screen cursor position */
			cursor_c=0; cursor_r=irow;
			for(ichar=0; ichar<fcur.o; ichar++) cursor_c+=VLEN(fcur.l->c[ichar], cursor_c);
			while(cursor_c >= cols) {
				cursor_c-=cols;
				cursor_r++;
			}
		}
		if(statusflags & S_DirtyScr || (l && l->dirty && (statusflags & S_DirtyDown ? statusflags|=S_DirtyScr : 1) )) {
			/* Print line content */
			if(l) l->dirty=FALSE;
			if(syntx && l) for(i=0; i<SYN_COLORS; i++)
				if(regexec(syntx->re[i], l->c, 1, match[i], 0) || match[i][0].rm_so == match[i][0].rm_eo)
					match[i][0].rm_so=match[i][0].rm_eo=-1;
			for(ixrow=ichar=ivchar=0; ixrow<=extralines && (irow+ixrow)<LINES2; ixrow++) {
				wmove(textwin, (irow+ixrow), (ivchar%cols));
				while(ivchar<(1+ixrow)*cols) {
					if(fcur.l==l && ichar==fcur.o) selection=!selection;
					if(fsel.l==l && ichar==fsel.o) selection=!selection;
					ifg=DefFG, ibg=DefBG;
					if(fcur.l==l) ifg=CurFG, ibg=CurBG;
					if(selection) ifg=SelFG, ibg=SelBG;
					if(syntx && l) for(i=0; i<SYN_COLORS; i++) {
						if(match[i][0].rm_so == -1) continue;
						if(ichar >= match[i][0].rm_eo) {
							if(regexec(syntx->re[i], &l->c[ichar], 1, match[i], REG_NOTBOL) || match[i][0].rm_so == match[i][0].rm_eo)
								continue;
							match[i][0].rm_so+=ichar;
							match[i][0].rm_eo+=ichar;
						}
						if(ichar >= match[i][0].rm_so && ichar < match[i][0].rm_eo)
							ifg=Syn0FG+i;
					}
					wattrset(textwin, textattrs[ifg][ibg]);
					if(l && ichar<l->len) {
						if(l->c[ichar] == 0x09) { /* Tab nightmare */
							wattrset(textwin, textattrs[SpcFG][ibg]);
							for(i=0; i<VLEN(0x09, ivchar%cols); i++) waddstr(textwin, ((i==0 && isutf8)?tabstr:" "));
						} else if(l->c[ichar] == ' ') { /* Space */
							wattrset(textwin, textattrs[SpcFG][ibg]);
							waddstr(textwin, (isutf8?spcstr:" "));
						} else if(ISCTRL(l->c[ichar])) { /* Add Ctrl-char as string to avoid problems at right screen end */
							wattrset(textwin, textattrs[CtrlFG][ibg]);
							waddstr(textwin, unctrl(l->c[ichar]));
						} else if(isutf8 && !ISASCII(l->c[ichar])) { /* Begin multi-byte char, dangerous at right screen end */
							for(i=0; i<UTF8LEN(l->c[ichar]); i++)
								if(ichar+i<l->len) c[i]=l->c[ichar+i];
								else c[i]=0x00;
							c[i]=0x00; /* Warning: we use i later... */
							waddstr(textwin, c);
						} else {
							waddch(textwin, l->c[ichar]);
						}
						ivchar+=VLEN(l->c[ichar], ivchar%cols);
						if(isutf8 && !ISASCII(l->c[ichar]) && i) ichar+=i; /* ...here */
						else ichar++;
					} else {
						ifg=DefFG, ibg=DefBG;
						if(fcur.l==l) ifg=CurFG, ibg=CurBG;
						if(selection) ifg=SelFG, ibg=SelBG;
						wattrset(textwin, textattrs[ifg][ibg]);
						waddch(textwin, ' ');
						ivchar++; ichar++;
					}
				}
			}
		} else if(l == fsel.l || l == fcur.l) selection=!selection;
		if(l) l=l->next;
	}

	/* Calculate nlst */
	for(iline=ncur, l=fcur.l; l; l=l->next, iline++)
		if(l==lstline) nlst=iline;

	/* Position cursor */
	wmove(textwin, cursor_r, cursor_c);

	/* Update env*/
	snprintf(buf, 16, "%ld", ncur);
	setenv(envs[EnvLine], buf, 1);
	snprintf(buf, 16, "%d", fcur.o);
	setenv(envs[EnvOffset], buf, 1);

	/* Update title */
	if(tmptitle)
		strncpy(title, tmptitle, BUFSIZ);
	else {
		snprintf(buf, 4, "%ld%%", (100*ncur)/nlst);
		snprintf(title, BUFSIZ, "%s [%s]%s%s%s%s %ld,%d  %s",
			(filename == NULL?"<No file>":filename),
			(syntx?syntx->name:"none"),
			(t_mod()?"[+]":""),
			(!t_rw()?"[RO]":""),
			(statusflags&S_CaseIns?"[icase]":""),
			(statusflags&S_Selecting?"[SEL]":""),
			ncur, fcur.o,
			(scrline==fstline?
				(nlst<lines3?"All":"Top"):
				(nlst-nscr<lines3?"Bot":buf)
			));
	}
	if(titlewin) {
		int i;
		wmove(titlewin, 0, 0);
		for(i=0; i<cols; i++) waddch(titlewin, ' ');
		mvwaddnstr(titlewin, 0, 0, title, cols);
	} else {
		putp(tsl_str); putp(title); putp(fsl_str);
	}

	/* Clean global dirty bits */
	statusflags&=~(S_DirtyScr|S_DirtyDown|S_NeedResize);
	tmptitle=NULL;

	/* And go.... */
	if(titlewin) wnoutrefresh(titlewin);
	wnoutrefresh(textwin);
	doupdate();
}

void
i_usage(void) {
	fputs("sandy - simple editor\n", stderr);
	i_die("usage: sandy [-r] [-u] [-t TABSTOP] [-s SYNTAX] [file | -]\n");
}

bool
i_writefile(void) {
	int fd;
	bool wok=TRUE;
	Line *l;

	if (filename == NULL || (fd = open(filename, O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO)) == -1) {
		/* error */
		tmptitle="WARNING! Can't save file!!!";
		return FALSE;
	}

	for(l=fstline; wok && l; l=l->next) {
		if(write(fd, l->c, l->len) == -1 ||
			(l->next && write(fd, "\n", 1) == -1)) wok=FALSE;
	}
	close(fd);
	return wok;
}

Filepos
m_bof(Filepos pos) {
	pos.l=fstline;
	pos.o=0;
	return pos;
}

Filepos
m_bol(Filepos pos) {
	Filepos vbol=pos;

	vbol.o=0;
	while(ISWORDBRK(vbol.l->c[vbol.o]) && ++vbol.o<vbol.l->len);
	if(pos.o!=0 && pos.o<=vbol.o) vbol.o=0;
	return vbol;
}

Filepos
m_eof(Filepos pos) {
	pos.l=lstline;
	pos.o=pos.l->len;
	return pos;
}

Filepos
m_eol(Filepos pos){
	pos.o=pos.l->len;
	return pos;
}

Filepos
m_nextchar(Filepos pos) {
	if(pos.o < pos.l->len) {
		pos.o++;
		FIXNEXT(pos);
	} else if(pos.l->next) {
		pos.l=pos.l->next;
		pos.o=0;
	}
	return pos;
}

Filepos
m_prevchar(Filepos pos) {
	if(pos.o > 0) {
		pos.o--;
		FIXPREV(pos);
	} else if(pos.l->prev) {
		pos.l=pos.l->prev;
		pos.o=pos.l->len;
	}
	return pos;
}

Filepos
m_nextword(Filepos pos) {
	Filepos p0;

	do {
		p0=pos;
		pos=m_nextchar(pos);
	} while((p0.o!=pos.o || p0.l!=pos.l) && !ISWORDBRK(pos.l->c[pos.o]));
	return pos;
}

Filepos
m_prevword(Filepos pos) {
	Filepos p0;

	pos=m_prevchar(pos);
	do {
		p0=pos;
		pos=m_prevchar(pos);
	} while((p0.o!=pos.o || p0.l!=pos.l) && !ISWORDBRK(pos.l->c[pos.o]));
	return p0;
}

Filepos
m_nextline(Filepos pos) {
	if(pos.l->next){
		pos.l=pos.l->next;
		if(pos.o>pos.l->len) pos.o=pos.l->len;
		FIXNEXT(pos);
	} else pos.o=pos.l->len;
	return pos;
}

Filepos
m_prevline(Filepos pos) {
	if(pos.l->prev){
		pos.l=pos.l->prev;
		if(pos.o>pos.l->len) pos.o=pos.l->len;
		FIXNEXT(pos);
	} else pos.o=0;
	return pos;
}

Filepos
m_nextscr(Filepos pos) {
	int i;
	Line *l;

	for(i=LINES2,l=pos.l; l->next && i>0; i-=1+l->vlen/cols, l=l->next);
	pos.l=l;
	if(pos.o>pos.l->len) pos.o=pos.l->len;
	FIXNEXT(pos);
	return pos;
}

Filepos
m_prevscr(Filepos pos) {
	int i;
	Line *l;

	for(i=LINES2,l=pos.l; l->prev && i>0; i-=1+l->vlen/cols, l=l->prev);
	pos.l=l;
	if(pos.o>pos.l->len) pos.o=pos.l->len;
	FIXNEXT(pos);
	return pos;
}

Filepos
m_stay(Filepos pos) {
	return pos;
}

Filepos
m_tomark(Filepos pos) {
	/* Be extra careful when moving to mark, as it might not exist */
	Line *l;
	for(l=fstline; l; l=l->next) if(l!=NULL && l==fmrk.l) {
		pos.l=fmrk.l;
		pos.o=fmrk.o;
		if(pos.o>pos.l->len) pos.o=pos.l->len;
		FIXNEXT(pos);
		break;
	}
	return pos;
}

Filepos
m_tosel(Filepos pos) {
	return fsel;
}

bool
t_bol(void) {
	return (fcur.o == 0);
}

bool
t_eol(void) {
	return (fcur.o == fcur.l->len);
}

bool
t_mod(void) {
	return (statusflags & S_Modified);
}

bool
t_rw(void) {
	return !(statusflags & S_Readonly);
}

bool
t_redo(void) {
	return (redos != NULL);
}

bool
t_sel(void) {
	return !(fcur.l==fsel.l && fcur.o == fsel.o);
}

bool
t_undo(void) {
	return (undos != NULL);
}

int
main(int argc, char **argv){
	int i;
	char *local_syn = NULL;

	/* Use system locale, hopefully UTF-8 */
	setlocale(LC_ALL,"");

	for(i = 1; i < argc && argv[i][0] == '-' && argv[i][1] != '\0'; i++) {
		if(!strcmp(argv[i], "-r")) {
			statusflags|=S_Readonly;
		} else if(!strcmp(argv[i], "-u")) {
			isutf8=!isutf8;
		} else if(!strcmp(argv[i], "-t")) {
			if(++i < argc) {
				tabstop=atoi(argv[i]);
			} else
				i_usage();
		} else if(!strcmp(argv[i], "-s")) {
			if(++i < argc) {
				local_syn=argv[i];
			} else
				i_usage();
		} else if(!strcmp(argv[i], "--")) {
			i++;
			break;
		} else if(!strcmp(argv[i], "-v"))
			i_die("sandy-"VERSION", © 2010 sandy engineers, see LICENSE for details\n");
		else
			i_usage();
	}
	i_setup();
	if(i < argc) i_readfile(argv[i]);
	if(local_syn) f_syntax(&(const Arg){ .v = local_syn });
	i_edit();
	i_cleanup(0);
}

