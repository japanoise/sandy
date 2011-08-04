/* A simplified way to customize */
#define HILIGHT_CURRENT 1
#define SHOW_NONPRINT   1
#define HILIGHT_SYNTAX  1

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
	"else printf \"\033[0;0H\033[7m"prompt"\033[K\033[0m \" >&2; read arg; fi &&" \
	"echo " cmd "\"$arg\" > ${SANDY_FIFO}", NULL } }

#define FIND   PROMPT("Find:",        "${SANDY_FIND}",   "/")
#define FINDBW PROMPT("Find (back):", "${SANDY_FIND}",   "?")
#define PIPE   PROMPT("Pipe:",        "${SANDY_PIPE}",   "!")
#define SAVEAS PROMPT("Save as:",     "${SANDY_FILE}",   "w")
#define CMD_P  PROMPT("Command:",     "/\n?\nw\n!\nsyntax\noffset", "")

/* Args to f_pipe / f_pipero */
/* TODO: make sandy-sel to wrap xsel or standalone */
#define TOCLIP   { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -ib || cat > /tmp/.sandy.clipboard.$USER" }
#define FROMCLIP { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -ob || cat /tmp/.sandy.clipboard.$USER" }
#define TOSEL    { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -i  || cat > /tmp/.sandy.selection.$USER" }
#define FROMSEL  { .v = "xsel -h >/dev/null 2>&1 && test -n \"$DISPLAY\" && xsel -o  || cat /tmp/.sandy.selection.$USER" }

/* Hooks are launched from the main code */
#define HOOK_SAVE_NO_FILE f_spawn (&(const Arg)SAVEAS)
#define HOOK_SELECT_MOUSE f_pipero(&(const Arg)TOSEL)
#undef  HOOK_DELETE_ALL   /* This affects every delete */
#undef  HOOK_SELECT_ALL   /* This affects every selection */

/* Key-bindings and stuff */
/* WARNING: use CONTROL(ch) ONLY with '@', (caps)A-Z, '[', '\', ']', '^', '_' or '?' */
/*          otherwise it may not mean what you think. See man 7 ascii for more info */
#define CONTROL(ch)   {(ch ^ 0x40)}
#define META(ch)      { 0x1B, ch }

static const Key curskeys[] = { /* Plain keys here, no CONTROL or META */
/* keyv.i,                  tests,                     func,      arg */
{ .keyv.i = KEY_BACKSPACE,  { t_rw,  0,    0,   0 },   f_delete,  { .m = m_prevchar } },
{ .keyv.i = KEY_DC,         { t_sel, t_rw, 0,   0 },   f_delete,  { .m = m_tosel    } },
{ .keyv.i = KEY_DC,         { t_rw,  0,    0,   0 },   f_delete,  { .m = m_nextchar } },
{ .keyv.i = KEY_IC,         { t_sel, 0,    0,   0 },   f_pipero,  TOCLIP },
{ .keyv.i = KEY_SDC,        { t_sel, t_rw, 0,   0 },   f_pipe,    TOCLIP },
{ .keyv.i = KEY_SIC,        { t_rw,  0,    0,   0 },   f_pipe,    FROMCLIP },
{ .keyv.i = KEY_HOME,       { 0,     0,    0,   0 },   f_moveb,   { .m = m_bol      } },
{ .keyv.i = KEY_END,        { 0,     0,    0,   0 },   f_moveb,   { .m = m_eol      } },
{ .keyv.i = KEY_SHOME,      { 0,     0,    0,   0 },   f_moveb,   { .m = m_bof      } },
{ .keyv.i = KEY_SEND,       { 0,     0,    0,   0 },   f_moveb,   { .m = m_eof      } },
{ .keyv.i = KEY_PPAGE,      { 0,     0,    0,   0 },   f_moveb,   { .m = m_prevscr  } },
{ .keyv.i = KEY_NPAGE,      { 0,     0,    0,   0 },   f_moveb,   { .m = m_nextscr  } },
{ .keyv.i = KEY_UP,         { 0,     0,    0,   0 },   f_moveb,   { .m = m_prevline } },
{ .keyv.i = KEY_DOWN,       { 0,     0,    0,   0 },   f_moveb,   { .m = m_nextline } },
{ .keyv.i = KEY_LEFT,       { 0,     0,    0,   0 },   f_moveb,   { .m = m_prevchar } },
{ .keyv.i = KEY_RIGHT,      { 0,     0,    0,   0 },   f_moveb,   { .m = m_nextchar } },
{ .keyv.i = KEY_SLEFT,      { 0,     0,    0,   0 },   f_moveb,   { .m = m_prevword } },
{ .keyv.i = KEY_SRIGHT,     { 0,     0,    0,   0 },   f_moveb,   { .m = m_nextword } },
};

static const Key stdkeys[] = {
/* keyv.c,                test,                     func,        arg */
{ .keyv.c = CONTROL('@'), { 0,     0,    0,   0 },  f_move,      { .m = m_tomark } },
{ .keyv.c = META(' '),    { 0,     0,    0,   0 },  f_mark,      { 0 } },
{ .keyv.c = META('`'),    { 0,     0,    0,   0 },  f_mark,      { 0 } },
{ .keyv.c = CONTROL('A'), { 0,     0,    0,   0 },  f_move,      { .m = m_bol } },
{ .keyv.c = CONTROL('B'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevchar } },
{ .keyv.c = META('b'),    { 0,     0,    0,   0 },  f_move,      { .m = m_prevword } },
{ .keyv.c = CONTROL('C'), { t_warn,t_mod,0,   0 },  f_toggle,    { .i = S_Running } },
{ .keyv.c = CONTROL('C'), { t_mod, 0,    0,   0 },  f_toggle,    { .i = S_Warned } },
{ .keyv.c = CONTROL('C'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ .keyv.c = META('c'),    { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "awk '{ for ( i=1; i <= NF; i++) { $i=tolower($i) ; sub(\".\", substr(toupper($i),1,1) , $i)  } print }'" } },
{ .keyv.c = CONTROL('D'), { t_sel, t_rw, 0,   0 },  f_pipe,      TOCLIP },
{ .keyv.c = CONTROL('D'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextchar } },
{ .keyv.c = META('d'),    { t_rw,  0,    0,   0 },  f_delete,    { .m = m_nextword } },
{ .keyv.c = CONTROL('E'), { 0,     0,    0,   0 },  f_move,      { .m = m_eol } },
{ .keyv.c = CONTROL('F'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextchar } },
{ .keyv.c = META('f'),    { 0,     0,    0,   0 },  f_move,      { .m = m_nextword } },
{ .keyv.c = CONTROL('G'), { t_sel, 0,    0,   0 },  f_select,    { .m = m_stay } },
{ .keyv.c = CONTROL('H'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ .keyv.c = CONTROL('I'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\t" } },
{ .keyv.c = CONTROL('J'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\n" } },
{ .keyv.c = CONTROL('J'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ .keyv.c = CONTROL('K'), { t_eol, t_rw, 0,   0 },  f_delete,    { .m = m_nextchar } },
{ .keyv.c = CONTROL('K'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_eol } },
{ .keyv.c = CONTROL('L'), { 0,     0,    0,   0 },  f_center,    { 0 } },
{ .keyv.c = META('l'),    { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "tr [A-Z] [a-z]" } },
{ .keyv.c = CONTROL('M'), { t_rw,  0,    0,   0 },  f_insert,    { .v = "\n" } },
{ .keyv.c = CONTROL('M'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ .keyv.c = CONTROL('N'), { 0,     0,    0,   0 },  f_move,      { .m = m_nextline } },
{ .keyv.c = CONTROL('O'), { t_sel, 0,    0,   0 },  f_select,    { .m = m_tosel } }, /* Swap fsel and fcur */
{ .keyv.c = CONTROL('P'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevline } },
{ .keyv.c = CONTROL('Q'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_InsEsc } },
{ .keyv.c = CONTROL('R'), { t_sel, 0,    0,   0 },  f_findbw,    { 0 } },
{ .keyv.c = CONTROL('R'), { 0,     0,    0,   0 },  f_spawn,     FINDBW },
{ .keyv.c = META('r'),    { 0,     0,    0,   0 },  f_findbw,    { 0 } },
{ .keyv.c = CONTROL('S'), { t_sel, 0,    0,   0 },  f_findfw,    { 0 } },
{ .keyv.c = CONTROL('S'), { 0,     0,    0,   0 },  f_spawn,     FIND },
{ .keyv.c = META('s'),    { 0,     0,    0,   0 },  f_findfw,    { 0 } },
{ .keyv.c = CONTROL('T'), { 0,     0,    0,   0 },  f_pipero ,   TOCLIP },
{ .keyv.c = CONTROL('U'), { t_bol, t_rw, 0,   0 },  f_delete,    { .m = m_prevchar } },
{ .keyv.c = CONTROL('U'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_bol } },
{ .keyv.c = META('u'),    { t_sel, t_rw, 0,   0 },  f_pipe,      { .v = "tr [a-z] [A-Z]" } },
{ .keyv.c = CONTROL('V'), { 0,     0,    0,   0 },  f_move,      { .m = m_prevscr } },
{ .keyv.c = META('v'),    { 0,     0,    0,   0 },  f_move,      { .m = m_nextscr } },
{ .keyv.c = CONTROL('W'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevword } },
{ .keyv.c = CONTROL('X'), { t_mod, 0,    0,   0 },  f_save,      { 0 } },
{ .keyv.c = CONTROL('X'), { 0,     0,    0,   0 },  f_toggle,    { .i = S_Running } },
{ .keyv.c = CONTROL('Y'), { t_rw,  0,    0,   0 },  f_pipe,      FROMCLIP },
{ .keyv.c = CONTROL('Z'), { 0     ,0,    0,   0 },  f_suspend,   { 0 } },
{ .keyv.c = CONTROL('['), { 0,     0,    0,   0 },  f_spawn,     CMD_P },  /* TODO: Sam's? */
{ .keyv.c = CONTROL('\\'),{ 0,     0,    0,   0 },  f_spawn,     PIPE },
{ .keyv.c = CONTROL(']'), { 0,     0,    0,   0 },  f_extsel,    { .i = ExtDefault } },
{ .keyv.c = CONTROL('^'), { t_redo,t_rw, 0,   0 },  f_undo,      { .i = -1 } },
{ .keyv.c = CONTROL('^'), { t_rw,  0,    0,   0 },  f_repeat,    { 0 } },
{ .keyv.c = CONTROL('_'), { t_undo,t_rw, 0,   0 },  f_undo,      { .i = 1 } },
{ .keyv.c = CONTROL('?'), { t_rw,  0,    0,   0 },  f_delete,    { .m = m_prevchar } },
{ .keyv.c = META(','),    { 0,     0,    0,   0 },  f_move,      { .m = m_bof } },
{ .keyv.c = META('.'),    { 0,     0,    0,   0 },  f_move,      { .m = m_eof } },
};

/* Commands read at the fifo */
static const Command cmds[] = { /* if(arg == 0) arg.v=regex_match */
/* regex,           tests,        func      arg */
{"^([0-9]+)$",      { 0,     0 }, f_line ,  { 0 } },
{"^/(.*)$",         { 0,     0 }, f_findfw, { 0 } },
{"^\\?(.*)$",       { 0,     0 }, f_findbw, { 0 } },
{"^![ \t]*(.*)$",   { t_rw,  0 }, f_pipe,   { 0 } },
{"^![ /t]*(.*)$",   { 0,     0 }, f_pipero, { 0 } },
{"^w[ \t]*(.*)$",   { 0,     0 }, f_save,   { 0 } },
{"^syntax (.*)$",   { 0,     0 }, f_syntax, { 0 } },
{"^offset (.*)$",   { 0,     0 }, f_offset, { 0 } },
{"^set icase$",     { 0,     0 }, f_toggle, { .i = S_CaseIns } },
{"^set ro$",        { 0,     0 }, f_toggle, { .i = S_Readonly } },
{"^q$",             { t_mod, 0 }, f_toggle, { .i = S_Warned } },
{"^q$",             { 0,     0 }, f_toggle, { .i = S_Running } },
{"^q!$",            { 0,     0 }, f_toggle, { .i = S_Running } },
};

/* Syntax color definition */
#define B "\\b"
/* #define B "^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|\\||$"  -- Use this if \b is not in your libc's regex implementation */

static const Syntax syntaxes[] = {
#if HILIGHT_SYNTAX
{"c", "\\.(c(pp|xx)?|h(pp|xx)?|cc)$", {
	/* HiRed   */  "",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
	/* LoGreen */  B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|namespace|template|public|protected|private|typename|this|friend|virtual|using|mutable|volatile|register|explicit)"B,
	/* HiMag   */  B"(goto|continue|break|return)"B,
	/* LoMag   */  "(^#(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma))|"B"[A-Z_][0-9A-Z_]+"B"",
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoRed   */  "(\"(\\\\.|[^\"])*\")",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	} },

{"sh", "\\.sh$", {
	/* HiRed   */  "",
	/* HiGreen */  "^[0-9A-Z_]+\\(\\)",
	/* LoGreen */  B"(case|do|done|elif|else|esac|exit|fi|for|function|if|in|local|read|return|select|shift|then|time|until|while)"B,
	/* HiMag   */  "",
	/* LoMag   */  "\"(\\\\.|[^\"])*\"",
	/* HiBlue  */  "(\\{|\\}|\\(|\\)|\\;|\\]|\\[|`|\\\\|\\$|<|>|!|=|&|\\|)",
	/* LoRed   */  "\\$\\{?[0-9A-Z_!@#$*?-]+\\}?",
	/* LoBlue  */  "#.*$",
	} },

{"makefile", "(Makefile[^/]*|\\.mk)$", {
	/* HiRed   */  "",
	/* HiGreen */  "",
	/* LoGreen */  "\\$+[{(][a-zA-Z0-9_-]+[})]",
	/* HiMag   */  B"(if|ifeq|else|endif)"B,
	/* LoMag   */  "",
	/* HiBlue  */  "^[^ 	]+:",
	/* LoRed   */  "[:=]",
	/* LoBlue  */  "#.*$",
	} },

{"man", "\\.[1-9]x?$", {
	/* HiRed   */  "\\.(BR?|I[PR]?).*$",
	/* HiGreen */  "",
	/* LoGreen */  "\\.(S|T)H.*$",
	/* HiMag   */  "\\.(br|DS|RS|RE|PD)",
	/* LoMag   */  "(\\.(S|T)H|\\.TP)",
	/* HiBlue  */  "\\.(BR?|I[PR]?|PP)",
	/* LoRed   */  "",
	/* LoBlue  */  "\\\\f[BIPR]",
	} },

{"vala", "\\.(vapi|vala)$", {
	/* HiRed   */  B"[A-Z_][0-9A-Z_]+\\>",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch|get|set|value|out|ref|enum)"B,
	/* LoGreen */  B"(uint|uint8|uint16|uint32|uint64|bool|byte|ssize_t|size_t|char|double|string|float|int|long|short|this|base|transient|void|true|false|null|unowned|owned)"B,
	/* HiMag   */  B"(try|catch|throw|finally|continue|break|return|new|sizeof|signal|delegate)"B,
	/* LoMag   */  B"(abstract|class|final|implements|import|instanceof|interface|using|private|public|static|strictfp|super|throws)"B,
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoRed   */  "\"(\\\\.|[^\"])*\"",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	} },
{"java", "\\.java$", {
	/* HiRed   */  B"[A-Z_][0-9A-Z_]+\\>",
	/* HiGreen */  B"(for|if|while|do|else|case|default|switch)"B,
	/* LoGreen */  B"(boolean|byte|char|double|float|int|long|short|transient|void|true|false|null)"B,
	/* HiMag   */  B"(try|catch|throw|finally|continue|break|return|new)"B,
	/* LoMag   */  B"(abstract|class|extends|final|implements|import|instanceof|interface|native|package|private|protected|public|static|strictfp|this|super|synchronized|throws|volatile)"B,
	/* HiBlue  */  "(\\(|\\)|\\{|\\}|\\[|\\])",
	/* LoRed   */  "\"(\\\\.|[^\"])*\"",
	/* LoBlue  */  "(//.*|/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	} },
#else  /* HILIGHT_SYNTAX */
{"", "\0", { "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0" } }
#endif /* HILIGHT_SYNTAX */
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

