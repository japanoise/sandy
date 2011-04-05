/* Things unlikely to be changed, yet still in the config.h file */
static       bool            isutf8     = TRUE; /* Default, reverse with -u */
static const char            fifobase[] = "/tmp/sandyfifo.";

/* TAB and Space character aspect on screen */
static       int    tabstop    = 8; /* Not const, as it may be changed via param */
static const char   tabstr[3]  = { (char)0xC2, (char)0xBB, 0x00 }; /* Double right arrow */
static const char   spcstr[3]  = { (char)0xC2, (char)0xB7, 0x00 }; /* Middle dot */

/* Paths */
//static const char   systempath[]  = "/etc/sandy";
//static const char   userpath[]    = ".sandy"; /* Relative to $HOME */

/* Args to f_spawn */
#define PROMPT(prompt, default, cmd) { .v = (const char *[]){ "/bin/sh", "-c", \
	"if [ $DISPLAY ]; then arg=\"`echo \\\"" default "\\\" | dmenu -p " prompt "`\";" \
	"else echo -n '\033[H\033[K\033[7m'; read -p '" prompt " ' arg; fi &&" \
	"echo " cmd " \"$arg\" > ${SANDY_FIFO}", NULL } }

#define FIND   PROMPT("Find:",        "${SANDY_FIND}",   "find")
#define FINDBW PROMPT("Find (back):", "${SANDY_FIND}",   "findbw")
#define PIPE   PROMPT("Pipe:",        "${SANDY_PIPE}",   "pipe")
#define SAVEAS PROMPT("Save as:",     "${SANDY_FILE}",   "save")
#define LINE   PROMPT("Line:",        "${SANDY_LINE}",   "line")
#define SYNTAX PROMPT("Syntax:",      "${SANDY_SYNTAX}", "syntax")

/* Args to f_pipe / f_pipero */
#define TOCLIP   { .v = "if [ $DISPLAY ] ; then xsel -ib; else cat > $HOME/.sandy.clipboard ; fi" }
#define FROMCLIP { .v = "if [ $DISPLAY ] ; then xsel -ob; else cat   $HOME/.sandy.clipboard ; fi" }
#define TOSEL    { .v = "if [ $DISPLAY ] ; then xsel -i;  else cat > $HOME/.sandy.selection ; fi" }
#define FROMSEL  { .v = "if [ $DISPLAY ] ; then xsel -o;  else cat   $HOME/.sandy.selection ; fi" }

/* Hooks are launched from the main code */
#define HOOK_SAVE_NO_FILE f_spawn (&(const Arg)SAVEAS)
#define HOOK_SELECT_MOUSE f_pipero(&(const Arg)TOSEL)
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
{ {KEY_IC},         { t_sel, 0,    0,   0 },   f_pipero,  TOCLIP },
{ {KEY_SDC},        { t_sel, t_rw, 0,   0 },   f_pipe,    TOCLIP },
{ {KEY_SIC},        { t_rw,  0,    0,   0 },   f_pipe,    FROMCLIP },
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
{ META('g'),    { 0,     0,    0,   0 },  f_spawn,     LINE },

/* Finding and selecting */
{ CONTROL('S'), { 0,     0,    0,   0 },  f_spawn,     FIND },
{ CONTROL('R'), { 0,     0,    0,   0 },  f_spawn,     FINDBW },
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
{ CONTROL('D'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('D'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextchar } },
{ CONTROL('D'), { 0,     0,    0,   0 },  f_select,    { .m = m_nextchar } },
{ CONTROL('?'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('?'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('?'), { 0,     0,    0,   0 },  f_select,    { .m = m_prevchar } },
{ CONTROL('H'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('H'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('H'), { 0,     0,    0,   0 },  f_select,    { .m = m_prevchar } },
{ CONTROL('U'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('U'), { t_bol, t_rw, 0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('U'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_bol } },
{ CONTROL('U'), { 0,     0,    0,   0 },  f_select,    { .m = m_bol } },
{ CONTROL('K'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('K'), { t_eol, t_rw, 0,   0 },  f_delete,    { .m = m_nextchar } },
{ CONTROL('K'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_eol } },
{ CONTROL('K'), { 0,     0,    0,   0 },  f_select,    { .m = m_eol } },
{ CONTROL('W'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('W'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevword } },
{ CONTROL('W'), { 0,     0,    0,   0 },  f_select,    { .m = m_prevword } },
{ META('d'),    { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ META('d'),    { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextword } },
{ META('d'),    { 0,     0,    0,   0 },  f_select,    { .m = m_nextword } },

/* Mark operation */
{ META(' '),    { 0,     0,    0,   0 },  f_mark,      { 0 } },
{ CONTROL('@'), { 0,     0,    0,   0 },  f_move,      { .m = m_tomark } },

/* File operations */
{ CONTROL('Q'), { t_mod, 0,    0,   0 },  f_title,     { .v = "WARNING! File not saved! Press META+SHIFT+Q to quit" } },
{ CONTROL('Q'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ META('q'),    { t_mod, 0,    0,   0 },  f_title,     { .v = "WARNING! File not saved!" } },
{ META('q'),    { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ META('Q'),    { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ META('w'),    { 0,     0,    0,   0 },  f_save,      { 0 } },
{ META('W'),    { 0,     0,    0,   0 },  f_spawn,     SAVEAS },

/* Text piping and modification */
{ CONTROL('\\'),{ 0,     0,    0,   0 },  f_spawn,     PIPE },
{ CONTROL('Y'), { t_rw,  0,    0,   0 },  f_pipe,      FROMCLIP },
{ CONTROL('C'), { t_sel, 0,    0,   0 },  f_pipero,    TOCLIP },

/* Windows-like crap TO REMOVE */
{ CONTROL('Z'), { t_undo,0,    0,   0 },  f_undo,      { .i =  1 } },

/* Others */
{ CONTROL('L'), { 0,     0,    0,   0 },  f_center,    { 0 } },
{ CONTROL('V'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_InsEsc } },
{ META('R'),    { 0,     0,    0,   0 },  f_toggle,    { .i = S_Readonly } },
{ META('S'),    { 0,     0,    0,   0 },  f_spawn,     SYNTAX },
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

