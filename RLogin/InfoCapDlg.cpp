// InfoCapDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "InfoCapDlg.h"

// CInfoCapDlg ダイアログ

IMPLEMENT_DYNAMIC(CInfoCapDlg, CDialogExt)

CInfoCapDlg::CInfoCapDlg(CWnd* pParent /*=NULL*/)
	: CDialogExt(CInfoCapDlg::IDD, pParent)
{
	m_InfoIndex = 1;
	m_CapIndex  = 2;
	m_pIndex = &m_CapIndex;
}

CInfoCapDlg::~CInfoCapDlg()
{
}

void CInfoCapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogExt::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INFOLIST, m_List);
	DDX_Control(pDX, IDC_ENTRY, m_Entry);
}

BEGIN_MESSAGE_MAP(CInfoCapDlg, CDialogExt)
	ON_BN_CLICKED(IDC_LOADCAP, &CInfoCapDlg::OnBnClickedLoadcap)
	ON_BN_CLICKED(IDC_LOADINFO, &CInfoCapDlg::OnBnClickedLoadinfo)
	ON_CBN_SELCHANGE(IDC_ENTRY, &CInfoCapDlg::OnCbnSelchangeEntry)
	ON_COMMAND(IDM_CAP_INPORT, &CInfoCapDlg::OnCapInport)
	ON_COMMAND(IDM_CAP_EXPORT, &CInfoCapDlg::OnCapExport)
	ON_COMMAND(IDM_CAP_CLIPBORD, &CInfoCapDlg::OnCapClipbord)
	ON_COMMAND(IDM_INFO_INPORT, &CInfoCapDlg::OnInfoInport)
	ON_COMMAND(IDM_INFO_EXPORT, &CInfoCapDlg::OnInfoExport)
	ON_COMMAND(IDM_INFO_CLIPBORD, &CInfoCapDlg::OnInfoClipbord)
END_MESSAGE_MAP()

#define	TYPE_BOL	0
#define	TYPE_NUM	1
#define	TYPE_STR	2

static const struct _InfoCapTitle {
	int		type;
	LPCTSTR	name;
	LPCTSTR	info;
	LPCTSTR	cap;
	LPCTSTR	memo;
} InfoCap[] = {
	{ TYPE_BOL,	_T("auto_left_margin"),				_T("bw"),		_T("bw"),	_T("cub1 wraps from col-umn 0 to last column")	},
	{ TYPE_BOL,	_T("auto_right_margin"),			_T("am"),		_T("am"),	_T("terminal has auto-matic margins")	},
	{ TYPE_BOL,	_T("back_color_erase"),				_T("bce"),		_T("ut"),	_T("screen erased withbackground color")	},
	{ TYPE_BOL,	_T("can_change"),					_T("ccc"),		_T("cc"),	_T("terminal can re-define existing col-ors")	},
	{ TYPE_BOL,	_T("ceol_standout_glitch"),			_T("xhp"),		_T("xs"),	_T("standout not erasedby overwriting (hp)")	},
	{ TYPE_BOL,	_T("col_addr_glitch"),				_T("xhpa"),		_T("YA"),	_T("only positive motionfor hpa/mhpa caps")	},
	{ TYPE_BOL,	_T("cpi_changes_res"),				_T("cpix"),		_T("YF"),	_T("changing characterpitch changes reso-lution")	},
	{ TYPE_BOL,	_T("cr_cancels_micro_mode"),		_T("crxm"),		_T("YB"),	_T("using cr turns offmicro mode")	},
	{ TYPE_BOL,	_T("dest_tabs_magic_smso"),			_T("xt"),		_T("xt"),	_T("tabs destructive,magic so char(t1061)")	},
	{ TYPE_BOL,	_T("eat_newline_glitch"),			_T("xenl"),		_T("xn"),	_T("newline ignoredafter 80 cols (con-cept)")	},
	{ TYPE_BOL,	_T("erase_overstrike"),				_T("eo"),		_T("eo"),	_T("can erase over-strikes with a blank")	},
	{ TYPE_BOL,	_T("generic_type"),					_T("gn"),		_T("gn"),	_T("generic line type")	},
	{ TYPE_BOL,	_T("hard_copy"),					_T("hc"),		_T("hc"),	_T("hardcopy terminal")	},
	{ TYPE_BOL,	_T("hard_cursor"),					_T("chts"),		_T("HC"),	_T("cursor is hard tosee")	},
	{ TYPE_BOL,	_T("has_meta_key"),					_T("km"),		_T("km"),	_T("Has a meta key(i.e., sets 8th-bit)")	},
	{ TYPE_BOL,	_T("has_print_wheel"),				_T("daisy"),	_T("YC"),	_T("printer needs opera-tor to change char-acter set")	},
	{ TYPE_BOL,	_T("has_status_line"),				_T("hs"),		_T("hs"),	_T("has extra statusline")	},
	{ TYPE_BOL,	_T("hue_lightness_saturation"),		_T("hls"),		_T("hl"),	_T("terminal uses onlyHLS color notation(Tektronix)")	},
	{ TYPE_BOL,	_T("insert_null_glitch"),			_T("in"),		_T("in"),	_T("insert mode distin-guishes nulls")	},
	{ TYPE_BOL,	_T("lpi_changes_res"),				_T("lpix"),		_T("YG"),	_T("changing line pitchchanges resolution")	},
	{ TYPE_BOL,	_T("memory_above"),					_T("da"),		_T("da"),	_T("display may beretained above thescreen")	},
	{ TYPE_BOL,	_T("memory_below"),					_T("db"),		_T("db"),	_T("display may beretained below thescreen")	},
	{ TYPE_BOL,	_T("move_insert_mode"),				_T("mir"),		_T("mi"),	_T("safe to move whilein insert mode")	},
	{ TYPE_BOL,	_T("move_standout_mode"),			_T("msgr"),		_T("ms"),	_T("safe to move whilein standout mode")	},
	{ TYPE_BOL,	_T("needs_xon_xoff"),				_T("nxon"),		_T("nx"),	_T("padding will notwork, xon/xoffrequired")	},
	{ TYPE_BOL,	_T("no_esc_ctlc"),					_T("xsb"),		_T("xb"),	_T("beehive (f1=escape,f2=ctrl C)")	},
	{ TYPE_BOL,	_T("no_pad_char"),					_T("npc"),		_T("NP"),	_T("pad character doesnot exist")	},
	{ TYPE_BOL,	_T("non_dest_scroll_region"),		_T("ndscr"),	_T("ND"),	_T("scrolling region isnon-destructive")	},
	{ TYPE_BOL,	_T("non_rev_rmcup"),				_T("nrrmc"),	_T("NR"),	_T("smcup does notreverse rmcup")	},
	{ TYPE_BOL,	_T("over_strike"),					_T("os"),		_T("os"),	_T("terminal can over-strike")	},
	{ TYPE_BOL,	_T("prtr_silent"),					_T("mc5i"),		_T("5i"),	_T("printer will notecho on screen")	},
	{ TYPE_BOL,	_T("row_addr_glitch"),				_T("xvpa"),		_T("YD"),	_T("only positive motionfor vpa/mvpa caps")	},
	{ TYPE_BOL,	_T("semi_auto_right_margin"),		_T("sam"),		_T("YE"),	_T("printing in lastcolumn causes cr")	},
	{ TYPE_BOL,	_T("status_line_esc_ok"),			_T("eslok"),	_T("es"),	_T("escape can be usedon the status line")	},
	{ TYPE_BOL,	_T("tilde_glitch"),					_T("hz"),		_T("hz"),	_T("cannot print ~'s(hazeltine)")	},
	{ TYPE_BOL,	_T("transparent_underline"),		_T("ul"),		_T("ul"),	_T("underline characteroverstrikes")	},
	{ TYPE_BOL,	_T("xon_xoff"),						_T("xon"),		_T("xo"),	_T("terminal usesxon/xoff handshaking")	},
	{ TYPE_NUM,	_T("columns"),						_T("cols"),		_T("co"),	_T("number of columns ina line")	},
	{ TYPE_NUM,	_T("init_tabs"),					_T("it"),		_T("it"),	_T("tabs initially every# spaces")	},
	{ TYPE_NUM,	_T("label_height"),					_T("lh"),		_T("lh"),	_T("rows in each label")	},
	{ TYPE_NUM,	_T("label_width"),					_T("lw"),		_T("lw"),	_T("columns in eachlabel")	},
	{ TYPE_NUM,	_T("lines"),						_T("lines"),	_T("li"),	_T("number of lines onscreen or page")	},
	{ TYPE_NUM,	_T("lines_of_memory"),				_T("lm"),		_T("lm"),	_T("lines of memory if >line. 0 means varies")	},
	{ TYPE_NUM,	_T("magic_cookie_glitch"),			_T("xmc"),		_T("sg"),	_T("number of blankcharacters left bysmso or rmso")	},
	{ TYPE_NUM,	_T("max_attributes"),				_T("ma"),		_T("ma"),	_T("maximum combinedattributes terminalcan handle")	},
	{ TYPE_NUM,	_T("max_colors"),					_T("colors"),	_T("Co"),	_T("maximum number ofcolors on screen")	},
	{ TYPE_NUM,	_T("max_pairs"),					_T("pairs"),	_T("pa"),	_T("maximum number ofcolor-pairs on thescreen")	},
	{ TYPE_NUM,	_T("maximum_windows"),				_T("wnum"),		_T("MW"),	_T("maximum number ofdefineable windows")	},
	{ TYPE_NUM,	_T("no_color_video"),				_T("ncv"),		_T("NC"),	_T("video attributesthat cannot be usedwith colors")	},
	{ TYPE_NUM,	_T("num_labels"),					_T("nlab"),		_T("Nl"),	_T("number of labels onscreen")	},
	{ TYPE_NUM,	_T("padding_baud_rate"),			_T("pb"),		_T("pb"),	_T("lowest baud ratewhere padding needed")	},
	{ TYPE_NUM,	_T("virtual_terminal"),				_T("vt"),		_T("vt"),	_T("virtual terminalnumber (CB/unix)")	},
	{ TYPE_NUM,	_T("width_status_line"),			_T("wsl"),		_T("ws"),	_T("number of columns instatus line")	},
	{ TYPE_NUM,	_T("bit_image_entwining"),			_T("bitwin"),	_T("Yo"),	_T("number of passes foreach bit-image row")	},
	{ TYPE_NUM,	_T("bit_image_type"),				_T("bitype"),	_T("Yp"),	_T("type of bit-imagedevice")	},
	{ TYPE_NUM,	_T("buffer_capacity"),				_T("bufsz"),	_T("Ya"),	_T("numbers of bytesbuffered beforeprinting")	},
	{ TYPE_NUM,	_T("buttons"),						_T("btns"),		_T("BT"),	_T("number of buttons onmouse")	},
	{ TYPE_NUM,	_T("dot_horz_spacing"),				_T("spinh"),	_T("Yc"),	_T("spacing of dots hor-izontally in dotsper inch")	},
	{ TYPE_NUM,	_T("dot_vert_spacing"),				_T("spinv"),	_T("Yb"),	_T("spacing of pins ver-tically in pins perinch")	},
	{ TYPE_NUM,	_T("max_micro_address"),			_T("maddr"),	_T("Yd"),	_T("maximum value inmicro_..._address")	},
	{ TYPE_NUM,	_T("max_micro_jump"),				_T("mjump"),	_T("Ye"),	_T("maximum value inparm_..._micro")	},
	{ TYPE_NUM,	_T("micro_col_size"),				_T("mcs"),		_T("Yf"),	_T("character step sizewhen in micro mode")	},
	{ TYPE_NUM,	_T("micro_line_size"),				_T("mls"),		_T("Yg"),	_T("line step size whenin micro mode")	},
	{ TYPE_NUM,	_T("number_of_pins"),				_T("npins"),	_T("Yh"),	_T("numbers of pins inprint-head")	},
	{ TYPE_NUM,	_T("output_res_char"),				_T("orc"),		_T("Yi"),	_T("horizontal resolu-tion in units perline")	},
	{ TYPE_NUM,	_T("output_res_horz_inch"),			_T("orhi"),		_T("Yk"),	_T("horizontal resolu-tion in units perinch")	},
	{ TYPE_NUM,	_T("output_res_line"),				_T("orl"),		_T("Yj"),	_T("vertical resolutionin units per line")	},
	{ TYPE_NUM,	_T("output_res_vert_inch"),			_T("orvi"),		_T("Yl"),	_T("vertical resolutionin units per inch")	},
	{ TYPE_NUM,	_T("print_rate"),					_T("cps"),		_T("Ym"),	_T("print rate in char-acters per second")	},
	{ TYPE_NUM,	_T("wide_char_size"),				_T("widcs"),	_T("Yn"),	_T("character step sizewhen in double widemode")	},
	{ TYPE_STR,	_T("acs_chars"),					_T("acsc"),		_T("ac"),	_T("graphics charsetpairs, based onvt100")	},
	{ TYPE_STR,	_T("back_tab"),						_T("cbt"),		_T("bt"),	_T("back tab (P)")	},
	{ TYPE_STR,	_T("bell"),							_T("bel"),		_T("bl"),	_T("audible signal(bell) (P)")	},
	{ TYPE_STR,	_T("carriage_return"),				_T("cr"),		_T("cr"),	_T("carriage return (P*)(P*)")	},
	{ TYPE_STR,	_T("change_char_pitch"),			_T("cpi"),		_T("ZA"),	_T("Change number ofcharacters per inchto #1")	},
	{ TYPE_STR,	_T("change_line_pitch"),			_T("lpi"),		_T("ZB"),	_T("Change number oflines per inch to #1")	},
	{ TYPE_STR,	_T("change_res_horz"),				_T("chr"),		_T("ZC"),	_T("Change horizontalresolution to #1")	},
	{ TYPE_STR,	_T("change_res_vert"),				_T("cvr"),		_T("ZD"),	_T("Change vertical res-olution to #1")	},
	{ TYPE_STR,	_T("change_scroll_region"),			_T("csr"),		_T("cs"),	_T("change region toline #1 to line #2(P)")	},
	{ TYPE_STR,	_T("char_padding"),					_T("rmp"),		_T("rP"),	_T("like ip but when ininsert mode")	},
	{ TYPE_STR,	_T("clear_all_tabs"),				_T("tbc"),		_T("ct"),	_T("clear all tab stops(P)")	},
	{ TYPE_STR,	_T("clear_margins"),				_T("mgc"),		_T("MC"),	_T("clear right and leftsoft margins")	},
	{ TYPE_STR,	_T("clear_screen"),					_T("clear"),	_T("cl"),	_T("clear screen andhome cursor (P*)")	},
	{ TYPE_STR,	_T("clr_bol"),						_T("el1"),		_T("cb"),	_T("Clear to beginningof line")	},
	{ TYPE_STR,	_T("clr_eol"),						_T("el"),		_T("ce"),	_T("clear to end of line(P)")	},
	{ TYPE_STR,	_T("clr_eos"),						_T("ed"),		_T("cd"),	_T("clear to end ofscreen (P*)")	},
	{ TYPE_STR,	_T("column_address"),				_T("hpa"),		_T("ch"),	_T("horizontal position#1, absolute (P)")	},
	{ TYPE_STR,	_T("command_character"),			_T("cmdch"),	_T("CC"),	_T("terminal settablecmd character inprototype !?")	},
	{ TYPE_STR,	_T("create_window"),				_T("cwin"),		_T("CW"),	_T("define a window #1from #2,#3 to #4,#5")	},
	{ TYPE_STR,	_T("cursor_address"),				_T("cup"),		_T("cm"),	_T("move to row #1columns #2")	},
	{ TYPE_STR,	_T("cursor_down"),					_T("cud1"),		_T("do"),	_T("down one line")	},
	{ TYPE_STR,	_T("cursor_home"),					_T("home"),		_T("ho"),	_T("home cursor (if nocup)")	},
	{ TYPE_STR,	_T("cursor_invisible"),				_T("civis"),	_T("vi"),	_T("make cursor invisi-ble")	},
	{ TYPE_STR,	_T("cursor_left"),					_T("cub1"),		_T("le"),	_T("move left one space")	},
	{ TYPE_STR,	_T("cursor_mem_address"),			_T("mrcup"),	_T("CM"),	_T("memory relative cur-sor addressing, moveto row #1 columns #2")	},
	{ TYPE_STR,	_T("cursor_normal"),				_T("cnorm"),	_T("ve"),	_T("make cursor appearnormal (undocivis/cvvis)")	},
	{ TYPE_STR,	_T("cursor_right"),					_T("cuf1"),		_T("nd"),	_T("non-destructivespace (move rightone space)")	},
	{ TYPE_STR,	_T("cursor_to_ll"),					_T("ll"),		_T("ll"),	_T("last line, firstcolumn (if no cup)")	},
	{ TYPE_STR,	_T("cursor_up"),					_T("cuu1"),		_T("up"),	_T("up one line")	},
	{ TYPE_STR,	_T("cursor_visible"),				_T("cvvis"),	_T("vs"),	_T("make cursor veryvisible")	},
	{ TYPE_STR,	_T("define_char"),					_T("defc"),		_T("ZE"),	_T("Define a character#1, #2 dots wide,descender #3")	},
	{ TYPE_STR,	_T("delete_character"),				_T("dch1"),		_T("dc"),	_T("delete character(P*)")	},
	{ TYPE_STR,	_T("delete_line"),					_T("dl1"),		_T("dl"),	_T("delete line (P*)")	},
	{ TYPE_STR,	_T("dial_phone"),					_T("dial"),		_T("DI"),	_T("dial number #1")	},
	{ TYPE_STR,	_T("dis_status_line"),				_T("dsl"),		_T("ds"),	_T("disable status line")	},
	{ TYPE_STR,	_T("display_clock"),				_T("dclk"),		_T("DK"),	_T("display clock")	},
	{ TYPE_STR,	_T("down_half_line"),				_T("hd"),		_T("hd"),	_T("half a line down")	},
	{ TYPE_STR,	_T("ena_acs"),						_T("enacs"),	_T("eA"),	_T("enable alternatechar set")	},
	{ TYPE_STR,	_T("enter_alt_charset_mode"),		_T("smacs"),	_T("as"),	_T("start alternatecharacter set (P)")	},
	{ TYPE_STR,	_T("enter_am_mode"),				_T("smam"),		_T("SA"),	_T("turn on automaticmargins")	},
	{ TYPE_STR,	_T("enter_blink_mode"),				_T("blink"),	_T("mb"),	_T("turn on blinking")	},
	{ TYPE_STR,	_T("enter_bold_mode"),				_T("bold"),		_T("md"),	_T("turn on bold (extrabright) mode")	},
	{ TYPE_STR,	_T("enter_ca_mode"),				_T("smcup"),	_T("ti"),	_T("string to start pro-grams using cup")	},
	{ TYPE_STR,	_T("enter_delete_mode"),			_T("smdc"),		_T("dm"),	_T("enter delete mode")	},
	{ TYPE_STR,	_T("enter_dim_mode"),				_T("dim"),		_T("mh"),	_T("turn on half-brightmode")	},
	{ TYPE_STR,	_T("enter_doublewide_mode"),		_T("swidm"),	_T("ZF"),	_T("Enter double-widemode")	},
	{ TYPE_STR,	_T("enter_draft_quality"),			_T("sdrfq"),	_T("ZG"),	_T("Enter draft-qualitymode")	},
	{ TYPE_STR,	_T("enter_insert_mode"),			_T("smir"),		_T("im"),	_T("enter insert mode")	},
	{ TYPE_STR,	_T("enter_italics_mode"),			_T("sitm"),		_T("ZH"),	_T("Enter italic mode")	},
	{ TYPE_STR,	_T("enter_leftward_mode"),			_T("slm"),		_T("ZI"),	_T("Start leftward car-riage motion")	},
	{ TYPE_STR,	_T("enter_micro_mode"),				_T("smicm"),	_T("ZJ"),	_T("Start micro-motionmode")	},
	{ TYPE_STR,	_T("enter_near_letter_quality"),	_T("snlq"),		_T("ZK"),	_T("Enter NLQ mode")	},
	{ TYPE_STR,	_T("enter_normal_quality"),			_T("snrmq"),	_T("ZL"),	_T("Enter normal-qualitymode")	},
	{ TYPE_STR,	_T("enter_protected_mode"),			_T("prot"),		_T("mp"),	_T("turn on protectedmode")	},
	{ TYPE_STR,	_T("enter_reverse_mode"),			_T("rev"),		_T("mr"),	_T("turn on reversevideo mode")	},
	{ TYPE_STR,	_T("enter_secure_mode"),			_T("invis"),	_T("mk"),	_T("turn on blank mode(characters invisi-ble)")	},
	{ TYPE_STR,	_T("enter_shadow_mode"),			_T("sshm"),		_T("ZM"),	_T("Enter shadow-printmode")	},
	{ TYPE_STR,	_T("enter_standout_mode"),			_T("smso"),		_T("so"),	_T("begin standout mode")	},
	{ TYPE_STR,	_T("enter_subscript_mode"),			_T("ssubm"),	_T("ZN"),	_T("Enter subscript mode")	},
	{ TYPE_STR,	_T("enter_superscript_mode"),		_T("ssupm"),	_T("ZO"),	_T("Enter superscriptmode")	},
	{ TYPE_STR,	_T("enter_underline_mode"),			_T("smul"),		_T("us"),	_T("begin underline mode")	},
	{ TYPE_STR,	_T("enter_upward_mode"),			_T("sum"),		_T("ZP"),	_T("Start upward car-riage motion")	},
	{ TYPE_STR,	_T("enter_xon_mode"),				_T("smxon"),	_T("SX"),	_T("turn on xon/xoffhandshaking")	},
	{ TYPE_STR,	_T("erase_chars"),					_T("ech"),		_T("ec"),	_T("erase #1 characters(P)")	},
	{ TYPE_STR,	_T("exit_alt_charset_mode"),		_T("rmacs"),	_T("ae"),	_T("end alternate char-acter set (P)")	},
	{ TYPE_STR,	_T("exit_am_mode"),					_T("rmam"),		_T("RA"),	_T("turn off automaticmargins")	},
	{ TYPE_STR,	_T("exit_attribute_mode"),			_T("sgr0"),		_T("me"),	_T("turn off allattributes")	},
	{ TYPE_STR,	_T("exit_ca_mode"),					_T("rmcup"),	_T("te"),	_T("strings to end pro-grams using cup")	},
	{ TYPE_STR,	_T("exit_delete_mode"),				_T("rmdc"),		_T("ed"),	_T("end delete mode")	},
	{ TYPE_STR,	_T("exit_doublewide_mode"),			_T("rwidm"),	_T("ZQ"),	_T("End double-wide mode")	},
	{ TYPE_STR,	_T("exit_insert_mode"),				_T("rmir"),		_T("ei"),	_T("exit insert mode")	},
	{ TYPE_STR,	_T("exit_italics_mode"),			_T("ritm"),		_T("ZR"),	_T("End italic mode")	},
	{ TYPE_STR,	_T("exit_leftward_mode"),			_T("rlm"),		_T("ZS"),	_T("End left-motion mode")	},
	{ TYPE_STR,	_T("exit_micro_mode"),				_T("rmicm"),	_T("ZT"),	_T("End micro-motionmode")	},
	{ TYPE_STR,	_T("exit_shadow_mode"),				_T("rshm"),		_T("ZU"),	_T("End shadow-printmode")	},
	{ TYPE_STR,	_T("exit_standout_mode"),			_T("rmso"),		_T("se"),	_T("exit standout mode")	},
	{ TYPE_STR,	_T("exit_subscript_mode"),			_T("rsubm"),	_T("ZV"),	_T("End subscript mode")	},
	{ TYPE_STR,	_T("exit_superscript_mode"),		_T("rsupm"),	_T("ZW"),	_T("End superscript mode")	},
	{ TYPE_STR,	_T("exit_underline_mode"),			_T("rmul"),		_T("ue"),	_T("exit underline mode")	},
	{ TYPE_STR,	_T("exit_upward_mode"),				_T("rum"),		_T("ZX"),	_T("End reverse charac-ter motion")	},
	{ TYPE_STR,	_T("exit_xon_mode"),				_T("rmxon"),	_T("RX"),	_T("turn off xon/xoffhandshaking")	},
	{ TYPE_STR,	_T("fixed_pause"),					_T("pause"),	_T("PA"),	_T("pause for 2-3 sec-onds")	},
	{ TYPE_STR,	_T("flash_hook"),					_T("hook"),		_T("fh"),	_T("flash switch hook")	},
	{ TYPE_STR,	_T("flash_screen"),					_T("flash"),	_T("vb"),	_T("visible bell (maynot move cursor)")	},
	{ TYPE_STR,	_T("form_feed"),					_T("ff"),		_T("ff"),	_T("hardcopy terminalpage eject (P*)")	},
	{ TYPE_STR,	_T("from_status_line"),				_T("fsl"),		_T("fs"),	_T("return from statusline")	},
	{ TYPE_STR,	_T("goto_window"),					_T("wingo"),	_T("WG"),	_T("go to window #1")	},
	{ TYPE_STR,	_T("hangup"),						_T("hup"),		_T("HU"),	_T("hang-up phone")	},
	{ TYPE_STR,	_T("init_1string"),					_T("is1"),		_T("i1"),	_T("initializationstring")	},
	{ TYPE_STR,	_T("init_2string"),					_T("is2"),		_T("is"),	_T("initializationstring")	},
	{ TYPE_STR,	_T("init_3string"),					_T("is3"),		_T("i3"),	_T("initializationstring")	},
	{ TYPE_STR,	_T("init_file"),					_T("if"),		_T("if"),	_T("name of initializa-tion file")	},
	{ TYPE_STR,	_T("init_prog"),					_T("iprog"),	_T("iP"),	_T("path name of programfor initialization")	},
	{ TYPE_STR,	_T("initialize_color"),				_T("initc"),	_T("Ic"),	_T("initialize color #1to (#2,#3,#4)")	},
	{ TYPE_STR,	_T("initialize_pair"),				_T("initp"),	_T("Ip"),	_T("Initialize colorpair #1 tofg=(#2,#3,#4),bg=(#5,#6,#7)")	},
	{ TYPE_STR,	_T("insert_character"),				_T("ich1"),		_T("ic"),	_T("insert character (P)")	},
	{ TYPE_STR,	_T("insert_line"),					_T("il1"),		_T("al"),	_T("insert line (P*)")	},
	{ TYPE_STR,	_T("insert_padding"),				_T("ip"),		_T("ip"),	_T("insert padding afterinserted character")	},
	{ TYPE_STR,	_T("key_a1"),						_T("ka1"),		_T("K1"),	_T("upper left of keypad")	},
	{ TYPE_STR,	_T("key_a3"),						_T("ka3"),		_T("K3"),	_T("upper right of key-pad")	},
	{ TYPE_STR,	_T("key_b2"),						_T("kb2"),		_T("K2"),	_T("center of keypad")	},
	{ TYPE_STR,	_T("key_backspace"),				_T("kbs"),		_T("kb"),	_T("backspace key")	},
	{ TYPE_STR,	_T("key_beg"),						_T("kbeg"),		_T("@1"),	_T("begin key")	},
	{ TYPE_STR,	_T("key_btab"),						_T("kcbt"),		_T("kB"),	_T("back-tab key")	},
	{ TYPE_STR,	_T("key_c1"),						_T("kc1"),		_T("K4"),	_T("lower left of keypad")	},
	{ TYPE_STR,	_T("key_c3"),						_T("kc3"),		_T("K5"),	_T("lower right of key-pad")	},
	{ TYPE_STR,	_T("key_cancel"),					_T("kcan"),		_T("@2"),	_T("cancel key")	},
	{ TYPE_STR,	_T("key_catab"),					_T("ktbc"),		_T("ka"),	_T("clear-all-tabs key")	},
	{ TYPE_STR,	_T("key_clear"),					_T("kclr"),		_T("kC"),	_T("clear-screen orerase key")	},
	{ TYPE_STR,	_T("key_close"),					_T("kclo"),		_T("@3"),	_T("close key")	},
	{ TYPE_STR,	_T("key_command"),					_T("kcmd"),		_T("@4"),	_T("command key")	},
	{ TYPE_STR,	_T("key_copy"),						_T("kcpy"),		_T("@5"),	_T("copy key")	},
	{ TYPE_STR,	_T("key_create"),					_T("kcrt"),		_T("@6"),	_T("create key")	},
	{ TYPE_STR,	_T("key_ctab"),						_T("kctab"),	_T("kt"),	_T("clear-tab key")	},
	{ TYPE_STR,	_T("key_dc"),						_T("kdch1"),	_T("kD"),	_T("delete-character key")	},
	{ TYPE_STR,	_T("key_dl"),						_T("kdl1"),		_T("kL"),	_T("delete-line key")	},
	{ TYPE_STR,	_T("key_down"),						_T("kcud1"),	_T("kd"),	_T("down-arrow key")	},
	{ TYPE_STR,	_T("key_eic"),						_T("krmir"),	_T("kM"),	_T("sent by rmir or smirin insert mode")	},
	{ TYPE_STR,	_T("key_end"),						_T("kend"),		_T("@7"),	_T("end key")	},
	{ TYPE_STR,	_T("key_enter"),					_T("kent"),		_T("@8"),	_T("enter/send key")	},
	{ TYPE_STR,	_T("key_eol"),						_T("kel"),		_T("kE"),	_T("clear-to-end-of-linekey")	},
	{ TYPE_STR,	_T("key_eos"),						_T("ked"),		_T("kS"),	_T("clear-to-end-of-screen key")	},
	{ TYPE_STR,	_T("key_exit"),						_T("kext"),		_T("@9"),	_T("exit key")	},
	{ TYPE_STR,	_T("key_f0"),						_T("kf0"),		_T("k0"),	_T("F0 function key")	},
	{ TYPE_STR,	_T("key_f1"),						_T("kf1"),		_T("k1"),	_T("F1 function key")	},
	{ TYPE_STR,	_T("key_f10"),						_T("kf10"),		_T("k;"),	_T("F10 function key")	},
	{ TYPE_STR,	_T("key_f11"),						_T("kf11"),		_T("F1"),	_T("F11 function key")	},
	{ TYPE_STR,	_T("key_f12"),						_T("kf12"),		_T("F2"),	_T("F12 function key")	},
	{ TYPE_STR,	_T("key_f13"),						_T("kf13"),		_T("F3"),	_T("F13 function key")	},
	{ TYPE_STR,	_T("key_f14"),						_T("kf14"),		_T("F4"),	_T("F14 function key")	},
	{ TYPE_STR,	_T("key_f15"),						_T("kf15"),		_T("F5"),	_T("F15 function key")	},
	{ TYPE_STR,	_T("key_f16"),						_T("kf16"),		_T("F6"),	_T("F16 function key")	},
	{ TYPE_STR,	_T("key_f17"),						_T("kf17"),		_T("F7"),	_T("F17 function key")	},
	{ TYPE_STR,	_T("key_f18"),						_T("kf18"),		_T("F8"),	_T("F18 function key")	},
	{ TYPE_STR,	_T("key_f19"),						_T("kf19"),		_T("F9"),	_T("F19 function key")	},
	{ TYPE_STR,	_T("key_f2"),						_T("kf2"),		_T("k2"),	_T("F2 function key")	},
	{ TYPE_STR,	_T("key_f20"),						_T("kf20"),		_T("FA"),	_T("F20 function key")	},
	{ TYPE_STR,	_T("key_f21"),						_T("kf21"),		_T("FB"),	_T("F21 function key")	},
	{ TYPE_STR,	_T("key_f22"),						_T("kf22"),		_T("FC"),	_T("F22 function key")	},
	{ TYPE_STR,	_T("key_f23"),						_T("kf23"),		_T("FD"),	_T("F23 function key")	},
	{ TYPE_STR,	_T("key_f24"),						_T("kf24"),		_T("FE"),	_T("F24 function key")	},
	{ TYPE_STR,	_T("key_f25"),						_T("kf25"),		_T("FF"),	_T("F25 function key")	},
	{ TYPE_STR,	_T("key_f26"),						_T("kf26"),		_T("FG"),	_T("F26 function key")	},
	{ TYPE_STR,	_T("key_f27"),						_T("kf27"),		_T("FH"),	_T("F27 function key")	},
	{ TYPE_STR,	_T("key_f28"),						_T("kf28"),		_T("FI"),	_T("F28 function key")	},
	{ TYPE_STR,	_T("key_f29"),						_T("kf29"),		_T("FJ"),	_T("F29 function key")	},
	{ TYPE_STR,	_T("key_f3"),						_T("kf3"),		_T("k3"),	_T("F3 function key")	},
	{ TYPE_STR,	_T("key_f30"),						_T("kf30"),		_T("FK"),	_T("F30 function key")	},
	{ TYPE_STR,	_T("key_f31"),						_T("kf31"),		_T("FL"),	_T("F31 function key")	},
	{ TYPE_STR,	_T("key_f32"),						_T("kf32"),		_T("FM"),	_T("F32 function key")	},
	{ TYPE_STR,	_T("key_f33"),						_T("kf33"),		_T("FN"),	_T("F33 function key")	},
	{ TYPE_STR,	_T("key_f34"),						_T("kf34"),		_T("FO"),	_T("F34 function key")	},
	{ TYPE_STR,	_T("key_f35"),						_T("kf35"),		_T("FP"),	_T("F35 function key")	},
	{ TYPE_STR,	_T("key_f36"),						_T("kf36"),		_T("FQ"),	_T("F36 function key")	},
	{ TYPE_STR,	_T("key_f37"),						_T("kf37"),		_T("FR"),	_T("F37 function key")	},
	{ TYPE_STR,	_T("key_f38"),						_T("kf38"),		_T("FS"),	_T("F38 function key")	},
	{ TYPE_STR,	_T("key_f39"),						_T("kf39"),		_T("FT"),	_T("F39 function key")	},
	{ TYPE_STR,	_T("key_f4"),						_T("kf4"),		_T("k4"),	_T("F4 function key")	},
	{ TYPE_STR,	_T("key_f40"),						_T("kf40"),		_T("FU"),	_T("F40 function key")	},
	{ TYPE_STR,	_T("key_f41"),						_T("kf41"),		_T("FV"),	_T("F41 function key")	},
	{ TYPE_STR,	_T("key_f42"),						_T("kf42"),		_T("FW"),	_T("F42 function key")	},
	{ TYPE_STR,	_T("key_f43"),						_T("kf43"),		_T("FX"),	_T("F43 function key")	},
	{ TYPE_STR,	_T("key_f44"),						_T("kf44"),		_T("FY"),	_T("F44 function key")	},
	{ TYPE_STR,	_T("key_f45"),						_T("kf45"),		_T("FZ"),	_T("F45 function key")	},
	{ TYPE_STR,	_T("key_f46"),						_T("kf46"),		_T("Fa"),	_T("F46 function key")	},
	{ TYPE_STR,	_T("key_f47"),						_T("kf47"),		_T("Fb"),	_T("F47 function key")	},
	{ TYPE_STR,	_T("key_f48"),						_T("kf48"),		_T("Fc"),	_T("F48 function key")	},
	{ TYPE_STR,	_T("key_f49"),						_T("kf49"),		_T("Fd"),	_T("F49 function key")	},
	{ TYPE_STR,	_T("key_f5"),						_T("kf5"),		_T("k5"),	_T("F5 function key")	},
	{ TYPE_STR,	_T("key_f50"),						_T("kf50"),		_T("Fe"),	_T("F50 function key")	},
	{ TYPE_STR,	_T("key_f51"),						_T("kf51"),		_T("Ff"),	_T("F51 function key")	},
	{ TYPE_STR,	_T("key_f52"),						_T("kf52"),		_T("Fg"),	_T("F52 function key")	},
	{ TYPE_STR,	_T("key_f53"),						_T("kf53"),		_T("Fh"),	_T("F53 function key")	},
	{ TYPE_STR,	_T("key_f54"),						_T("kf54"),		_T("Fi"),	_T("F54 function key")	},
	{ TYPE_STR,	_T("key_f55"),						_T("kf55"),		_T("Fj"),	_T("F55 function key")	},
	{ TYPE_STR,	_T("key_f56"),						_T("kf56"),		_T("Fk"),	_T("F56 function key")	},
	{ TYPE_STR,	_T("key_f57"),						_T("kf57"),		_T("Fl"),	_T("F57 function key")	},
	{ TYPE_STR,	_T("key_f58"),						_T("kf58"),		_T("Fm"),	_T("F58 function key")	},
	{ TYPE_STR,	_T("key_f59"),						_T("kf59"),		_T("Fn"),	_T("F59 function key")	},
	{ TYPE_STR,	_T("key_f6"),						_T("kf6"),		_T("k6"),	_T("F6 function key")	},
	{ TYPE_STR,	_T("key_f60"),						_T("kf60"),		_T("Fo"),	_T("F60 function key")	},
	{ TYPE_STR,	_T("key_f61"),						_T("kf61"),		_T("Fp"),	_T("F61 function key")	},
	{ TYPE_STR,	_T("key_f62"),						_T("kf62"),		_T("Fq"),	_T("F62 function key")	},
	{ TYPE_STR,	_T("key_f63"),						_T("kf63"),		_T("Fr"),	_T("F63 function key")	},
	{ TYPE_STR,	_T("key_f7"),						_T("kf7"),		_T("k7"),	_T("F7 function key")	},
	{ TYPE_STR,	_T("key_f8"),						_T("kf8"),		_T("k8"),	_T("F8 function key")	},
	{ TYPE_STR,	_T("key_f9"),						_T("kf9"),		_T("k9"),	_T("F9 function key")	},
	{ TYPE_STR,	_T("key_find"),						_T("kfnd"),		_T("@0"),	_T("find key")	},
	{ TYPE_STR,	_T("key_help"),						_T("khlp"),		_T("%1"),	_T("help key")	},
	{ TYPE_STR,	_T("key_home"),						_T("khome"),	_T("kh"),	_T("home key")	},
	{ TYPE_STR,	_T("key_ic"),						_T("kich1"),	_T("kI"),	_T("insert-character key")	},
	{ TYPE_STR,	_T("key_il"),						_T("kil1"),		_T("kA"),	_T("insert-line key")	},
	{ TYPE_STR,	_T("key_left"),						_T("kcub1"),	_T("kl"),	_T("left-arrow key")	},
	{ TYPE_STR,	_T("key_ll"),						_T("kll"),		_T("kH"),	_T("lower-left key (homedown)")	},
	{ TYPE_STR,	_T("key_mark"),						_T("kmrk"),		_T("%2"),	_T("mark key")	},
	{ TYPE_STR,	_T("key_message"),					_T("kmsg"),		_T("%3"),	_T("message key")	},
	{ TYPE_STR,	_T("key_move"),						_T("kmov"),		_T("%4"),	_T("move key")	},
	{ TYPE_STR,	_T("key_next"),						_T("knxt"),		_T("%5"),	_T("next key")	},
	{ TYPE_STR,	_T("key_npage"),					_T("knp"),		_T("kN"),	_T("next-page key")	},
	{ TYPE_STR,	_T("key_open"),						_T("kopn"),		_T("%6"),	_T("open key")	},
	{ TYPE_STR,	_T("key_options"),					_T("kopt"),		_T("%7"),	_T("options key")	},
	{ TYPE_STR,	_T("key_ppage"),					_T("kpp"),		_T("kP"),	_T("previous-page key")	},
	{ TYPE_STR,	_T("key_previous"),					_T("kprv"),		_T("%8"),	_T("previous key")	},
	{ TYPE_STR,	_T("key_print"),					_T("kprt"),		_T("%9"),	_T("print key")	},
	{ TYPE_STR,	_T("key_redo"),						_T("krdo"),		_T("%0"),	_T("redo key")	},
	{ TYPE_STR,	_T("key_reference"),				_T("kref"),		_T("&1"),	_T("reference key")	},
	{ TYPE_STR,	_T("key_refresh"),					_T("krfr"),		_T("&2"),	_T("refresh key")	},
	{ TYPE_STR,	_T("key_replace"),					_T("krpl"),		_T("&3"),	_T("replace key")	},
	{ TYPE_STR,	_T("key_restart"),					_T("krst"),		_T("&4"),	_T("restart key")	},
	{ TYPE_STR,	_T("key_resume"),					_T("kres"),		_T("&5"),	_T("resume key")	},
	{ TYPE_STR,	_T("key_right"),					_T("kcuf1"),	_T("kr"),	_T("right-arrow key")	},
	{ TYPE_STR,	_T("key_save"),						_T("ksav"),		_T("&6"),	_T("save key")	},
	{ TYPE_STR,	_T("key_sbeg"),						_T("kBEG"),		_T("&9"),	_T("shifted begin key")	},
	{ TYPE_STR,	_T("key_scancel"),					_T("kCAN"),		_T("&0"),	_T("shifted cancel key")	},
	{ TYPE_STR,	_T("key_scommand"),					_T("kCMD"),		_T("*1"),	_T("shifted command key")	},
	{ TYPE_STR,	_T("key_scopy"),					_T("kCPY"),		_T("*2"),	_T("shifted copy key")	},
	{ TYPE_STR,	_T("key_screate"),					_T("kCRT"),		_T("*3"),	_T("shifted create key")	},
	{ TYPE_STR,	_T("key_sdc"),						_T("kDC"),		_T("*4"),	_T("shifted delete-char-acter key")	},
	{ TYPE_STR,	_T("key_sdl"),						_T("kDL"),		_T("*5"),	_T("shifted delete-linekey")	},
	{ TYPE_STR,	_T("key_select"),					_T("kslt"),		_T("*6"),	_T("select key")	},
	{ TYPE_STR,	_T("key_send"),						_T("kEND"),		_T("*7"),	_T("shifted end key")	},
	{ TYPE_STR,	_T("key_seol"),						_T("kEOL"),		_T("*8"),	_T("shifted clear-to-end-of-line key")	},
	{ TYPE_STR,	_T("key_sexit"),					_T("kEXT"),		_T("*9"),	_T("shifted exit key")	},
	{ TYPE_STR,	_T("key_sf"),						_T("kind"),		_T("kF"),	_T("scroll-forward key")	},
	{ TYPE_STR,	_T("key_sfind"),					_T("kFND"),		_T("*0"),	_T("shifted find key")	},
	{ TYPE_STR,	_T("key_shelp"),					_T("kHLP"),		_T("#1"),	_T("shifted help key")	},
	{ TYPE_STR,	_T("key_shome"),					_T("kHOM"),		_T("#2"),	_T("shifted home key")	},
	{ TYPE_STR,	_T("key_sic"),						_T("kIC"),		_T("#3"),	_T("shifted insert-char-acter key")	},
	{ TYPE_STR,	_T("key_sleft"),					_T("kLFT"),		_T("#4"),	_T("shifted left-arrowkey")	},
	{ TYPE_STR,	_T("key_smessage"),					_T("kMSG"),		_T("%a"),	_T("shifted message key")	},
	{ TYPE_STR,	_T("key_smove"),					_T("kMOV"),		_T("%b"),	_T("shifted move key")	},
	{ TYPE_STR,	_T("key_snext"),					_T("kNXT"),		_T("%c"),	_T("shifted next key")	},
	{ TYPE_STR,	_T("key_soptions"),					_T("kOPT"),		_T("%d"),	_T("shifted options key")	},
	{ TYPE_STR,	_T("key_sprevious"),				_T("kPRV"),		_T("%e"),	_T("shifted previous key")	},
	{ TYPE_STR,	_T("key_sprint"),					_T("kPRT"),		_T("%f"),	_T("shifted print key")	},
	{ TYPE_STR,	_T("key_sr"),						_T("kri"),		_T("kR"),	_T("scroll-backward key")	},
	{ TYPE_STR,	_T("key_sredo"),					_T("kRDO"),		_T("%g"),	_T("shifted redo key")	},
	{ TYPE_STR,	_T("key_sreplace"),					_T("kRPL"),		_T("%h"),	_T("shifted replace key")	},
	{ TYPE_STR,	_T("key_sright"),					_T("kRIT"),		_T("%i"),	_T("shifted right-arrowkey")	},
	{ TYPE_STR,	_T("key_srsume"),					_T("kRES"),		_T("%j"),	_T("shifted resume key")	},
	{ TYPE_STR,	_T("key_ssave"),					_T("kSAV"),		_T("!1"),	_T("shifted save key")	},
	{ TYPE_STR,	_T("key_ssuspend"),					_T("kSPD"),		_T("!2"),	_T("shifted suspend key")	},
	{ TYPE_STR,	_T("key_stab"),						_T("khts"),		_T("kT"),	_T("set-tab key")	},
	{ TYPE_STR,	_T("key_sundo"),					_T("kUND"),		_T("!3"),	_T("shifted undo key")	},
	{ TYPE_STR,	_T("key_suspend"),					_T("kspd"),		_T("&7"),	_T("suspend key")	},
	{ TYPE_STR,	_T("key_undo"),						_T("kund"),		_T("&8"),	_T("undo key")	},
	{ TYPE_STR,	_T("key_up"),						_T("kcuu1"),	_T("ku"),	_T("up-arrow key")	},
	{ TYPE_STR,	_T("keypad_local"),					_T("rmkx"),		_T("ke"),	_T("leave 'key-board_transmit' mode")	},
	{ TYPE_STR,	_T("keypad_xmit"),					_T("smkx"),		_T("ks"),	_T("enter 'key-board_transmit' mode")	},
	{ TYPE_STR,	_T("lab_f0"),						_T("lf0"),		_T("l0"),	_T("label on functionkey f0 if not f0")	},
	{ TYPE_STR,	_T("lab_f1"),						_T("lf1"),		_T("l1"),	_T("label on functionkey f1 if not f1")	},
	{ TYPE_STR,	_T("lab_f10"),						_T("lf10"),		_T("la"),	_T("label on functionkey f10 if not f10")	},
	{ TYPE_STR,	_T("lab_f2"),						_T("lf2"),		_T("l2"),	_T("label on functionkey f2 if not f2")	},
	{ TYPE_STR,	_T("lab_f3"),						_T("lf3"),		_T("l3"),	_T("label on functionkey f3 if not f3")	},
	{ TYPE_STR,	_T("lab_f4"),						_T("lf4"),		_T("l4"),	_T("label on functionkey f4 if not f4")	},
	{ TYPE_STR,	_T("lab_f5"),						_T("lf5"),		_T("l5"),	_T("label on functionkey f5 if not f5")	},
	{ TYPE_STR,	_T("lab_f6"),						_T("lf6"),		_T("l6"),	_T("label on functionkey f6 if not f6")	},
	{ TYPE_STR,	_T("lab_f7"),						_T("lf7"),		_T("l7"),	_T("label on functionkey f7 if not f7")	},
	{ TYPE_STR,	_T("lab_f8"),						_T("lf8"),		_T("l8"),	_T("label on functionkey f8 if not f8")	},
	{ TYPE_STR,	_T("lab_f9"),						_T("lf9"),		_T("l9"),	_T("label on functionkey f9 if not f9")	},
	{ TYPE_STR,	_T("label_format"),					_T("fln"),		_T("Lf"),	_T("label format")	},
	{ TYPE_STR,	_T("label_off"),					_T("rmln"),		_T("LF"),	_T("turn off soft labels")	},
	{ TYPE_STR,	_T("label_on"),						_T("smln"),		_T("LO"),	_T("turn on soft labels")	},
	{ TYPE_STR,	_T("meta_off"),						_T("rmm"),		_T("mo"),	_T("turn off meta mode")	},
	{ TYPE_STR,	_T("meta_on"),						_T("smm"),		_T("mm"),	_T("turn on meta mode(8th-bit on)")	},
	{ TYPE_STR,	_T("micro_column_address"),			_T("mhpa"),		_T("ZY"),	_T("Like column_addressin micro mode")	},
	{ TYPE_STR,	_T("micro_down"),					_T("mcud1"),	_T("ZZ"),	_T("Like cursor_down inmicro mode")	},
	{ TYPE_STR,	_T("micro_left"),					_T("mcub1"),	_T("Za"),	_T("Like cursor_left inmicro mode")	},
	{ TYPE_STR,	_T("micro_right"),					_T("mcuf1"),	_T("Zb"),	_T("Like cursor_right inmicro mode")	},
	{ TYPE_STR,	_T("micro_row_address"),			_T("mvpa"),		_T("Zc"),	_T("Like row_address #1in micro mode")	},
	{ TYPE_STR,	_T("micro_up"),						_T("mcuu1"),	_T("Zd"),	_T("Like cursor_up inmicro mode")	},
	{ TYPE_STR,	_T("newline"),						_T("nel"),		_T("nw"),	_T("newline (behave likecr followed by lf)")	},
	{ TYPE_STR,	_T("order_of_pins"),				_T("porder"),	_T("Ze"),	_T("Match software bitsto print-head pins")	},
	{ TYPE_STR,	_T("orig_colors"),					_T("oc"),		_T("oc"),	_T("Set all color pairsto the original ones")	},
	{ TYPE_STR,	_T("orig_pair"),					_T("op"),		_T("op"),	_T("Set default pair toits original value")	},
	{ TYPE_STR,	_T("pad_char"),						_T("pad"),		_T("pc"),	_T("padding char(instead of null)")	},
	{ TYPE_STR,	_T("parm_dch"),						_T("dch"),		_T("DC"),	_T("delete #1 characters(P*)")	},
	{ TYPE_STR,	_T("parm_delete_line"),				_T("dl"),		_T("DL"),	_T("delete #1 lines (P*)")	},
	{ TYPE_STR,	_T("parm_down_cursor"),				_T("cud"),		_T("DO"),	_T("down #1 lines (P*)")	},
	{ TYPE_STR,	_T("parm_down_micro"),				_T("mcud"),		_T("Zf"),	_T("Like parm_down_cur-sor in micro mode")	},
	{ TYPE_STR,	_T("parm_ich"),						_T("ich"),		_T("IC"),	_T("insert #1 characters(P*)")	},
	{ TYPE_STR,	_T("parm_index"),					_T("indn"),		_T("SF"),	_T("scroll forward #1lines (P)")	},
	{ TYPE_STR,	_T("parm_insert_line"),				_T("il"),		_T("AL"),	_T("insert #1 lines (P*)")	},
	{ TYPE_STR,	_T("parm_left_cursor"),				_T("cub"),		_T("LE"),	_T("move #1 charactersto the left (P)")	},
	{ TYPE_STR,	_T("parm_left_micro"),				_T("mcub"),		_T("Zg"),	_T("Like parm_left_cur-sor in micro mode")	},
	{ TYPE_STR,	_T("parm_right_cursor"),			_T("cuf"),		_T("RI"),	_T("move #1 charactersto the right (P*)")	},
	{ TYPE_STR,	_T("parm_right_micro"),				_T("mcuf"),		_T("Zh"),	_T("Like parm_right_cur-sor in micro mode")	},
	{ TYPE_STR,	_T("parm_rindex"),					_T("rin"),		_T("SR"),	_T("scroll back #1 lines(P)")	},
	{ TYPE_STR,	_T("parm_up_cursor"),				_T("cuu"),		_T("UP"),	_T("up #1 lines (P*)")	},
	{ TYPE_STR,	_T("parm_up_micro"),				_T("mcuu"),		_T("Zi"),	_T("Like parm_up_cursorin micro mode")	},
	{ TYPE_STR,	_T("pkey_key"),						_T("pfkey"),	_T("pk"),	_T("program function key#1 to type string #2")	},
	{ TYPE_STR,	_T("pkey_local"),					_T("pfloc"),	_T("pl"),	_T("program function key#1 to execute string#2")	},
	{ TYPE_STR,	_T("pkey_xmit"),					_T("pfx"),		_T("px"),	_T("program function key#1 to transmitstring #2")	},
	{ TYPE_STR,	_T("plab_norm"),					_T("pln"),		_T("pn"),	_T("program label #1 toshow string #2")	},
	{ TYPE_STR,	_T("print_screen"),					_T("mc0"),		_T("ps"),	_T("print contents ofscreen")	},
	{ TYPE_STR,	_T("prtr_non"),						_T("mc5p"),		_T("pO"),	_T("turn on printer for#1 bytes")	},
	{ TYPE_STR,	_T("prtr_off"),						_T("mc4"),		_T("pf"),	_T("turn off printer")	},
	{ TYPE_STR,	_T("prtr_on"),						_T("mc5"),		_T("po"),	_T("turn on printer")	},
	{ TYPE_STR,	_T("pulse"),						_T("pulse"),	_T("PU"),	_T("select pulse dialing")	},
	{ TYPE_STR,	_T("quick_dial"),					_T("qdial"),	_T("QD"),	_T("dial number #1 with-out checking")	},
	{ TYPE_STR,	_T("remove_clock"),					_T("rmclk"),	_T("RC"),	_T("remove clock")	},
	{ TYPE_STR,	_T("repeat_char"),					_T("rep"),		_T("rp"),	_T("repeat char #1 #2times (P*)")	},
	{ TYPE_STR,	_T("req_for_input"),				_T("rfi"),		_T("RF"),	_T("send next input char(for ptys)")	},
	{ TYPE_STR,	_T("reset_1string"),				_T("rs1"),		_T("r1"),	_T("reset string")	},
	{ TYPE_STR,	_T("reset_2string"),				_T("rs2"),		_T("r2"),	_T("reset string")	},
	{ TYPE_STR,	_T("reset_3string"),				_T("rs3"),		_T("r3"),	_T("reset string")	},
	{ TYPE_STR,	_T("reset_file"),					_T("rf"),		_T("rf"),	_T("name of reset file")	},
	{ TYPE_STR,	_T("restore_cursor"),				_T("rc"),		_T("rc"),	_T("restore cursor toposition of lastsave_cursor")	},
	{ TYPE_STR,	_T("row_address"),					_T("vpa"),		_T("cv"),	_T("vertical position #1absolute (P)")	},
	{ TYPE_STR,	_T("save_cursor"),					_T("sc"),		_T("sc"),	_T("save current cursorposition (P)")	},
	{ TYPE_STR,	_T("scroll_forward"),				_T("ind"),		_T("sf"),	_T("scroll text up (P)")	},
	{ TYPE_STR,	_T("scroll_reverse"),				_T("ri"),		_T("sr"),	_T("scroll text down (P)")	},
	{ TYPE_STR,	_T("select_char_set"),				_T("scs"),		_T("Zj"),	_T("Select characterset, #1")	},
	{ TYPE_STR,	_T("set_attributes"),				_T("sgr"),		_T("sa"),	_T("define videoattributes #1-#9(PG9)")	},
	{ TYPE_STR,	_T("set_background"),				_T("setb"),		_T("Sb"),	_T("Set background color#1")	},
	{ TYPE_STR,	_T("set_bottom_margin"),			_T("smgb"),		_T("Zk"),	_T("Set bottom margin atcurrent line")	},
	{ TYPE_STR,	_T("set_bottom_margin_parm"),		_T("smgbp"),	_T("Zl"),	_T("Set bottom margin atline #1 or (if smgtpis not given) #2lines from bottom")	},
	{ TYPE_STR,	_T("set_clock"),					_T("sclk"),		_T("SC"),	_T("set clock, #1 hrs #2mins #3 secs")	},
	{ TYPE_STR,	_T("set_color_pair"),				_T("scp"),		_T("sp"),	_T("Set current colorpair to #1")	},
	{ TYPE_STR,	_T("set_foreground"),				_T("setf"),		_T("Sf"),	_T("Set foreground color#1")	},
	{ TYPE_STR,	_T("set_left_margin"),				_T("smgl"),		_T("ML"),	_T("set left soft marginat current column.See smgl. (ML is notin BSD termcap).")	},
	{ TYPE_STR,	_T("set_left_margin_parm"),			_T("smglp"),	_T("Zm"),	_T("Set left (right)margin at column #1")	},
	{ TYPE_STR,	_T("set_right_margin"),				_T("smgr"),		_T("MR"),	_T("set right soft mar-gin at current col-umn")	},
	{ TYPE_STR,	_T("set_right_margin_parm"),		_T("smgrp"),	_T("Zn"),	_T("Set right margin atcolumn #1")	},
	{ TYPE_STR,	_T("set_tab"),						_T("hts"),		_T("st"),	_T("set a tab in everyrow, current columns")	},
	{ TYPE_STR,	_T("set_top_margin"),				_T("smgt"),		_T("Zo"),	_T("Set top margin atcurrent line")	},
	{ TYPE_STR,	_T("set_top_margin_parm"),			_T("smgtp"),	_T("Zp"),	_T("Set top (bottom)margin at row #1")	},
	{ TYPE_STR,	_T("set_window"),					_T("wind"),		_T("wi"),	_T("current window islines #1-#2 cols#3-#4")	},
	{ TYPE_STR,	_T("start_bit_image"),				_T("sbim"),		_T("Zq"),	_T("Start printing bitimage graphics")	},
	{ TYPE_STR,	_T("start_char_set_def"),			_T("scsd"),		_T("Zr"),	_T("Start character setdefinition #1, with#2 characters in theset")	},
	{ TYPE_STR,	_T("stop_bit_image"),				_T("rbim"),		_T("Zs"),	_T("Stop printing bitimage graphics")	},
	{ TYPE_STR,	_T("stop_char_set_def"),			_T("rcsd"),		_T("Zt"),	_T("End definition ofcharacter set #1")	},
	{ TYPE_STR,	_T("subscript_characters"),			_T("subcs"),	_T("Zu"),	_T("List of subscript-able characters")	},
	{ TYPE_STR,	_T("superscript_characters"),		_T("supcs"),	_T("Zv"),	_T("List of superscript-able characters")	},
	{ TYPE_STR,	_T("tab"),							_T("ht"),		_T("ta"),	_T("tab to next 8-spacehardware tab stop")	},
	{ TYPE_STR,	_T("these_cause_cr"),				_T("docr"),		_T("Zw"),	_T("Printing any ofthese characterscauses CR")	},
	{ TYPE_STR,	_T("to_status_line"),				_T("tsl"),		_T("ts"),	_T("move to status line,column #1")	},
	{ TYPE_STR,	_T("tone"),							_T("tone"),		_T("TO"),	_T("select touch tonedialing")	},
	{ TYPE_STR,	_T("underline_char"),				_T("uc"),		_T("uc"),	_T("underline char andmove past it")	},
	{ TYPE_STR,	_T("up_half_line"),					_T("hu"),		_T("hu"),	_T("half a line up")	},
	{ TYPE_STR,	_T("user0"),						_T("u0"),		_T("u0"),	_T("User string #0")	},
	{ TYPE_STR,	_T("user1"),						_T("u1"),		_T("u1"),	_T("User string #1")	},
	{ TYPE_STR,	_T("user2"),						_T("u2"),		_T("u2"),	_T("User string #2")	},
	{ TYPE_STR,	_T("user3"),						_T("u3"),		_T("u3"),	_T("User string #3")	},
	{ TYPE_STR,	_T("user4"),						_T("u4"),		_T("u4"),	_T("User string #4")	},
	{ TYPE_STR,	_T("user5"),						_T("u5"),		_T("u5"),	_T("User string #5")	},
	{ TYPE_STR,	_T("user6"),						_T("u6"),		_T("u6"),	_T("User string #6")	},
	{ TYPE_STR,	_T("user7"),						_T("u7"),		_T("u7"),	_T("User string #7")	},
	{ TYPE_STR,	_T("user8"),						_T("u8"),		_T("u8"),	_T("User string #8")	},
	{ TYPE_STR,	_T("user9"),						_T("u9"),		_T("u9"),	_T("User string #9")	},
	{ TYPE_STR,	_T("wait_tone"),					_T("wait"),		_T("WA"),	_T("wait for dial-tone")	},
	{ TYPE_STR,	_T("xoff_character"),				_T("xoffc"),	_T("XF"),	_T("XOFF character")	},
	{ TYPE_STR,	_T("xon_character"),				_T("xonc"),		_T("XN"),	_T("XON character")	},
	{ TYPE_STR,	_T("zero_motion"),					_T("zerom"),	_T("Zx"),	_T("No motion for subse-quent character")	},
	{ TYPE_STR,	_T("alt_scancode_esc"),				_T("scesa"),	_T("S8"),	_T("Alternate escapefor scancode emu-lation")	},
	{ TYPE_STR,	_T("bit_image_carriage_return"),	_T("bicr"),		_T("Yv"),	_T("Move to beginningof same row")	},
	{ TYPE_STR,	_T("bit_image_newline"),			_T("binel"),	_T("Zz"),	_T("Move to next rowof the bit image")	},
	{ TYPE_STR,	_T("bit_image_repeat"),				_T("birep"),	_T("Xy"),	_T("Repeat bit imagecell #1 #2 times")	},
	{ TYPE_STR,	_T("char_set_names"),				_T("csnm"),		_T("Zy"),	_T("Produce #1'th itemfrom list of char-acter set names")	},
	{ TYPE_STR,	_T("code_set_init"),				_T("csin"),		_T("ci"),	_T("Init sequence formultiple codesets")	},
	{ TYPE_STR,	_T("color_names"),					_T("colornm"),	_T("Yw"),	_T("Give name forcolor #1")	},
	{ TYPE_STR,	_T("define_bit_image_region"),		_T("defbi"),	_T("Yx"),	_T("Define rectan-gualar bit imageregion")	},
	{ TYPE_STR,	_T("device_type"),					_T("devt"),		_T("dv"),	_T("Indicate lan-guage/codeset sup-port")	},
	{ TYPE_STR,	_T("display_pc_char"),				_T("dispc"),	_T("S1"),	_T("Display PC charac-ter #1")	},
	{ TYPE_STR,	_T("end_bit_image_region"),			_T("endbi"),	_T("Yy"),	_T("End a bit-imageregion")	},
	{ TYPE_STR,	_T("enter_pc_charset_mode"),		_T("smpch"),	_T("S2"),	_T("Enter PC characterdisplay mode")	},
	{ TYPE_STR,	_T("enter_scancode_mode"),			_T("smsc"),		_T("S4"),	_T("Enter PC scancodemode")	},
	{ TYPE_STR,	_T("exit_pc_charset_mode"),			_T("rmpch"),	_T("S3"),	_T("Exit PC characterdisplay mode")	},
	{ TYPE_STR,	_T("exit_scancode_mode"),			_T("rmsc"),		_T("S5"),	_T("Exit PC scancodemode")	},
	{ TYPE_STR,	_T("get_mouse"),					_T("getm"),		_T("Gm"),	_T("Curses should getbutton events,parameter #1 notdocumented.")	},
	{ TYPE_STR,	_T("key_mouse"),					_T("kmous"),	_T("Km"),	_T("Mouse event hasoccurred")	},
	{ TYPE_STR,	_T("mouse_info"),					_T("minfo"),	_T("Mi"),	_T("Mouse statusinformation")	},
	{ TYPE_STR,	_T("pc_term_options"),				_T("pctrm"),	_T("S6"),	_T("PC terminaloptions")	},
	{ TYPE_STR,	_T("pkey_plab"),					_T("pfxl"),		_T("xl"),	_T("Program functionkey #1 to typestring #2 and showstring #3")	},
	{ TYPE_STR,	_T("req_mouse_pos"),				_T("reqmp"),	_T("RQ"),	_T("Request mouseposition")	},
	{ TYPE_STR,	_T("scancode_escape"),				_T("scesc"),	_T("S7"),	_T("Escape for scan-code emulation")	},
	{ TYPE_STR,	_T("set0_des_seq"),					_T("s0ds"),		_T("s0"),	_T("Shift to codeset 0(EUC set 0, ASCII)")	},
	{ TYPE_STR,	_T("set1_des_seq"),					_T("s1ds"),		_T("s1"),	_T("Shift to codeset 1")	},
	{ TYPE_STR,	_T("set2_des_seq"),					_T("s2ds"),		_T("s2"),	_T("Shift to codeset 2")	},
	{ TYPE_STR,	_T("set3_des_seq"),					_T("s3ds"),		_T("s3"),	_T("Shift to codeset 3")	},
	{ TYPE_STR,	_T("set_a_background"),				_T("setab"),	_T("AB"),	_T("Set backgroundcolor to #1, usingANSI escape")	},
	{ TYPE_STR,	_T("set_a_foreground"),				_T("setaf"),	_T("AF"),	_T("Set foregroundcolor to #1, usingANSI escape")	},
	{ TYPE_STR,	_T("set_color_band"),				_T("setcolor"),	_T("Yz"),	_T("Change to ribboncolor #1")	},
	{ TYPE_STR,	_T("set_page_length"),				_T("slines"),	_T("YZ"),	_T("Set page length to#1 lines")	},
	{ TYPE_STR,	_T("set_tb_margin"),				_T("smgtb"),	_T("MT"),	_T("Sets both top andbottom margins to#1, #2")	},
	{ TYPE_STR,	_T("enter_horizontal_hl_mode"),		_T("ehhlm"),	_T("Xh"),	_T("Enter horizontalhighlight mode")	},
	{ TYPE_STR,	_T("enter_left_hl_mode"),			_T("elhlm"),	_T("Xl"),	_T("Enter left highlightmode")	},
	{ TYPE_STR,	_T("enter_low_hl_mode"),			_T("elohlm"),	_T("Xo"),	_T("Enter low highlightmode")	},
	{ TYPE_STR,	_T("enter_right_hl_mode"),			_T("erhlm"),	_T("Xr"),	_T("Enter right high-light mode")	},
	{ TYPE_STR,	_T("enter_top_hl_mode"),			_T("ethlm"),	_T("Xt"),	_T("Enter top highlightmode")	},
	{ TYPE_STR,	_T("enter_vertical_hl_mode"),		_T("evhlm"),	_T("Xv"),	_T("Enter vertical high-light mode")	},
	{ TYPE_STR,	_T("set_a_attributes"),				_T("sgr1"),		_T("sA"),	_T("Define second set ofvideo attributes#1-#6")	},
	{ TYPE_STR,	_T("set_pglen_inch"),				_T("slengthsL"),_T("YI"),	_T("Set page lengthto #1 hundredth ofan inch")	},
	{ TYPE_STR,	_T("set_pglen_inch"),				_T("slength"),	_T("sL"),	_T("YI Set page lengthto #1 hundredth ofan inch")	},
	{ TYPE_STR,	_T("termcap_init2"),				_T("OTi2"),		_T("i2"),	_T("secondary initialization string")	},
	{ TYPE_STR,	_T("termcap_reset"),				_T("OTrs"),		_T("rs"),	_T("terminal reset string")	},
	{ TYPE_NUM,	_T("magic_cookie_glitch_ul"),		_T("OTug"),		_T("ug"),	_T("number of blanks left by ul")	},
	{ TYPE_BOL,	_T("backspaces_with_bs"),			_T("OTbs"),		_T("bs"),	_T("uses ^H to move left")	},
	{ TYPE_BOL,	_T("crt_no_scrolling"),				_T("OTns"),		_T("ns"),	_T("crt cannot scroll")	},
	{ TYPE_BOL,	_T("no_correctly_working_cr"),		_T("OTnc"),		_T("nc"),	_T("no way to go to start of line")	},
	{ TYPE_NUM,	_T("carriage_return_delay"),		_T("OTdC"),		_T("dC"),	_T("pad needed for CR")	},
	{ TYPE_NUM,	_T("new_line_delay"),				_T("OTdN"),		_T("dN"),	_T("pad needed for LF")	},
	{ TYPE_STR,	_T("linefeed_if_not_lf"),			_T("OTnl"),		_T("nl"),	_T("use to move down")	},
	{ TYPE_STR,	_T("backspace_if_not_bs"),			_T("OTbc"),		_T("bc"),	_T("move left, if not ^H")	},
	{ TYPE_BOL,	_T("linefeed_is_newline"),			_T("OTNL"),		_T("NL"),	_T("move down with \n")	},
	{ TYPE_NUM,	_T("backspace_delay"),				_T("OTdB"),		_T("dB"),	_T("padding required for ^H")	},
	{ TYPE_NUM,	_T("horizontal_tab_delay"),			_T("OTdT"),		_T("dT"),	_T("padding required for ^I")	},
	{ TYPE_NUM,	_T("number_of_function_keys"),		_T("OTkn"),		_T("kn"),	_T("count of function keys")	},
	{ TYPE_STR,	_T("other_non_function_keys"),		_T("OTko"),		_T("ko"),	_T("list of self-mapped keycaps")	},
	{ TYPE_BOL,	_T("has_hardware_tabs"),			_T("OTpt"),		_T("pt"),	_T("has 8-char tabs invoked with ^I")	},
	{ TYPE_BOL,	_T("return_does_clr_eol"),			_T("OTxr"),		_T("xr"),	_T("return clears the line")	},
	{ TYPE_STR,	_T("acs_ulcorner"),					_T("OTG2"),		_T("G2"),	_T("single upper left")	},
	{ TYPE_STR,	_T("acs_llcorner"),					_T("OTG3"),		_T("G3"),	_T("single lower left")	},
	{ TYPE_STR,	_T("acs_urcorner"),					_T("OTG1"),		_T("G1"),	_T("single upper right")	},
	{ TYPE_STR,	_T("acs_lrcorner"),					_T("OTG4"),		_T("G4"),	_T("single lower right")	},
	{ TYPE_STR,	_T("acs_ltee"),						_T("OTGR"),		_T("GR"),	_T("tee pointing right")	},
	{ TYPE_STR,	_T("acs_rtee"),						_T("OTGL"),		_T("GL"),	_T("tee pointing left")	},
	{ TYPE_STR,	_T("acs_btee"),						_T("OTGU"),		_T("GU"),	_T("tee pointing up")	},
	{ TYPE_STR,	_T("acs_ttee"),						_T("OTGD"),		_T("GD"),	_T("tee pointing down")	},
	{ TYPE_STR,	_T("acs_hline"),					_T("OTGH"),		_T("GH"),	_T("single horizontal line")	},
	{ TYPE_STR,	_T("acs_vline"),					_T("OTGV"),		_T("GV"),	_T("single vertical line")	},
	{ TYPE_STR,	_T("acs_plus"),						_T("OTGC"),		_T("GC"),	_T("single intersection")	},
	{ TYPE_STR,	_T("memory_lock"),					_T("meml"),		_T("ml"),	_T("lock memory above cursor")	},
	{ TYPE_STR,	_T("memory_unlock"),				_T("memu"),		_T("mu"),	_T("unlock memory")	},
	{ TYPE_STR,	_T("box_chars_1"),					_T("box1"),		_T("bx"),	_T("box characters primary set")	},
	{ (-1),		NULL,								NULL,			NULL,		NULL	},
};

static const LV_COLUMN InitListTab[6] = {
	{ LVCF_TEXT | LVCF_WIDTH, 0, 100, _T("Entry"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  50, _T("Info"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  40, _T("Cap"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0,  20, _T("Type"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 100, _T("Value"),	0, 0 },
	{ LVCF_TEXT | LVCF_WIDTH, 0, 250, _T("Memo"),	0, 0 },
};

static LPCTSTR	TypeStr[] = { _T(" "), _T("#"), _T("="), _T("@") };

/************************** Update 2018/01/31
static LPCTSTR	TermCap = _T("rlogin-color:")\
	_T("am:ut:xn:km:mi:ms:co#80:it#8:li#24:Co#256:pa#64:bt=\\E[Z:bl=^G:cr=^M:cs=\\E[%i%d;%dr:ct=\\E[3g:")\
	_T("cl=\\E[H\\E[2J:cb=\\E[1K:ce=\\E[K:cd=\\E[J:ch=\\E[%i%dG:cm=\\E[%i%d;%dH:do=^J:ho=\\E[H:vi=\\E[?25l:")\
	_T("le=^H:ve=\\E[?25h:nd=\\E[C:up=\\E[A:vs=\\E[?25h:dc=\\E[P:dl=\\E[M:as=\\E(0:SA=\\E[?7h:")\
	_T("mb=\\E[5m:md=\\E[1m:ti=\\E7\\E[?47h:mh=\\E[2m:im=\\E[4h:ZH=\\E[3m:mr=\\E[7m:mk=\\E[8m:so=\\E[7m:")\
	_T("us=\\E[4m:ec=\\E[%dX:ae=\\E(B:RA=\\E[?7l:me=\\E[m:te=\\E[2J\\E[?47l\\E8:ei=\\E[4l:ZR=\\E[23m:")\
	_T("se=\\E[27m:ue=\\E[24m:is=\\E[m\\E[?7h\\E[4l\\E>\\E7\\E[r\\E[?1;3;4;6l\\E8:al=\\E[L:kb=^H:kD=\\E[3~:")\
	_T("kd=\\EOB:@7=\\E[F:k1=\\EOP:k;=\\E[21~:F1=\\E[23~:F2=\\E[24~:k2=\\EOQ:k3=\\EOR:k4=\\EOS:")\
	_T("k5=\\E[15~:k6=\\E[17~:k7=\\E[18~:k8=\\E[19~:k9=\\E[20~:kh=\\E[H:kI=\\E[2~:kl=\\EOD:")\
	_T("Km=\\E[M:kN=\\E[6~:kP=\\E[5~:kr=\\EOC:ku=\\EOA:ke=\\E[?1l\\E>:ks=\\E[?1h\\E=:op=\\E[39;49m:")\
	_T("DC=\\E[%dP:DL=\\E[%dM:DO=\\E[%dB:IC=\\E[%d@:SF=\\E[%dS:AL=\\E[%dL:LE=\\E[%dD:RI=\\E[%dC:SR=\\E[%dT:")\
	_T("UP=\\E[%dA:r1=\\Ec:rc=\\E8:cv=\\E[%i%dd:sc=\\E7:sf=^J:sr=\\EM:AB=\\E[48;5;%dm:AF=\\E[38;5;%dm:st=\\EH:ta=^I:")\
	_T("bs:kn#12:pt:ml=\\El:mu=\\Em:");

	// Diff xterm-256color
static LPCTSTR	TermCap = _T("rlogin-color:")\
	_T("IC=\\E[%d@:RA=\\E[?7l:SA=\\E[?7h:SF=\\E[%dS:SR=\\E[%dT:ZH=\\E[3m:ZR=\\E[23m:bt=\\E[Z:cb=\\E[1K:")\
	_T("ch=\\E[%i%dG:cr=^M:cv=\\E[%i%dd:do=^J:ec=\\E[%dX:it#8:mb=\\E[5m:mh=\\E[2m:mk=\\E[8m:pt:r1=\\Ec:ta=^I:")\
	_T("ve=\\E[?25h:vi=\\E[?25l:vs=\\E[?25h:tc=xterm-256color:");
***************************/
static LPCTSTR	TermCap = _T("rlogin-color:")\
	_T("*6=\\EOF:@7=\\EOF:AB=\\E[48;5;%dm:AF=\\E[38;5;%dm:AL=\\E[%dL:AX:Co#256:DC=\\E[%dP:DL=\\E[%dM:")\
	_T("DO=\\E[%dB:F1=\\E[23~:F2=\\E[24~:IC=\\E[%d@:Km=\\E[M:LE=\\E[%dD:RA=\\E[?7l:RI=\\E[%dC:SA=\\E[?7h:")\
	_T("SF=\\E[%dS:SR=\\E[%dT:UP=\\E[%dA:XT:ZH=\\E[3m:ZR=\\E[23m:ae=\\E(B:al=\\E[L:am:as=\\E(0:bl=^G:")\
	_T("bs:bt=\\E[Z:cb=\\E[1K:cd=\\E[J:ce=\\E[K:ch=\\E[%i%dG:cl=\\E[H\\E[2J:cm=\\E[%i%d;%dH:co#80:cr=^M:")\
	_T("cs=\\E[%i%d;%dr:ct=\\E[3g:cv=\\E[%i%dd:dc=\\E[P:dl=\\E[M:do=^J:ec=\\E[%dX:ei=\\E[4l:ho=\\E[H:")\
	_T("im=\\E[4h:is=\\E[!p\\E[?3;4l\\E[4l\\E>\\E]104^G:it#8:k1=\\EOP:k2=\\EOQ:k3=\\EOR:k4=\\EOS:")\
	_T("k5=\\E[15~:k6=\\E[17~:k7=\\E[18~:k8=\\E[19~:k9=\\E[20~:k;=\\E[21~:kD=\\E[3~:kH=\\EOF:kI=\\E[2~:")\
	_T("kN=\\E[6~:kP=\\E[5~:kb=^H:kd=\\EOB:ke=\\E[?1l\\E>:kh=\\EOH:kl=\\EOD:km:kn#12:kr=\\EOC:ks=\\E[?1h\\E=:")\
	_T("ku=\\EOA:le=^H:li#24:mb=\\E[5m:md=\\E[1m:me=\\E[m:mh=\\E[2m:mi:mk=\\E[8m:ml=\\El:mr=\\E[7m:ms:")\
	_T("mu=\\Em:nd=\\E[C:op=\\E[39;49m:pa#65536:pt:r1=\\Ec:rc=\\E8:rs=\\E[!p\\E[?3;4l\\E[4l\\E>\\E]104^G:")\
	_T("sc=\\E7:se=\\E[27m:sf=^J:so=\\E[7m:sr=\\EM:st=\\EH:ta=^I:te=\\E[?1049l:ti=\\E[?1049h:ue=\\E[24m:")\
	_T("up=\\E[A:us=\\E[4m:ut:ve=\\E[?25h:vi=\\E[?25l:vs=\\E[?25h:xn:");

void CInfoCapDlg::SetNode(CStringIndex &cap, LPCTSTR p)
{
	int n;
	CStringArray tmp;
	CStringIndex use;

	while ( *p != _T('\0') ) {
		while ( *p == _T(' ') || *p == _T('\t') || *p == _T('\n') || *p == _T('\r') )
			p++;
		n = 0;
		tmp.RemoveAll();
		tmp.Add(_T(""));
		while ( *p != _T('\0') && *p != _T(':') ) {
			if ( p[0] == _T('\\') && _tcschr(_T("\\:#="), p[1]) != NULL ) {
				tmp[n] += *(p++);
				tmp[n] += *(p++);
			} else if ( n == 0 && (*p == _T('#') || *p == _T('=')) ) {
				tmp.Add(_T(""));
				n++;
				tmp[n] += *(p++);
			} else if ( *p == _T('\'') ) {
				tmp[n] += _T("\\047");
				p++;
			} else
				tmp[n] += *(p++);
		}
		if ( *p == _T(':') )
			p++;
		if ( tmp[0].IsEmpty() )
			continue;

		if ( tmp.GetSize() <= 1 )
			tmp.Add(_T(" "));

		if ( tmp[0].Right(1).Compare(_T("@")) == 0 ) {
			n = tmp[0].GetLength();
			tmp[0].Delete(n - 1);
			cap[tmp[0]] = _T("@");
			continue;
		}

		if ( !tmp[1].IsEmpty() && (tmp[0].Compare(_T("use")) == 0 || tmp[0].Compare(_T("tc")) == 0) ) {
			SetCapName(use, tmp[1].Mid(1));
			for ( n = 0 ; n < use.GetSize() ; n++ ) {
				if ( cap.Find(use[n].GetIndex()) < 0 )
					cap[use[n].GetIndex()] = (LPCTSTR)(use[n]);
			}
			continue;
		}

		if ( cap.Find(tmp[0]) >= 0 )
			continue;

		cap[tmp[0]] = tmp[1];
	}
}
void CInfoCapDlg::SetEntry(CStringIndex &cap, LPCTSTR entry)
{
	LPCTSTR p;

	cap.m_String.Empty();
	cap.RemoveAll();
	cap.SetNoCase(FALSE);
	m_pIndex = &m_CapIndex;

	if ( entry == NULL || entry[0] == _T('\0') )
		entry = TermCap;

	for ( p = entry ; *p == _T(' ') || *p == _T('\t') || *p == _T('\r') || *p == _T('\n') ; )
		p++;
	while ( *p != _T('\0') && *p != _T(':') )
		cap.m_String += *(p++);
	if ( *p == _T(':') )
		p++;

	SetNode(cap, p);
}
void CInfoCapDlg::SetCapName(CStringIndex &cap, LPCTSTR name)
{
	int n;

	cap.m_String = name;
	cap.RemoveAll();
	cap.SetNoCase(FALSE);

	if ( (n = m_CapName[name]) >= 0 && m_CapName[name].m_String.IsEmpty() ) {
		m_CapName[name].m_String = _T("X");
		SetNode(cap, m_CapNode[n]);
		m_CapName[name].m_String.Empty();
	}
}
void CInfoCapDlg::LoadCapFile(LPCTSTR filename)
{
	int n, i;
	FILE *fp;
	LPCTSTR w;
	CStringA str;
	CString uni;
	LPSTR p, s;
	CHAR tmp[BUFSIZ];
	CStringArray name;
	CWaitCursor wait;

	m_CapName.RemoveAll();
	m_CapNode.RemoveAll();
	m_pIndex = &m_CapIndex;

	if ( (fp = _tfopen(filename, _T("rt"))) == NULL )
		return;

	str.Empty();
	while ( fgets(tmp, BUFSIZ, fp) != NULL ) {
		for ( p = tmp ; *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ; )
			p++;
		if ( *p == '#' )
			continue;
		for ( s = p ; *s != '\0' && *s != '\r' && *s != '\n' ; )
			s++;
		if ( s > p && s[-1] == '\\' ) {
			s[-1] = '\0';
			str += p;
			continue;
		}
		if ( *s == '\0' ) {
			str += p;
			continue;
		}
		while ( s > p && (s[-1] == ' ' || s[-1] == '\t') )
			s--;
		*s = '\0';
		str += p;

		if ( str.IsEmpty() )
			continue;

#ifdef	_UNICODE
		uni = str;
		w = uni;
#else
		w = str;
#endif
		n = 0;
		name.RemoveAll();
		name.Add(_T(""));
		while ( *w != _T('\0') && *w != (':') ) {
			if ( *w == _T('|') ) {
				w++;
				name.Add(_T(""));
				n++;
			} else
				name[n] += *(w++);
		}
		if ( *w == _T(':') )
			w++;
		while ( *w == _T(' ') || *w == _T('\t') )
			w++;

		if ( *w != _T('\0') ) {
			i = (int)m_CapNode.Add(w);
			for ( n = 0 ; n < name.GetSize() ; n++ ) {
				if ( name[n].IsEmpty() )
					continue;
				m_CapName[name[n]] = i;
				m_CapName[name[n]].m_String.Empty();
				m_Entry.AddString(name[n]);
			}
		}

		str.Empty();
	}

	fclose(fp);
}
void CInfoCapDlg::LoadInfoFile(LPCTSTR filename)
{
	int n, i, c;
	FILE *fp;
	CStringA str;
	CString tmp, wrk, uni;
	LPCTSTR p, s;
	CStringArray name;
	CWaitCursor wait;

	m_CapName.RemoveAll();
	m_CapNode.RemoveAll();
	m_pIndex = &m_InfoIndex;

	if ( (fp = _tfopen(filename, _T("rt"))) == NULL )
		return;

	for ( c = 0 ; c != EOF ; ) {
		while ( (c = fgetc(fp)) != EOF ) {
			if ( c == '\n' ) {
				if ( (c = fgetc(fp)) == EOF )
					break;
				else if ( c != ' ' && c != '\t' ) {
					ungetc(c, fp);
					break;
				}
			} else if ( c != '\r' )
				str += (char)c;
		}

#ifdef	_UNICODE
		uni = str;
		p = uni;
#else
		p = str;
#endif
		if ( *p == _T('\0') || *p == _T(' ') || *p == _T('\t') || *p == _T('#') ) {
			str.Empty();
			continue;
		}
		
		n = 0;
		name.RemoveAll();
		name.Add(_T(""));
		while ( *p != _T('\0') && *p != _T(',') ) {
			if ( *p == _T('|') ) {
				p++;
				name.Add(_T(""));
				n++;
			} else
				name[n] += *(p++);
		}
		if ( *p == _T(',') )
			p++;
		while ( *p == _T(' ') || *p == _T('\t') )
			p++;

		tmp.Empty();
		while ( *p != _T('\0') ) {
			if ( p[0] == _T('\\') && p[1] == _T('\\') ) {
				tmp += *(p++);
				tmp += *(p++);
			} else if ( p[0] == _T('\\') && p[1] == _T(',') ) {
				tmp += _T(",");
				p += 2;
			} else if ( *p == _T(':') ) {
				tmp += _T("\\072");
				p++;
			} else if ( *p == _T(',') ) {
				tmp += _T(":");
				p++;
			} else if ( p[0] == _T('%') && p[1] == _T('%') ) {
				tmp += *(p++);
				tmp += *(p++);
			} else if ( p[0] == _T('%') && p[1] == _T('\'') ) {
				s = p + 2;
				n = GetOneChar(&s);
				if ( *s == _T('\'') ) {
					wrk.Format(_T("%%{%d}"), n);
					tmp += wrk;
					p = s + 1;
				} else
					tmp += *(p++);
			} else
				tmp += *(p++);
		}

		if ( !tmp.IsEmpty() ) {
			i = (int)m_CapNode.Add(tmp);
			for ( n = 0 ; n < name.GetSize() ; n++ ) {
				if ( name[n].IsEmpty() )
					continue;
				m_CapName[name[n]] = i;
				m_CapName[name[n]].m_String.Empty();
				m_Entry.AddString(name[n]);
			}
		}

		str.Empty();
	}

	fclose(fp);
}
void CInfoCapDlg::SetList(CStringIndex &cap, CStringBinary &index)
{
	int n, i;

	m_EntryName = cap.m_String;
	m_Entry.SetWindowText(m_EntryName);

	for ( n = 0 ; n < m_List.GetItemCount() ; n++ ) {
		if ( m_List.GetItemText(n, 0).IsEmpty() ) {
			m_List.DeleteItem(n);
			n--;
		} else if ( !m_List.GetItemText(n, 4).IsEmpty() )
			m_List.SetItemText(n, 4, _T(""));
	}

	for ( n = 0 ; n < cap.GetSize() ; n++ ) {
		if ( (i = index[cap[n].m_nIndex]) < 0 ) {
			i = m_List.GetItemCount();
			m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, i, _T(""), 0, 0, 0, i);
			m_List.SetItemText(i, index, cap[n].m_nIndex);
			switch(cap[n].m_String[0]) {
			case _T(' '):
				m_List.SetItemText(i, 3, _T(" "));
				m_List.SetItemText(i, 4, _T("Set"));
				break;
			case _T('#'):
			case _T('='):
				m_List.SetItemText(i, 3, cap[n].m_String.Left(1));
				m_List.SetItemText(i, 4, cap[n].m_String.Mid(1));
				break;
			}
		} else if ( (i = m_List.GetParamItem(i)) >= 0 ) {
			switch(cap[n].m_String[0]) {
			case _T(' '):
				m_List.SetItemText(i, 4, _T("Set"));
				break;
			case _T('#'):
			case _T('='):
				m_List.SetItemText(i, 4, cap[n].m_String.Mid(1));
				break;
			}
		}
	}

	m_List.DoSortItem();
}
LPCTSTR CInfoCapDlg::InfoToCap(LPCTSTR p, BOOL *pe)
{
	int n, a, t;
	int pos = 0;
	int inc = 0;
	BYTE tab[12];
	LPCTSTR s;
	CString tmp;
	LPCTSTR orig = p;
	static const struct _InfoToCapTab {
		LPCTSTR	info;
		LPCTSTR	cap;
		int		mode;	// 001=%i	002=%r
	} InfoToCapTab[] = {
		{ _T("%i%p1%d;%p2%d"),					_T("%i%d;%d"),		000	},
		{ _T("%i%p2%d;%p1%d"),					_T("%i%r%d;%d"),	002	},
		{ _T("%p1%d;%p2%d"),					_T("%d;%d"),		000	},
		{ _T("%p2%d;%p1%d"),					_T("%r%d;%d"),		002	},
		{ _T("%p1%{1}%+%d;%p2%{1}%+%d"),		_T("%d;%d"),		001	},
		{ _T("%p2%{1}%+%d;%p1%{1}%+%d"),		_T("%r%d;%d"),		003	},
		{ _T("%p1%{32}%+%c%p2%{32}%+%c"),		_T("%+ %+ "),		000	},
		{ _T("%p2%{32}%+%c%p1%{32}%+%c"),		_T("%r%+ %+ "),		002	},
		{ _T("%p1%{1}%+%d"),					_T("%d"),			001	},
		{ _T("%p1%{32}%+%c"),					_T("%+ "),			000	},
		{ _T("%p1%d"),							_T("%d"),			000	},
		{ _T("%p1%c"),							_T("%."),			000	},
		{ _T("%p2%{1}%+%d"),					_T("%r%d"),			003	},
		{ _T("%p2%{32}%+%c"),					_T("%r%+ "),		002	},
		{ _T("%p2%d"),							_T("%r%d"),			002	},
		{ _T("%p2%c"),							_T("%r%."),			002	},
		{ _T("%%"),								_T("%%"),			000	},
		{ _T("%i"),								_T("%i"),			000	},
		{ NULL,									NULL,				000	},
	};

	if ( m_pIndex->m_Value == 2 )
		return p;

	for ( n = 0 ; n < 12 ; n++ )
		tab[n] = _T('1') + n;

	m_WorkStr.Empty();
	while ( *p != _T('\0') ) {
		if ( *p == _T('%') ) {
			for ( n = 0 ; (s = InfoToCapTab[n].info) != NULL ; n++ ) {
				a = 0;
				tmp.Empty();
				while ( *s != _T('\0') ) {
					if ( s[0] == _T('%') && s[1] == _T('p') && s[2] == _T('1') ) {
						tmp += _T("%p");
						tmp += tab[pos];
						a++;
						s += 3;
					} else if ( s[0] == _T('%') && s[1] == _T('p') && s[2] == _T('2') ) {
						tmp += _T("%p");
						tmp += tab[pos + 1];
						a++;
						s += 3;
					} else {
						tmp += *(s++);
					}
				}
				if ( _tcsncmp(p, tmp, tmp.GetLength()) == 0 )
					break;
			}
			if ( InfoToCapTab[n].info != NULL ) {
				if ( (InfoToCapTab[n].mode & 004) != 0 ) {
					if ( inc == 0 )
						m_WorkStr += _T("%i");
					inc++;
				} else if ( a > 0 && inc ) {
					*pe = TRUE;
					return orig;
				}
				if ( (InfoToCapTab[n].mode & 002) != 0 ) {
					t = tab[pos];
					tab[pos] = tab[pos + 1];
					tab[pos + 1] = t;
				}
				m_WorkStr += InfoToCapTab[n].cap;
				p += tmp.GetLength();
				if ( (pos += a) > 10 ) {
					*pe = TRUE;
					return orig;
				}
			} else {
				*pe = TRUE;
				return orig;
			}
		} else if ( p[0] == _T('$') && p[1] == _T('<') ) {			// skip delay...
			for ( s = p + 2 ; *s != _T('\0') && *s != _T('>') ; )
				s++;
			if ( *s == _T('>') )
				p = s + 1;
		} else if ( *p == _T(':') ) {
			m_WorkStr += _T("\\072");
			p++;
		} else
			m_WorkStr += *(p++);
	}
	return m_WorkStr;
}
int CInfoCapDlg::GetOneChar(LPCTSTR *pp, BOOL bPar)
{
	int n = 0;
	LPCTSTR p = *pp;

	if ( *p == _T('\0') )
		return (-1);

	if ( p[0] == _T('\\') && p[1] >= _T('0') && p[1] <= _T('3') ) {
		p += 1;
		if ( *p >= _T('0') && *p <= _T('7') )
			n = n * 8 + (*(p++) - _T('0'));
		if ( *p >= _T('0') && *p <= _T('7') )
			n = n * 8 + (*(p++) - _T('0'));
		if ( *p >= _T('0') && *p <= _T('7') )
			n = n * 8 + (*(p++) - _T('0'));
	} else if ( *p == _T('\\') ) {
		switch(p[1]) {
		case _T('E'): n = 0x1B; break;
		case _T('n'): n = 0x0A; break;
		case _T('r'): n = 0x0D; break;
		case _T('t'): n = 0x09; break;
		case _T('b'): n = 0x08; break;
		case _T('f'): n = 0x0C; break;
		default:  n = p[1]; break;
		}
		p += 2;
	} else if ( bPar && p[0] == _T('%') && p[1] == _T('%') ) {
		n = _T('%');
		p += 2;
	} else if ( p[0] == _T('^') && p[1] >= _T('@') && p[1] <= _T('_') ) {
		n = p[1] - _T('@');
		p += 2;
	} else
		n = *(p++);

	*pp = p;
	return n;
}
void CInfoCapDlg::SetEscStr(LPCTSTR str, CBuffer &out)
{
	int c;

	out.Clear();
	while ( (c = GetOneChar(&str, FALSE)) != (-1) )
		out.PutWord(c);
}
LPCTSTR CInfoCapDlg::CapToInfo(LPCTSTR p, BOOL *pe)
{
	int n;
	int i = 0;
	int push = 0;
	CString tmp;
	BYTE tab[12];
	LPCTSTR orig = p;
	LPCTSTR param = _T("%%p%c");

	if ( m_pIndex->m_Value == 1 )
		return p;

	for ( n = 0 ; n < 12 ; n++ )
		tab[n] = _T('1') + n;

	m_WorkStr.Empty();
	while ( *p != _T('\0') ) {
		if ( *p == _T('%') ) {
			switch(p[1]) {
			case _T('%'):
				m_WorkStr += *(p++);
				m_WorkStr += *(p++);
				break;
			case _T('d'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					push++;
				}
				m_WorkStr += _T("%d");
				p += 2;
				break;
			case _T('2'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					push++;
				}
				m_WorkStr += _T("%2d");
				p += 2;
				break;
			case _T('3'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					push++;
				}
				m_WorkStr += _T("%3d");
				p += 2;
				break;
			case _T('.'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					push++;
				}
				m_WorkStr += _T("%c");
				p += 2;
				break;
			case _T('+'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					push++;
				}
				p += 2;
				if ( (n = GetOneChar(&p)) < 0 ) {
					*pe = TRUE;
					return orig;
				}
				tmp.Format(_T("%%{%d}%%+%%c"), n);
				m_WorkStr += tmp;
				break;
			case _T('i'):
				m_WorkStr += *(p++);
				m_WorkStr += *(p++);
				break;
			case _T('r'):
				n = tab[i];
				tab[i] = tab[i + 1];
				tab[i + 1] = n;
				p += 2;
				break;
			case _T('>'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					m_WorkStr += tmp;
					push++;
				}
				p += 2;
				if ( (n = GetOneChar(&p)) < 0 ) {
					*pe = TRUE;
					return orig;
				}
				tmp.Format(_T("%%?%%{%d}%%>"), n);
				m_WorkStr += tmp;
				if ( (n = GetOneChar(&p)) < 0 ) {
					*pe = TRUE;
					return orig;
				}
				tmp.Format(_T("%%t%%{%d}%%+%%;"), n);
				m_WorkStr += tmp;
				push++;
				break;
			case _T('n'):
				param = (_tcslen(param) > 8 ? _T("%%p%c") : _T("%%p%c%%{96}%%^"));
				p += 2;
				break;
			case _T('B'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					push++;
				}
				m_WorkStr += _T("%{10}%/%{16}%*%{10}%m%+");
				push++;
				p += 2;
				break;
			case _T('D'):
				if ( --push < 0 ) {
					tmp.Format(param, tab[i++]);
					m_WorkStr += tmp;
					m_WorkStr += tmp;
					push++;
				}
				m_WorkStr += _T("%{2}%*%-");
				push++;
				p += 2;
				break;
			default:
				m_WorkStr += *(p++);
				m_WorkStr += *(p++);
				break;
			}
			if ( i > 10 ) {
				*pe = TRUE;
				return orig;
			}
		} else if ( *p == _T(',') ) {
			m_WorkStr += _T("\\054");
			p++;
		} else
			m_WorkStr += *(p++);
	}
	return m_WorkStr;
}
void CInfoCapDlg::SetCapStr(BOOL lf)
{
	int n, i;
	int len = 0;
	int all;
	TCHAR c;
	CStringLoad tmp, err;
	BOOL en;
	BOOL ec = FALSE;
	CDWordArray ptab;

	m_Entry.GetWindowText(m_EntryName);

	m_TermCap = m_EntryName;
	m_TermCap += _T(":");
	all = m_TermCap.GetLength();

	if ( lf )
		m_TermCap += _T("\\\r\n\t:");

	ptab.SetSize(m_List.GetItemCount());
	for ( i = 0 ; i < ptab.GetSize() ; i++ )
		ptab[i] = (-1);

	for ( i = 0 ; i < m_List.GetItemCount() ; i++ ) {
		if ( m_List.GetItemText(i, 4).IsEmpty() || m_List.GetItemText(i, 2).IsEmpty() )
			continue;
		if ( (n = (int)m_List.GetItemData(i)) >= 0 )
			ptab[n] = i;
	}

	for ( i = 0 ; i < ptab.GetSize() ; i++ ) {
		if ( (n = ptab[i]) < 0 )
			continue;
		//if ( m_List.GetItemText(n, 4).IsEmpty() || m_List.GetItemText(n, 2).IsEmpty() )
		//	continue;
		c = m_List.GetItemText(n, 3)[0];
		tmp = m_List.GetItemText(n, 2);
		if ( c != _T(' ') ) {
			en = FALSE;
			tmp += c;
			tmp += InfoToCap(m_List.GetItemText(n, 4), &en);
			if ( en ) {
				ec = TRUE;
				if ( !err.IsEmpty() )
					err += _T("\n");
				if ( tmp.GetLength() > 40 ) {
					err += tmp.Mid(0, 40);
					err += _T("...");
				} else
					err += tmp;
			}
		}
		tmp += _T(":");
		all += tmp.GetLength();
		if ( lf && (len += tmp.GetLength()) > 60 ) {
			m_TermCap += _T("\\\r\n\t:");
			len = tmp.GetLength();
		}
		m_TermCap += tmp;
	}

	if ( lf )
		m_TermCap += _T("\r\n");

	if ( ec ) {
		tmp.Empty();
		if ( all >= 1024 )
			tmp.LoadString(IDE_INFOCAPOVER1024);
		tmp += CStringLoad(IDE_INFOCAPINFOCONVERROR);
		tmp += err;
		::AfxMessageBox(tmp, MB_ICONHAND);
	} else if ( all >= 1024 )
		::AfxMessageBox(CStringLoad(IDE_INFOCAPOVER1024), MB_ICONINFORMATION);
}
void CInfoCapDlg::SetInfoStr(BOOL lf)
{
	int n, i;
	int len = 0;
	TCHAR c;
	CStringLoad tmp, err;
	BOOL en;
	BOOL ec = FALSE;

	m_Entry.GetWindowText(m_EntryName);

	m_TermInfo = m_EntryName;
	m_TermInfo += _T(", ");

	if ( lf )
		m_TermInfo += _T("\r\n\t");

	for ( i = 0 ; i < m_List.GetItemCount() ; i++ ) {
		if ( (n = m_List.GetParamItem(i)) < 0 )
			continue;
		if ( m_List.GetItemText(n, 4).IsEmpty() || m_List.GetItemText(n, 1).IsEmpty() )
			continue;
		c = m_List.GetItemText(n, 3)[0];
		tmp = m_List.GetItemText(n, 1);
		if ( c != _T(' ') ) {
			en = FALSE;
			tmp += c;
			tmp += CapToInfo(m_List.GetItemText(n, 4), &en);
			if ( en ) {
				ec = TRUE;
				if ( !err.IsEmpty() )
					err += _T("\n");
				if ( tmp.GetLength() > 40 ) {
					err += tmp.Mid(0, 40);
					err += _T("...");
				} else
					err += tmp;
			}
		}
		tmp += _T(", ");
		if ( lf && (len += tmp.GetLength()) > 60 ) {
			m_TermInfo += _T("\r\n\t");
			len = tmp.GetLength();
		}
		m_TermInfo += tmp;
	}

	if ( lf )
		m_TermInfo += _T("\r\n");

	if ( ec ) {
		tmp.LoadString(IDE_INFOCAPCONVERROR);
		tmp += err;
		::AfxMessageBox(tmp, MB_ICONHAND);
	}
}
void CInfoCapDlg::InfoNameToCapName(CStringBinary &tab)
{
	int n;

	for ( n = 0 ; InfoCap[n].type != (-1) ; n++ )
		tab[InfoCap[n].info] = InfoCap[n].cap;
}

// CInfoCapDlg メッセージ ハンドラ

static const INITDLGTAB ItemTab[] = {
	{ IDC_ENTRY,		ITM_RIGHT_RIGHT },
	{ IDC_LOADCAP,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_LOADINFO,		ITM_LEFT_RIGHT | ITM_RIGHT_RIGHT },
	{ IDC_INFOLIST,		ITM_RIGHT_RIGHT | ITM_BTM_BTM  },

	{ IDOK,				ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM  },
	{ IDCANCEL,			ITM_LEFT_MID | ITM_RIGHT_MID | ITM_TOP_BTM | ITM_BTM_BTM  },

	{ IDC_TITLE1,		ITM_RIGHT_RIGHT },

	{ 0,	0 },
};

BOOL CInfoCapDlg::OnInitDialog()
{
	CDialogExt::OnInitDialog();

	InitItemOffset(ItemTab);

	int n;
	CStringIndex cap;

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT); // | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn(_T("InfoCapDlg"), InitListTab, 6);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 3);
	m_List.m_EditSubItem = (1 << 4);

	m_List.DeleteAllItems();
	for ( n = 0 ; InfoCap[n].type != (-1) ; n++ ) {
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, InfoCap[n].name, 0, 0, 0, n);
		m_List.SetItemText(n, 1, InfoCap[n].info);
		m_List.SetItemText(n, 2, InfoCap[n].cap);
		m_List.SetItemText(n, 3, TypeStr[InfoCap[n].type]);
		m_List.SetItemText(n, 5, InfoCap[n].memo);
		m_List.SetItemData(n, n);
		m_InfoIndex[InfoCap[n].info] = n;
		m_CapIndex[InfoCap[n].cap]   = n;
	}

	if ( m_TermCap.IsEmpty() )
		m_TermCap = TermCap;

	SetEntry(cap, m_TermCap);
	SetList(cap, m_CapIndex);

	SetSaveProfile(_T("InfoCapDlg"));

	return TRUE;
}

void CInfoCapDlg::OnOK()
{
	SetCapStr(FALSE);
	CDialogExt::OnOK();
}

void CInfoCapDlg::OnBnClickedLoadcap()
{
	int n;
	CString tmp;
	CFileDialog dlg(TRUE, _T(""), _T("termcap"), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_Entry.GetWindowText(tmp);

	for ( n = m_Entry.GetCount() - 1 ; n >= 0 ; n-- )
		m_Entry.DeleteString(n);

	LoadCapFile(dlg.GetPathName());

	m_Entry.SetWindowText(tmp);
}

void CInfoCapDlg::OnBnClickedLoadinfo()
{
	int n;
	CString tmp;
	CFileDialog dlg(TRUE, _T(""), _T("terminfo"), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	m_Entry.GetWindowText(tmp);

	for ( n = m_Entry.GetCount() - 1 ; n >= 0 ; n-- )
		m_Entry.DeleteString(n);

	LoadInfoFile(dlg.GetPathName());

	m_Entry.SetWindowText(tmp);
}

void CInfoCapDlg::OnCbnSelchangeEntry()
{
	int n;
	CString tmp;
	CStringIndex cap;

	if ( (n = m_Entry.GetCurSel()) < 0 )
		return;
	m_Entry.GetLBText(n, tmp);

	SetCapName(cap, tmp);
	SetList(cap, *m_pIndex);

#ifdef	USE_DEBUGCAP
	m_SaveEntry.Add(tmp);
	m_SaveCapInfo.SetNoCase(FALSE);
	for ( n = 0 ; n < cap.GetSize() ; n++ ) {
		while ( m_SaveCapInfo[cap[n].m_nIndex].GetSize() < m_SaveEntry.GetSize() )
			m_SaveCapInfo[cap[n].m_nIndex].Add(_T(""));
		m_SaveCapInfo[cap[n].m_nIndex].Add(cap[n].m_String);
	}
	for ( n = 0 ; n < m_SaveCapInfo.GetSize() ; n++ ) {
		if ( cap.Find(m_SaveCapInfo[n].m_nIndex) < 0 )
			m_SaveCapInfo[n].Add(_T(""));
	}
#endif
}

void CInfoCapDlg::OnCapInport()
{
	int n;
	CStringBinary *bp;
	CStringIndex cap;
	CFileDialog dlg(TRUE, _T(""), _T("termcap.txt"), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	for ( n = m_Entry.GetCount() - 1 ; n >= 0 ; n-- )
		m_Entry.DeleteString(n);

	LoadCapFile(dlg.GetPathName());

	if ( (bp = m_CapName.FindValue(0)) == NULL ) {
		m_Entry.SetWindowText(_T(""));
		return;
	}

	m_Entry.SetWindowText(bp->m_Index);
	SetCapName(cap, bp->m_Index);
	SetList(cap, *m_pIndex);
}

void CInfoCapDlg::OnCapExport()
{
	FILE *fp;
	CFileDialog dlg(FALSE, _T(""), _T("termcap.txt"), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	SetCapStr(TRUE);

	if ( (fp = _tfopen(dlg.GetPathName(), _T("wb"))) == NULL )
		return;

	fputs(TstrToMbs(m_TermCap), fp);

	fclose(fp);
}

void CInfoCapDlg::OnCapClipbord()
{
#ifndef USE_DEBUGCAP
	SetCapStr(TRUE);

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(m_TermCap);
#else
	int n, i;
	m_TermCap.Empty();
	for ( n = 0 ; n < m_SaveCapInfo.GetSize() ; n++ ) {
		m_TermCap += m_SaveCapInfo[n].m_nIndex;
		m_TermCap += _T('\t');
		for ( i = 0 ; i < m_SaveCapInfo[n].GetSize() ; i++ ) {
			m_TermCap += _T("\"");
			if ( m_SaveCapInfo[n][i].m_String[0] == _T(' ') )
				m_TermCap += _T('*');
			else if ( m_SaveCapInfo[n][i].m_String[0] != _T('@') )
				m_TermCap += m_SaveCapInfo[n][i].m_String;
			m_TermCap += _T("\"\t");
		}
		m_TermCap += _T("\r\n");
	}
	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(m_TermCap);
#endif
}

void CInfoCapDlg::OnInfoInport()
{
	int n;
	CStringBinary *bp;
	CStringIndex cap;
	CFileDialog dlg(TRUE, _T(""), _T("terminfo.txt"), OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	for ( n = m_Entry.GetCount() - 1 ; n >= 0 ; n-- )
		m_Entry.DeleteString(n);

	LoadInfoFile(dlg.GetPathName());

	if ( (bp = m_CapName.FindValue(0)) == NULL ) {
		m_Entry.SetWindowText(_T(""));
		return;
	}

	m_Entry.SetWindowText(bp->m_Index);
	SetCapName(cap, bp->m_Index);
	SetList(cap, *m_pIndex);
}

void CInfoCapDlg::OnInfoExport()
{
	FILE *fp;
	CFileDialog dlg(FALSE, _T(""), _T("terminfo.txt"), OFN_OVERWRITEPROMPT, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( DpiAwareDoModal(dlg) != IDOK )
		return;

	SetInfoStr(TRUE);

	if ( (fp = _tfopen(dlg.GetPathName(), _T("wb"))) == NULL )
		return;

	fputs(TstrToMbs(m_TermInfo), fp);

	fclose(fp);
}

void CInfoCapDlg::OnInfoClipbord()
{
	SetInfoStr(TRUE);

	((CMainFrame *)::AfxGetMainWnd())->SetClipboardText(m_TermInfo);
}
