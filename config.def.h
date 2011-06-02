/* A simplified way to customize */
#define HILIGHT_CURRENT 1
#define SHOW_NONPRINT   0
#define HILIGHT_SYNTAX  1 /* TODO: implement this */

/* Things unlikely to be changed, yet still in the config.h file */
static const bool   isutf8     = TRUE;
static const char   fifobase[] = "/tmp/sandyfifo.";
static       int    tabstop    = 8; /* Not const, as it may be changed via param */
/* static const char   systempath[]  = "/etc/sandy"; */
/* static const char   userpath[]    = ".sandy"; */ /* Relative to $HOME */

#if SHOW_NONPRINT /* TODO: show newline character too (as $) */
static const char   tabstr[3]  = { (char)0xC2, (char)0xBB, 0x00 }; /* Double right arrow */
static const char   spcstr[3]  = { (char)0xC2, (char)0xB7, 0x00 }; /* Middle dot */
static const char   nlstr[2]   = { '$', 0x00 }; /* '$' is tradition for EOL */
#else
static const char   tabstr[2]  = { ' ', 0 };
static const char   spcstr[2]  = { ' ', 0 };
static const char   nlstr[1]   = { 0 };
#endif

/* Args to f_spawn */
#define PROMPT(prompt, default, cmd) { .v = (const char *[]){ "/bin/sh", "-c", \
	"dmenu -v >/dev/null 2>&1 || DISPLAY=\"\";"\
	"if [ -n \"$DISPLAY\" ]; then arg=\"`echo \\\"" default "\\\" | dmenu -p '" prompt "'`\";" \
	"else printf \"\033[0;0H\033[7m"prompt"\033[K\033[0m \"; read arg; fi &&" \
	"echo " cmd "\"$arg\" > ${SANDY_FIFO}", NULL } }

#define FIND   PROMPT("Find:",        "${SANDY_FIND}",   "/")
#define FINDBW PROMPT("Find (back):", "${SANDY_FIND}",   "?")
#define PIPE   PROMPT("Pipe:",        "${SANDY_PIPE}",   "|")
#define SAVEAS PROMPT("Save as:",     "${SANDY_FILE}",   "w")
#define CMD_P  PROMPT("Command:",     "/\n?\n|\nw\nsyntax\noffset", "")

/* Args to f_pipe / f_pipero */
/* TODO: make sandy-sel to wrap xsel or standalone */
#define TOCLIP   { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -ib || cat > ~/.sandy.clipboard" }
#define FROMCLIP { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -ob || cat ~/.sandy.clipboard" }
#define TOSEL    { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -i  || cat > ~/.sandy.selection" }
#define FROMSEL  { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -o  || cat ~/.sandy.selection" }

/* Hooks are launched from the main code */
#define HOOK_SAVE_NO_FILE f_spawn (&(const Arg)SAVEAS)
#define HOOK_SELECT_MOUSE f_pipero(&(const Arg)TOSEL)
#undef  HOOK_DELETE_ALL   /* This affects every delete */
#undef  HOOK_SELECT_ALL   /* This affects every selection */

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
{ CONTROL('@'), { 0,     0,    0,   0 },  f_mark,      { 0 } },
{ CONTROL('A'), { t_bol, 0,    0,   0 },  f_move,      { .m = m_prevscr } },
{ CONTROL('A'), { 0,     0,    0,   0 },  f_move,      { .m = m_bol } },
{ CONTROL('B'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevchar } },
{ CONTROL('C'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('C'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextword } },
{ CONTROL('C'), { 0,     0,    0,   0 },  f_select,    { .m = m_nextword } },
{ CONTROL('D'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('D'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextchar } },
{ CONTROL('D'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextchar } },
{ CONTROL('E'), { t_eol, 0,    0,   0 },  f_move,      { .m = m_nextscr } },
{ CONTROL('E'), { 0,     0,    0,   0 },  f_move,      { .m = m_eol } },
{ CONTROL('F'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextchar } },
{ CONTROL('G'), { t_sel, 0,    0,   0 },  f_select,    { .m = m_stay } },
{ CONTROL('G'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_Selecting } },
{ CONTROL('H'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('H'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('H'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevchar } },
{ CONTROL('I'), { t_sel, t_rw, 0,   0 },  f_pipelines, { .v = "sed 's/^/\\t/'" } },
{ CONTROL('I'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\t" } },
{ CONTROL('J'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\n" } },
{ CONTROL('J'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ CONTROL('K'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('K'), { t_eol, t_rw, 0,   0 },  f_delete,    { .m = m_nextchar } },
{ CONTROL('K'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_eol } },
{ CONTROL('K'), { 0,     0,    0,   0 },  f_select,    { .m = m_eol } },
{ CONTROL('L'), { 0,     0,    0,   0 },  f_center,    { 0 } },
{ CONTROL('M'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\n" } },
{ CONTROL('M'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ CONTROL('N'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ CONTROL('O'), { t_sel, 0,    0,   0 },  f_select,    { .m = m_tosel } }, /* Swap fsel and fcur */
{ CONTROL('O'), { 0,     0,    0,   0 },  f_move,      { .m = m_tomark } },
{ CONTROL('P'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevline } },
{ CONTROL('Q'), { t_warn,t_mod,0,   0 },  f_toggle,    { .i = S_Running } },
{ CONTROL('Q'), { t_mod, 0,    0,   0 },  f_warn,      { .v = "WARNING! File modified!!!" } },
{ CONTROL('Q'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ CONTROL('R'), { t_sel, 0,    0,   0 },  f_findbw,    { 0 } },
{ CONTROL('R'), { 0,     0,    0,   0 },  f_spawn,     FINDBW },
{ CONTROL('S'), { t_sel, 0,    0,   0 },  f_findfw,    { 0 } },
{ CONTROL('S'), { 0,     0,    0,   0 },  f_spawn,     FIND },
{ CONTROL('T'), { 0,     0,    0,   0 },  f_pipero ,   TOCLIP },
{ CONTROL('U'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('U'), { t_bol, t_rw, 0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('U'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_bol } },
{ CONTROL('U'), { 0,     0,    0,   0 },  f_select,    { .m = m_bol } },
{ CONTROL('V'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_InsEsc } },
{ CONTROL('W'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('W'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevword } },
{ CONTROL('W'), { 0,     0,    0,   0 },  f_select,    { .m = m_prevword } },
{ CONTROL('X'), { t_mod ,0,    0,   0 },  f_save,      { 0 } },
{ CONTROL('X'), { 0     ,0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ CONTROL('Y'), { t_rw,  0,    0,   0 },  f_pipe,      FROMCLIP },
{ CONTROL('Z'), { 0     ,0,    0,   0 },  f_suspend,   { 0 } },
{ CONTROL('['), { 0,     0,    0,   0 },  f_spawn,     CMD_P },  /* TODO: Sam's? */
{ CONTROL('\\'),{ 0,     0,    0,   0 },  f_spawn,     PIPE },
{ CONTROL(']'), { 0,     0,    0,   0 },  f_extsel,    { .i = ExtDefault } },
{ CONTROL('^'), { t_redo,t_rw, 0,   0 },  f_undo,      { .i = -1 } },
/*{ CONTROL('^'), { t_rw,  0,    0,   0 },  f_undo,      { .i = 0 } }, */ /* TODO: repeat, implement */
{ CONTROL('_'), { t_undo,t_rw, 0,   0 },  f_undo,      { .i = 1 } },
{ CONTROL('?'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ CONTROL('?'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ CONTROL('?'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevchar } },
};

/* Commands read at the fifo */
static Command cmds[] = { /* Use only f_ funcs that take Arg.v */
/* \0, regex,             tests,        func */
{NULL, "^([0-9]+)$",      { 0,     0 }, f_line   },
{NULL, "^/(.*)$",         { 0,     0 }, f_findfw },
{NULL, "^\\?(.*)$",       { 0,     0 }, f_findbw },
{NULL, "^\\|[ \t]*(.*)$", { t_rw,  0 }, f_pipe   },
{NULL, "^\\|[ \t]*(.*)$", { 0,     0 }, f_pipero },
{NULL, "^w[ \t]*(.*)$",   { 0,     0 }, f_save   },
{NULL, "^syntax (.*)$",   { 0,     0 }, f_syntax },
{NULL, "^offset (.*)$",   { 0,     0 }, f_offset },
};

/* Syntax color definition */
#define B "(^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|$)"

static Syntax syntaxes[] = {
{"c", NULL, "\\.(c(pp|xx)?|h(pp|xx)?|cc)$", { NULL }, {
	/* HiRed   */  "",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
	/* LoGreen */  B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|namespace|template|public|protected|private|typename|this|friend|virtual|using|mutable|volatile|register|explicit)"B,
	/* HiMag   */  B"(goto|continue|break|return)"B,
	/* LoMag   */  "(^#(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma))|"B"\\<[A-Z_][0-9A-Z_]+\\>"B"",
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoRed   */  "(\"(\\\\.|[^\"])*\")",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	} },

{"sh", NULL, "\\.sh$", { NULL }, {
	/* HiRed   */  "",
	/* HiGreen */  "^[0-9A-Z_]+\\(\\)",
	/* LoGreen */  B"(case|do|done|elif|else|esac|exit|fi|for|function|if|in|local|read|return|select|shift|then|time|until|while)"B,
	/* HiMag   */  "",
	/* LoMag   */  "\"(\\\\.|[^\"])*\"",
	/* HiBlue  */  "(\\{|\\}|\\(|\\)|\\;|\\]|\\[|`|\\\\|\\$|<|>|!|=|&|\\|)",
	/* LoRed   */  "\\$\\{?[0-9A-Z_!@#$*?-]+\\}?",
	/* LoBlue  */  "#.*$",
	} },

{"makefile", NULL, "(Makefile[^/]*|\\.mk)$", { NULL }, {
	/* HiRed   */  "",
	/* HiGreen */  "",
	/* LoGreen */  "\\$+[{(][a-zA-Z0-9_-]+[})]",
	/* HiMag   */  B"(if|ifeq|else|endif)"B,
	/* LoMag   */  "",
	/* HiBlue  */  "^[^ 	]+:",
	/* LoRed   */  "[:=]",
	/* LoBlue  */  "#.*$",
	} },

{"man", NULL, "\\.[1-9]x?$", { NULL }, {
	/* HiRed   */  "\\.(BR?|I[PR]?).*$",
	/* HiGreen */  "",
	/* LoGreen */  "\\.(S|T)H.*$",
	/* HiMag   */  "\\.(br|DS|RS|RE|PD)",
	/* LoMag   */  "(\\.(S|T)H|\\.TP)",
	/* HiBlue  */  "\\.(BR?|I[PR]?|PP)",
	/* LoRed   */  "",
	/* LoBlue  */  "\\\\f[BIPR]",
	} },

{"vala", NULL, "\\.(vapi|vala)$", { NULL }, {
	/* HiRed   */  B"[A-Z_][0-9A-Z_]+\\>",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch|get|set|value|out|ref|enum)"B,
	/* LoGreen */  B"(uint|uint8|uint16|uint32|uint64|bool|byte|ssize_t|size_t|char|double|string|float|int|long|short|this|base|transient|void|true|false|null|unowned|owned)"B,
	/* HiMag   */  B"(try|catch|throw|finally|continue|break|return|new|sizeof|signal|delegate)"B,
	/* LoMag   */  B"(abstract|class|final|implements|import|instanceof|interface|using|private|public|static|strictfp|super|throws)"B,
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoRed   */  "\"(\\\\.|[^\"])*\"",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	} },
{"java", NULL, "\\.java$", { NULL }, {
	/* HiRed   */  B"[A-Z_][0-9A-Z_]+\\>",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch)"B,
	/* LoGreen */  B"(boolean|byte|char|double|float|int|long|short|transient|void|true|false|null)"B,
	/* HiMag   */  B"(try|catch|throw|finally|continue|break|return|new)"B,
	/* LoMag   */  B"(abstract|class|extends|final|implements|import|instanceof|interface|native|package|private|protected|public|static|strictfp|this|super|synchronized|throws|volatile)"B,
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoRed   */  "\"(\\\\.|[^\"])*\"",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	} },
};

/* Colors */
static const short  fgcolors[LastFG] = {
	[DefFG]  = -1,
	[CurFG]  = (HILIGHT_CURRENT?COLOR_BLACK:-1),
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
	[CurBG] = (HILIGHT_CURRENT?COLOR_CYAN:-1),
	[SelBG] = COLOR_YELLOW,
};

