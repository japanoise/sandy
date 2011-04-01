/* Things unlikely to be changed, yet still in the config.h file */
static       bool            isutf8     = TRUE; /* Default, reverse with -u */
static const char            fifobase[] = "/tmp/sandyfifo.";

/* TAB and Space character aspect on screen */
static       int    tabstop    = 8; /* Not const, as it may be changed via param */
static const char   tabstr[3]  = { (char)0xC2, (char)0xBB, 0x00 }; /* Double right arrow */
static const char   spcstr[3]  = { (char)0xC2, (char)0xB7, 0x00 }; /* Middle dot */

/* Custom function declaration */
static bool t_x(void);

/* Paths */
//static const char   systempath[]  = "/etc/sandy";
//static const char   userpath[]    = ".sandy"; /* Relative to $HOME */

/* Args to f_spawn, X version */
#define FIND_X { .v = (const char *[]){ "/bin/sh", "-c", \
	"arg=\"`echo \\\"${SANDY_FIND}\\\" | dmenu -p Find:`\" &&" \
	"echo find \"$arg\" > ${SANDY_FIFO}", NULL } }
#define FINDBW_X { .v = (const char *[]){ "/bin/sh", "-c", \
	"arg=\"`echo \\\"${SANDY_FIND}\\\" | dmenu -p 'Find (back):'`\" &&" \
	"echo findbw \"$arg\" > ${SANDY_FIFO}", NULL } }
#define PIPE_X { .v = (const char *[]){ "/bin/sh", "-c", \
	"arg=\"`echo \\\"${SANDY_PIPE}\\\" | dmenu -p Pipe:`\" " \
	"&&" "echo pipe \"$arg\" > ${SANDY_FIFO}" , NULL } }
#define SAVEAS_X { .v = (const char *[]){ "/bin/sh", "-c", \
	"arg=\"`echo \\\"${SANDY_FILE}\\\" | dmenu -p 'Save as:'`\" &&" \
	"echo save \"$arg\" > ${SANDY_FIFO}", NULL } }
#define LINE_X { .v = (const char *[]){ "/bin/sh", "-c", \
	"arg=\"`echo \\\"${SANDY_LINE}\\\" | dmenu -p 'Line:'`\" &&" \
	"echo line \"$arg\" > ${SANDY_FIFO}", NULL } }
#define SYNTAX_X { .v = (const char *[]){ "/bin/sh", "-c", \
	"arg=\"`echo \\\"${SANDY_SYNTAX}\\\" | dmenu -p 'Syntax:'`\" &&" \
	"echo syntax \"$arg\" > ${SANDY_FIFO}", NULL } }

/* Args to f_spawn, non-X version */
#define FIND_NOX { .v = (const char *[]){ "/bin/sh", "-c", \
	"echo -n '\033[H\033[K\033[7m'; read -p 'Find: ' arg && echo find $arg > ${SANDY_FIFO}", NULL } }
#define FINDBW_NOX { .v = (const char *[]){ "/bin/sh", "-c", \
	"echo -n '\033[H\033[K\033[7m'; read -p 'Find (back): ' arg && echo findbw $arg > ${SANDY_FIFO}", NULL } }
#define PIPE_NOX { .v = (const char *[]){ "/bin/sh", "-c", \
	"echo -n '\033[H\033[K\033[7m'; read -p 'Pipe: ' arg && echo pipe $arg > ${SANDY_FIFO}", NULL } }
#define SAVEAS_NOX { .v = (const char *[]){ "/bin/sh", "-c", \
	"echo -n '\033[H\033[K\033[7m'; read -p 'Save as: ' arg && echo save $arg > ${SANDY_FIFO}", NULL } }
#define LINE_NOX { .v = (const char *[]){ "/bin/sh", "-c", \
	"echo -n '\033[H\033[K\033[7m'; read -p 'Line: ' arg && echo line $arg > ${SANDY_FIFO}", NULL } }
#define SYNTAX_NOX { .v = (const char *[]){ "/bin/sh", "-c", \
	"echo -n '\033[H\033[K\033[7m'; read -p 'Syntax: ' arg && echo syntax $arg > ${SANDY_FIFO}", NULL } }

/* Hooks are launched from the main code */
#define HOOK_SAVE_NO_FILE if(t_x()) f_spawn(&(const Arg)SAVEAS_X); else f_spawn(&(const Arg)SAVEAS_NOX)
#define HOOK_SELECT_MOUSE if(t_x()) f_pipero(&(const Arg){ .v = ("xsel -i")})
#undef  HOOK_PRE_DELETE   /* This affects every delete */
#undef  HOOK_SELECT_ALL   /* Do not bother */

/* Key-bindings and stuff */
/* WARNING: use CONTROL(ch) ONLY with '@', (caps)A-Z, '[', '\', ']', '^', '_' or '?' */
/*          otherwise it may not mean what you think. See man 7 ascii for more info */
#define CONTROL(ch)   {(ch ^ 0x40)}
#define META(ch)      {0x1B, ch}

static const Key curskeys[] = { /* Don't use CONTROL or META here */
/* keyv,            tests,                     func,      arg */
{ {KEY_BACKSPACE},  { t_sel, t_rw, 0,   0 },   f_delete,  { .m = m_tosel    } },
{ {KEY_BACKSPACE},  { t_rw,  0,    0,   0 },   f_delete,  { .m = m_prevchar } },
{ {KEY_DC},         { t_sel, t_rw, 0,   0 },   f_delete,  { .m = m_tosel    } },
{ {KEY_DC},         { t_rw,  0,    0,   0 },   f_delete,  { .m = m_nextchar } },
{ {KEY_IC},         { t_sel, t_x,  0,   0 },   f_pipero,  { .v = "xsel -ib"  } },
{ {KEY_IC},         { t_sel, 0,    0,   0 },   f_pipero,  { .v = "cat > $HOME/.sandy.clipboard" } },
{ {KEY_SDC},        { t_sel, t_rw, t_x, 0 },   f_pipe,    { .v = "xsel -ib"  } },
{ {KEY_SDC},        { t_sel, t_rw, 0,   0 },   f_pipe,    { .v = "cat > $HOME/.sandy.clipboard" } },
{ {KEY_SIC},        { t_rw,  t_x,  0,   0 },   f_pipe,    { .v = "xsel -ob"  } },
{ {KEY_SIC},        { t_rw,  0,    0,   0 },   f_pipe,    { .v = "cat $HOME/.sandy.clipboard" } },
{ {KEY_HOME},       { 0,     0,    0,   0 },   f_move,    { .m = m_bol      } },
{ {KEY_END},        { 0,     0,    0,   0 },   f_move,    { .m = m_eol      } },
{ {KEY_SHOME},      { 0,     0,    0,   0 },   f_move,    { .m = m_bof      } },
{ {KEY_SEND},       { 0,     0,    0,   0 },   f_move,    { .m = m_eof      } },
{ {KEY_PPAGE},      { 0,     0,    0,   0 },   f_move,    { .m = m_prevscr  } },
{ {KEY_NPAGE},      { 0,     0,    0,   0 },   f_move,    { .m = m_nextscr  } },
{ {KEY_UP},         { 0,     0,    0,   0 },   f_move,    { .m = m_prevline } },
{ {KEY_DOWN},       { 0,     0,    0,   0 },   f_move,    { .m = m_nextline } },
{ {KEY_LEFT},       { 0,     0,    0,   0 },   f_move,    { .m = m_prevchar } },
{ {KEY_RIGHT},      { 0,     0,    0,   0 },   f_move,    { .m = m_nextchar } },
{ {KEY_SLEFT},      { 0,     0,    0,   0 },   f_move,    { .m = m_prevword } },
{ {KEY_SRIGHT},     { 0,     0,    0,   0 },   f_move,    { .m = m_nextword } },
};

static const Key stdkeys[] = {
/* keyv,        test,                     func,        arg */
/* You probably know these as TAB, Enter and Return */
{ CONTROL('I'), { t_sel, t_rw, 0,   0 },  f_pipelines, { .v = "sed 's/^/\\t/'" } },
{ CONTROL('I'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\t" } },
{ CONTROL('J'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\n" } },
{ CONTROL('J'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ CONTROL('M'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\n" } },
{ CONTROL('M'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },

/* Cursor movement, also when selecting */
{ CONTROL('A'), { 0,     0,    0,   0 },  f_move,      { .m = m_bol } },
{ CONTROL('E'), { 0,     0,    0,   0 },  f_move,      { .m = m_eol } },
{ CONTROL('F'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextchar } },
{ META('f'),    { 0,     0,    0,   0 },  f_move,      { .m = m_nextword } },
{ CONTROL('B'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevchar } },
{ META('b'),    { 0,     0,    0,   0 },  f_move,      { .m = m_prevword } },
{ CONTROL('N'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ CONTROL('P'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevline } },
{ META(','),    { 0,     0,    0,   0 },  f_move,      { .m = m_prevscr } },
{ META('.'),    { 0,     0,    0,   0 },  f_move,      { .m = m_nextscr } },
{ META('<'),    { 0,     0,    0,   0 },  f_move,      { .m = m_bof } },
{ META('>'),    { 0,     0,    0,   0 },  f_move,      { .m = m_eof } },
{ META('g'),    { t_x,   0,    0,   0 },  f_spawn,     LINE_X },
{ META('g'),    { 0,     0,    0,   0 },  f_spawn,     LINE_NOX },

/* Finding and selecting */
{ CONTROL('S'), { t_x,   0,    0,   0 },  f_spawn,     FIND_X },
{ CONTROL('S'), { 0,     0,    0,   0 },  f_spawn,     FIND_NOX },
{ CONTROL('R'), { t_x,   0,    0,   0 },  f_spawn,     FINDBW_X },
{ CONTROL('R'), { 0,     0,    0,   0 },  f_spawn,     FINDBW_NOX },
{ META('n'),    { 0,     0,    0,   0 },  f_findfw,    { 0 } },
{ META('p'),    { 0,     0,    0,   0 },  f_findbw,    { 0 } },
{ CONTROL('X'), { 0,     0,    0,   0 },  f_extsel,    { .i = ExtDefault } },
{ META('x'),    { 0,     0,    0,   0 },  f_extsel,    { .i = ExtAll } },
{ CONTROL('G'), { t_sel, 0,    0,   0 },  f_select,    { .m = m_stay } },
{ CONTROL('G'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_Selecting } },
{ META('i'),    { 0,     0,    0,   0 },  f_toggle,    { .i = S_CaseIns } },
{ META('s'),    { t_sel, 0,    0,   0 },  f_pipero,    { .v = "(sed 's/$/\\n/;2q' | (read arg && echo find \"$arg\" > ${SANDY_FIFO}))" } },
{ META('r'),    { t_sel, 0,    0,   0 },  f_pipero,    { .v = "(sed 's/$/\\n/;2q' | (read arg && echo findbw \"$arg\" > ${SANDY_FIFO}))" } },

/* Text deletion */
{ CONTROL('D'), { t_sel, t_rw, t_x, 0 },  f_pipe,      { .v = "xsel -ib" } },
{ CONTROL('D'), { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "cat > $HOME/.sandy.clipboard" } },
{ CONTROL('D'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextchar } },
{ CONTROL('D'), { 0,     0,    0,   0 },  f_select,    { .m = m_nextchar } },
{ CONTROL('?'), { t_sel, t_rw, t_x, 0 },  f_pipe,      { .v = "xsel -ib" } },
{ CONTROL('?'), { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "cat > $HOME/.sandy.clipboard" } },
{ CONTROL('?'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('?'), { 0,     0,    0,   0 },  f_select,    { .m = m_prevchar } },
{ CONTROL('H'), { t_sel, t_rw, t_x, 0 },  f_pipe,      { .v = "xsel -ib" } },
{ CONTROL('H'), { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "cat > $HOME/.sandy.clipboard" } },
{ CONTROL('H'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('H'), { 0,     0,    0,   0 },  f_select,    { .m = m_prevchar } },
{ CONTROL('U'), { t_sel, t_rw, t_x, 0 },  f_pipe,      { .v = "xsel -ib" } },
{ CONTROL('U'), { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "cat > $HOME/.sandy.clipboard" } },
{ CONTROL('U'), { t_bol, t_rw, 0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('U'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_bol } },
{ CONTROL('U'), { 0,     0,    0,   0 },  f_select,    { .m = m_bol } },
{ CONTROL('K'), { t_sel, t_rw, t_x, 0 },  f_pipe,      { .v = "xsel -ib" } },
{ CONTROL('K'), { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "cat > $HOME/.sandy.clipboard" } },
{ CONTROL('K'), { t_eol, t_rw, 0,   0 },  f_delete,    { .m = m_nextchar } },
{ CONTROL('K'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_eol } },
{ CONTROL('K'), { 0,     0,    0,   0 },  f_select,    { .m = m_eol } },
{ CONTROL('W'), { t_sel, t_rw, t_x, 0 },  f_pipe,      { .v = "xsel -ib" } },
{ CONTROL('W'), { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "cat > $HOME/.sandy.clipboard" } },
{ CONTROL('W'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevword } },
{ CONTROL('W'), { 0,     0,    0,   0 },  f_select,    { .m = m_prevword } },
{ META('d'),    { t_sel, t_rw, t_x, 0 },  f_pipe,      { .v = "xsel -ib" } },
{ META('d'),    { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "cat > $HOME/.sandy.clipboard" } },
{ META('d'),    { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextword } },
{ META('d'),    { 0,     0,    0,   0 },  f_select,    { .m = m_nextword } },

/* Mark operation */
{ META(' '),    { 0,     0,    0,   0 },  f_mark,      { 0 } },
{ CONTROL('@'), { 0,     0,    0,   0 },  f_move,      { .m = m_tomark } },

/* File operations */
{ META('q'),    { t_mod, 0,    0,   0 },  f_title,     { .v = "WARNING! File not saved!" } },
{ META('q'),    { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ META('Q'),    { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ META('w'),    { 0,     0,    0,   0 },  f_save,      { 0 } },
{ META('W'),    { t_x,   0,    0,   0 },  f_spawn,     SAVEAS_X },
{ META('W'),    { 0,     0,    0,   0 },  f_spawn,     SAVEAS_NOX },

/* Text piping and modification */
{ CONTROL('\\'),{ t_x,   0,    0,   0 },  f_spawn,     PIPE_X },
{ CONTROL('\\'),{ 0,     0,    0,   0 },  f_spawn,     PIPE_NOX },
{ CONTROL('Y'), { t_rw,  t_x,  0,   0 },  f_pipe,      { .v = "xsel -ob" } },
{ CONTROL('Y'), { t_rw,  0,    0,   0 },  f_pipe,      { .v = "cat $HOME/.sandy.clipboard" } },
{ CONTROL('C'), { t_sel, t_x,  0,   0 },  f_pipero,    { .v = "xsel -ib" } },
{ CONTROL('C'), { t_sel, 0,    0,   0 },  f_pipero,    { .v = "cat > $HOME/.sandy.clipboard" } },

/* Windows-like crap TO REMOVE */
{ CONTROL('Z'), { t_undo,0,    0,   0 },  f_undo,      { .i =  1 } },

/* Others */
{ CONTROL('L'), { 0,     0,    0,   0 },  f_center,    { 0 } },
{ CONTROL('V'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_InsEsc } },
{ META('R'),    { 0,     0,    0,   0 },  f_toggle,    { .i = S_Readonly } },
{ META('S'),    { t_x,   0,    0,   0 },  f_spawn,     SYNTAX_X },
{ META('S'),    { 0,     0,    0,   0 },  f_spawn,     SYNTAX_NOX },
};

/* Commands read at the fifo */
static Command cmds[] = { /* Use only f_ funcs that take Arg.v */
/* \0, regex,           tests,        func */
{NULL, "^find (.*)$",   { 0,     0 }, f_findfw },
{NULL, "^findbw (.*)$", { 0,     0 }, f_findbw },
{NULL, "^pipe (.*)$",   { t_rw,  0 }, f_pipe   },
{NULL, "^pipe (.*)$",   { 0,     0 }, f_pipero },
{NULL, "^save (.*)$",   { 0,     0 }, f_save   },
{NULL, "^syntax (.*)$", { 0,     0 }, f_syntax },
{NULL, "^line (.*)$",   { 0,     0 }, f_line   },
{NULL, "^offset (.*)$", { 0,     0 }, f_offset },
};

/* Syntax color definition */
static Syntax syntaxes[] = {
{"c", NULL, "\\.(c(pp|xx)?|h(pp|xx)?|cc)$", { NULL }, {
	/* HiRed   */  "",
	/* HiGreen */  "\\<(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)\\>",
	/* LoGreen */  "\\<(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|namespace|template|public|protected|private|typename|this|friend|virtual|using|mutable|volatile|register|explicit)\\>",
	/* HiMag   */  "\\<(goto|continue|break|return)\\>",
	/* LoMag   */  "^[[:space:]]*#[[:space:]]*(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma)",
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoRed   */  "(\\<[A-Z_][0-9A-Z_]+\\>|\"(\\.|[^\"])*\")",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	} },
};

/* Colors */
static const short  fgcolors[LastFG] = {
        [DefFG]  = -1,
        [CurFG]  = COLOR_BLACK,
        [SelFG]  = COLOR_BLACK,
        [SpcFG]  = COLOR_WHITE,
        [CtrlFG] = COLOR_RED,
        [Syn0FG] = COLOR_RED,
        [Syn1FG] = COLOR_GREEN,
        [Syn2FG] = COLOR_GREEN,
        [Syn3FG] = COLOR_MAGENTA,
        [Syn4FG] = COLOR_MAGENTA,
        [Syn5FG] = COLOR_BLUE,
        [Syn6FG] = COLOR_RED,
        [Syn7FG] = COLOR_BLUE,
};

static const int colorattrs[LastFG] = {
        [DefFG]  = 0,
        [CurFG]  = 0,
        [SelFG]  = 0,
        [SpcFG]  = A_DIM,
        [CtrlFG] = A_DIM,
        [Syn0FG] = A_BOLD,
        [Syn1FG] = A_BOLD,
        [Syn2FG] = 0,
        [Syn3FG] = A_BOLD,
        [Syn4FG] = 0,
        [Syn5FG] = A_BOLD,
        [Syn6FG] = 0,
        [Syn7FG] = 0,
};

static const int bwattrs[LastFG] = {
        [DefFG]  = 0,
        [CurFG]  = 0,
        [SelFG]  = A_REVERSE,
        [SpcFG]  = A_DIM,
        [CtrlFG] = A_DIM,
        [Syn0FG] = A_BOLD,
        [Syn1FG] = A_BOLD,
        [Syn2FG] = A_BOLD,
        [Syn3FG] = A_BOLD,
        [Syn4FG] = A_BOLD,
        [Syn5FG] = A_BOLD,
        [Syn6FG] = A_BOLD,
        [Syn7FG] = A_BOLD,
};

static const short  bgcolors[LastBG] = {
        [DefBG] = -1,
        [CurBG] = COLOR_CYAN,
        [SelBG] = COLOR_YELLOW,
};

/* Custom function implementation */
bool
t_x(void) {
	return (getenv("DISPLAY") != NULL);
}

