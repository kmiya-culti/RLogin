
#define	BD_SIZE			1
#define	BD_HALFSIZE		18

#define	BD_NONE			0x00
#define	BD_LINE1		0x01	// │
#define	BD_LINE2		0x02	// ┃
#define	BD_LINE3		0x03	// ║
#define	BD_LINE4		0x04	// ╭
#define	BD_LINE5		0x05	// ╳
#define	BD_DOT2			0x10	// ╎
#define	BD_DOT3			0x20	// ┆
#define	BD_DOT4			0x30	// ┊

static const BYTE BorderTab[][4] = {
	//	LEFT				RIGHT				UP					DOWN
	{	BD_LINE1,			BD_LINE1,			BD_NONE,			BD_NONE				},	// ─ (U+2500)
	{	BD_LINE2,			BD_LINE2,			BD_NONE,			BD_NONE				},	// ━
	{	BD_NONE,			BD_NONE,			BD_LINE1,			BD_LINE1			},	// │
	{	BD_NONE,			BD_NONE,			BD_LINE2,			BD_LINE2			},	// ┃
	{	BD_LINE1 | BD_DOT3,	BD_LINE1 | BD_DOT3,	BD_NONE,			BD_NONE				},	// ┄
	{	BD_LINE2 | BD_DOT3,	BD_LINE2 | BD_DOT3,	BD_NONE,			BD_NONE				},	// ┅
	{	BD_NONE,			BD_NONE,			BD_LINE1 | BD_DOT3,	BD_LINE1 | BD_DOT3	},	// ┆
	{	BD_NONE,			BD_NONE,			BD_LINE2 | BD_DOT3,	BD_LINE2 | BD_DOT3	},	// ┇
	{	BD_LINE1 | BD_DOT4,	BD_LINE1 | BD_DOT4,	BD_NONE,			BD_NONE				},	// ┈
	{	BD_LINE2 | BD_DOT4,	BD_LINE2 | BD_DOT4,	BD_NONE,			BD_NONE				},	// ┉
	{	BD_NONE,			BD_NONE,			BD_LINE1 | BD_DOT4,	BD_LINE1 | BD_DOT4	},	// ┊
	{	BD_NONE,			BD_NONE,			BD_LINE2 | BD_DOT4,	BD_LINE2 | BD_DOT4	},	// ┋
	{	BD_NONE,			BD_LINE1,			BD_NONE,			BD_LINE1			},	// ┌
	{	BD_NONE,			BD_LINE2,			BD_NONE,			BD_LINE1			},	// ┍
	{	BD_NONE,			BD_LINE1,			BD_NONE,			BD_LINE2			},	// ┎
	{	BD_NONE,			BD_LINE2,			BD_NONE,			BD_LINE2			},	// ┏
	{	BD_LINE1,			BD_NONE,			BD_NONE,			BD_LINE1			},	// ┐ (U+2510)
	{	BD_LINE2,			BD_NONE,			BD_NONE,			BD_LINE1			},	// ┑
	{	BD_LINE1,			BD_NONE,			BD_NONE,			BD_LINE2			},	// ┒
	{	BD_LINE2,			BD_NONE,			BD_NONE,			BD_LINE2			},	// ┓
	{	BD_NONE,			BD_LINE1,			BD_LINE1,			BD_NONE				},	// └
	{	BD_NONE,			BD_LINE2,			BD_LINE1,			BD_NONE				},	// ┕
	{	BD_NONE,			BD_LINE1,			BD_LINE2,			BD_NONE				},	// ┖
	{	BD_NONE,			BD_LINE2,			BD_LINE2,			BD_NONE				},	// ┗
	{	BD_LINE1,			BD_NONE,			BD_LINE1,			BD_NONE				},	// ┘
	{	BD_LINE2,			BD_NONE,			BD_LINE1,			BD_NONE				},	// ┙
	{	BD_LINE1,			BD_NONE,			BD_LINE2,			BD_NONE				},	// ┚
	{	BD_LINE2,			BD_NONE,			BD_LINE2,			BD_NONE				},	// ┛
	{	BD_NONE,			BD_LINE1,			BD_LINE1,			BD_LINE1			},	// ├
	{	BD_NONE,			BD_LINE2,			BD_LINE1,			BD_LINE1			},	// ┝
	{	BD_NONE,			BD_LINE1,			BD_LINE2,			BD_LINE1			},	// ┞
	{	BD_NONE,			BD_LINE1,			BD_LINE1,			BD_LINE2			},	// ┟
	{	BD_NONE,			BD_LINE1,			BD_LINE2,			BD_LINE2			},	// ┠ (U+2520)
	{	BD_NONE,			BD_LINE2,			BD_LINE2,			BD_LINE1			},	// ┡
	{	BD_NONE,			BD_LINE2,			BD_LINE1,			BD_LINE2			},	// ┢
	{	BD_NONE,			BD_LINE2,			BD_LINE2,			BD_LINE2			},	// ┣
	{	BD_LINE1,			BD_NONE,			BD_LINE1,			BD_LINE1			},	// ┤
	{	BD_LINE2,			BD_NONE,			BD_LINE1,			BD_LINE1			},	// ┥
	{	BD_LINE1,			BD_NONE,			BD_LINE2,			BD_LINE1			},	// ┦
	{	BD_LINE1,			BD_NONE,			BD_LINE1,			BD_LINE2			},	// ┧
	{	BD_LINE1,			BD_NONE,			BD_LINE2,			BD_LINE2			},	// ┨
	{	BD_LINE2,			BD_NONE,			BD_LINE2,			BD_LINE1			},	// ┩
	{	BD_LINE2,			BD_NONE,			BD_LINE1,			BD_LINE2			},	// ┪
	{	BD_LINE2,			BD_NONE,			BD_LINE2,			BD_LINE2			},	// ┫
	{	BD_LINE1,			BD_LINE1,			BD_NONE,			BD_LINE1			},	// ┬
	{	BD_LINE2,			BD_LINE1,			BD_NONE,			BD_LINE1			},	// ┭
	{	BD_LINE1,			BD_LINE2,			BD_NONE,			BD_LINE1			},	// ┮
	{	BD_LINE2,			BD_LINE2,			BD_NONE,			BD_LINE1			},	// ┯
	{	BD_LINE1,			BD_LINE1,			BD_NONE,			BD_LINE2			},	// ┰ (U+2530)
	{	BD_LINE2,			BD_LINE1,			BD_NONE,			BD_LINE2			},	// ┱
	{	BD_LINE1,			BD_LINE2,			BD_NONE,			BD_LINE2			},	// ┲
	{	BD_LINE2,			BD_LINE2,			BD_NONE,			BD_LINE2			},	// ┳
	{	BD_LINE1,			BD_LINE1,			BD_LINE1,			BD_NONE				},	// ┴
	{	BD_LINE2,			BD_LINE1,			BD_LINE1,			BD_NONE				},	// ┵
	{	BD_LINE1,			BD_LINE2,			BD_LINE1,			BD_NONE				},	// ┶
	{	BD_LINE2,			BD_LINE2,			BD_LINE1,			BD_NONE				},	// ┷
	{	BD_LINE1,			BD_LINE1,			BD_LINE2,			BD_NONE				},	// ┸
	{	BD_LINE2,			BD_LINE1,			BD_LINE2,			BD_NONE				},	// ┹
	{	BD_LINE1,			BD_LINE2,			BD_LINE2,			BD_NONE				},	// ┺
	{	BD_LINE2,			BD_LINE2,			BD_LINE2,			BD_NONE				},	// ┻
	{	BD_LINE1,			BD_LINE1,			BD_LINE1,			BD_LINE1			},	// ┼
	{	BD_LINE2,			BD_LINE1,			BD_LINE1,			BD_LINE1			},	// ┽
	{	BD_LINE1,			BD_LINE2,			BD_LINE1,			BD_LINE1			},	// ┾
	{	BD_LINE2,			BD_LINE2,			BD_LINE1,			BD_LINE1			},	// ┿
	{	BD_LINE1,			BD_LINE1,			BD_LINE2,			BD_LINE1			},	// ╀ (U+2540)
	{	BD_LINE1,			BD_LINE1,			BD_LINE1,			BD_LINE2			},	// ╁
	{	BD_LINE1,			BD_LINE1,			BD_LINE2,			BD_LINE2			},	// ╂
	{	BD_LINE2,			BD_LINE1,			BD_LINE2,			BD_LINE1			},	// ╃
	{	BD_LINE1,			BD_LINE2,			BD_LINE2,			BD_LINE1			},	// ╄
	{	BD_LINE2,			BD_LINE1,			BD_LINE1,			BD_LINE2			},	// ╅
	{	BD_LINE1,			BD_LINE2,			BD_LINE1,			BD_LINE2			},	// ╆
	{	BD_LINE2,			BD_LINE2,			BD_LINE2,			BD_LINE1			},	// ╇
	{	BD_LINE2,			BD_LINE2,			BD_LINE1,			BD_LINE2			},	// ╈
	{	BD_LINE2,			BD_LINE1,			BD_LINE2,			BD_LINE2			},	// ╉
	{	BD_LINE1,			BD_LINE2,			BD_LINE2,			BD_LINE2			},	// ╊
	{	BD_LINE2,			BD_LINE2,			BD_LINE2,			BD_LINE2			},	// ╋
	{	BD_LINE1 | BD_DOT2,	BD_LINE1 | BD_DOT2,	BD_NONE,			BD_NONE				},	// ╌
	{	BD_LINE2 | BD_DOT2,	BD_LINE2 | BD_DOT2,	BD_NONE,			BD_NONE				},	// ╍
	{	BD_NONE,			BD_NONE,			BD_LINE1 | BD_DOT2,	BD_LINE1 | BD_DOT2	},	// ╎
	{	BD_NONE,			BD_NONE,			BD_LINE2 | BD_DOT2,	BD_LINE2 | BD_DOT2	},	// ╏
	{	BD_LINE3,			BD_LINE3,			BD_NONE,			BD_NONE				},	// ═ (U+2550)
	{	BD_NONE,			BD_NONE,			BD_LINE3,			BD_LINE3			},	// ║
	{	BD_NONE,			BD_LINE3,			BD_NONE,			BD_LINE1			},	// ╒
	{	BD_NONE,			BD_LINE1,			BD_NONE,			BD_LINE3			},	// ╓
	{	BD_NONE,			BD_LINE3,			BD_NONE,			BD_LINE3			},	// ╔
	{	BD_LINE3,			BD_NONE,			BD_NONE,			BD_LINE1			},	// ╕
	{	BD_LINE1,			BD_NONE,			BD_NONE,			BD_LINE3			},	// ╖
	{	BD_LINE3,			BD_NONE,			BD_NONE,			BD_LINE3			},	// ╗
	{	BD_NONE,			BD_LINE3,			BD_LINE1,			BD_NONE				},	// ╘
	{	BD_NONE,			BD_LINE1,			BD_LINE3,			BD_NONE				},	// ╙
	{	BD_NONE,			BD_LINE3,			BD_LINE3,			BD_NONE				},	// ╚
	{	BD_LINE3,			BD_NONE,			BD_LINE1,			BD_NONE				},	// ╛
	{	BD_LINE1,			BD_NONE,			BD_LINE3,			BD_NONE				},	// ╜
	{	BD_LINE3,			BD_NONE,			BD_LINE3,			BD_NONE				},	// ╝
	{	BD_NONE,			BD_LINE3,			BD_LINE1,			BD_LINE1			},	// ╞
	{	BD_NONE,			BD_LINE1,			BD_LINE3,			BD_LINE3			},	// ╟
	{	BD_NONE,			BD_LINE3,			BD_LINE3,			BD_LINE3			},	// ╠ (U+2560)
	{	BD_LINE3,			BD_NONE,			BD_LINE1,			BD_LINE1			},	// ╡
	{	BD_LINE1,			BD_NONE,			BD_LINE3,			BD_LINE3			},	// ╢
	{	BD_LINE3,			BD_NONE,			BD_LINE3,			BD_LINE3			},	// ╣
	{	BD_LINE3,			BD_LINE3,			BD_NONE,			BD_LINE1			},	// ╤
	{	BD_LINE1,			BD_LINE1,			BD_NONE,			BD_LINE3			},	// ╥
	{	BD_LINE3,			BD_LINE3,			BD_NONE,			BD_LINE3			},	// ╦
	{	BD_LINE3,			BD_LINE3,			BD_LINE1,			BD_NONE				},	// ╧
	{	BD_LINE1,			BD_LINE1,			BD_LINE3,			BD_NONE				},	// ╨
	{	BD_LINE3,			BD_LINE3,			BD_LINE3,			BD_NONE				},	// ╩
	{	BD_LINE3,			BD_LINE3,			BD_LINE1,			BD_LINE1			},	// ╪
	{	BD_LINE1,			BD_LINE1,			BD_LINE3,			BD_LINE3			},	// ╫
	{	BD_LINE3,			BD_LINE3,			BD_LINE3,			BD_LINE3			},	// ╬
	{	BD_NONE,			BD_NONE,			BD_NONE,			BD_LINE4			},	// ╭
	{	BD_LINE4,			BD_NONE,			BD_NONE,			BD_NONE				},	// ╮
	{	BD_NONE,			BD_NONE,			BD_LINE4,			BD_NONE				},	// ╯
	{	BD_NONE,			BD_LINE4,			BD_NONE,			BD_NONE				},	// ╰ (U+2570)
	{	BD_LINE5,			BD_NONE,			BD_NONE,			BD_NONE				},	// ╱
	{	BD_NONE,			BD_NONE,			BD_LINE5,			BD_NONE				},	// ╲
	{	BD_LINE5,			BD_NONE,			BD_LINE5,			BD_NONE				},	// ╳
	{	BD_LINE1,			BD_NONE,			BD_NONE,			BD_NONE				},	// ╴
	{	BD_NONE,			BD_NONE,			BD_LINE1,			BD_NONE				},	// ╵
	{	BD_NONE,			BD_LINE1,			BD_NONE,			BD_NONE				},	// ╶
	{	BD_NONE,			BD_NONE,			BD_NONE,			BD_LINE1			},	// ╷
	{	BD_LINE2,			BD_NONE,			BD_NONE,			BD_NONE				},	// ╸
	{	BD_NONE,			BD_NONE,			BD_LINE2,			BD_NONE				},	// ╹
	{	BD_NONE,			BD_LINE2,			BD_NONE,			BD_NONE				},	// ╺
	{	BD_NONE,			BD_NONE,			BD_NONE,			BD_LINE2			},	// ╻
	{	BD_LINE1,			BD_LINE2,			BD_NONE,			BD_NONE				},	// ╼
	{	BD_NONE,			BD_NONE,			BD_LINE1,			BD_LINE2			},	// ╽
	{	BD_LINE2,			BD_LINE1,			BD_NONE,			BD_NONE				},	// ╾
	{	BD_NONE,			BD_NONE,			BD_LINE2,			BD_LINE1			},	// ╿ (U+257F)
};

#define	FI_NONE				0

#define	FI_L_LEFT			01000
#define	FI_L_CENTER			02000
#define	FI_L_RIGHT			03000

#define	FI_T_TOP			00010
#define	FI_T_CENTER			00020
#define	FI_T_BOTTOM			00030
#define	FI_T_CALC			00040

#define	FI_R_LEFT			00100
#define	FI_R_CENTER			00200
#define	FI_R_RIGHT			00300
#define	FI_R_CALC			00400

#define	FI_B_TOP			00001
#define	FI_B_CENTER			00002
#define	FI_B_BOTTOM			00003
	
static const DWORD FillboxTab[][2] = {
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_CENTER,			FI_NONE				},	// ▀ (U+2580)
	{	FI_L_LEFT | FI_T_CALC | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▁ (U+2581)
	{	FI_L_LEFT | FI_T_CALC | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▂ (U+2582)
	{	FI_L_LEFT | FI_T_CALC | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▃ (U+2583)
	{	FI_L_LEFT | FI_T_CALC | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▄ (U+2584)
	{	FI_L_LEFT | FI_T_CALC | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▅ (U+2585)
	{	FI_L_LEFT | FI_T_CALC | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▆ (U+2586)
	{	FI_L_LEFT | FI_T_CALC | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▇ (U+2587)
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// █ (U+2588)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CALC | FI_B_BOTTOM,				FI_NONE				},	// ▉ (U+2589)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CALC | FI_B_BOTTOM,				FI_NONE				},	// ▊ (U+258A)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CALC | FI_B_BOTTOM,				FI_NONE				},	// ▋ (U+258B)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CALC | FI_B_BOTTOM,				FI_NONE				},	// ▌ (U+258C)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CALC | FI_B_BOTTOM,				FI_NONE				},	// ▍ (U+258D)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CALC | FI_B_BOTTOM,				FI_NONE				},	// ▎ (U+258E)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CALC | FI_B_BOTTOM,				FI_NONE				},	// ▏ (U+258F)
	{	FI_L_CENTER | FI_T_TOP | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▐ (U+2590)
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ░ (U+2591)
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▒ (U+2592)
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▓ (U+2593)
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_TOP,				FI_NONE				},	// ▔ (U+2594)
	{	FI_L_RIGHT | FI_T_TOP | FI_R_RIGHT | FI_B_BOTTOM,			FI_NONE				},	// ▕ (U+2595)
	{	FI_L_LEFT | FI_T_CENTER | FI_R_CENTER | FI_B_BOTTOM,		FI_NONE				},	// ▖ (U+2596)
	{	FI_L_CENTER | FI_T_CENTER | FI_R_RIGHT | FI_B_BOTTOM,		FI_NONE				},	// ▗ (U+2597)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CENTER | FI_B_CENTER,			FI_NONE				},	// ▘ (U+2598)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CENTER | FI_B_CENTER,			FI_L_LEFT | FI_T_CENTER | FI_R_RIGHT | FI_B_BOTTOM		},	// ▙ (U+2599)
	{	FI_L_LEFT | FI_T_TOP | FI_R_CENTER | FI_B_CENTER,			FI_L_CENTER | FI_T_CENTER | FI_R_RIGHT | FI_B_BOTTOM	},	// ▚ (U+259A)
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_CENTER,			FI_L_LEFT | FI_T_CENTER | FI_R_CENTER | FI_B_BOTTOM		},	// ▛ (U+259B)
	{	FI_L_LEFT | FI_T_TOP | FI_R_RIGHT | FI_B_CENTER,			FI_L_CENTER | FI_T_CENTER | FI_R_RIGHT | FI_B_BOTTOM	},	// ▜ (U+259C)
	{	FI_L_CENTER | FI_T_TOP | FI_R_RIGHT | FI_B_CENTER,			FI_NONE													},	// ▝ (U+259D)
	{	FI_L_CENTER | FI_T_TOP | FI_R_RIGHT | FI_B_CENTER,			FI_L_LEFT | FI_T_CENTER | FI_R_CENTER | FI_B_BOTTOM		},	// ▞ (U+259E)
	{	FI_L_CENTER | FI_T_TOP | FI_R_RIGHT | FI_B_CENTER,			FI_L_LEFT | FI_T_CENTER| FI_R_RIGHT | FI_B_BOTTOM		},	// ▟ (U+259F)
};

#ifdef	USE_PARENTHESIS

#define	PARENTHESIS_START	0x239B
#define	PARENTHESIS_END		0x23BD

#define	PTS_SEP				254
#define	PTS_END				255

#define	PTS_SEP_MARK		{ PTS_SEP, 0 }
#define	PTS_END_MARK		{ PTS_END, 0 }

static const BYTE KakoTab[35][16][2] = {
	{ { 80, 15 }, { 68, 21 }, { 56, 34 }, { 50, 46 }, { 50, 100 }, PTS_END_MARK },							// ⎛	U+239B
	{ { 50, 0 }, { 50, 100 }, PTS_END_MARK },																// ⎜	U+239C
	{ { 50, 0 }, { 50, 54 }, { 56, 66 }, { 68, 79 }, { 80, 85 }, { 68, 79 }, PTS_END_MARK },				// ⎝	U+239D
	{ { 20, 15 }, { 32, 21 }, { 44, 34 }, { 50, 46 }, { 50, 100 }, PTS_END_MARK },							// ⎞	U+239E
	{ { 50, 0 }, { 50, 100 }, PTS_END_MARK },																// ⎟	U+239F
	{ { 50, 0 }, { 50, 54 }, { 44, 66 }, { 32, 79 }, { 20, 85 }, { 32, 79 }, PTS_END_MARK },				// ⎠	U+23A0

	{ { 75, 15 }, { 50, 15 }, { 50, 100 }, PTS_END_MARK },													// ⎡	U+23A1
	{ { 50, 0 }, { 50, 100 }, PTS_END_MARK },																// ⎢	U+23A2
	{ { 50, 0 }, { 50, 85 }, { 75, 85 }, { 50, 85 }, PTS_END_MARK },										// ⎣	U+23A3
	{ { 25, 15 }, { 50, 15 }, { 50, 100 }, PTS_END_MARK },													// ⎤	U+23A4
	{ { 50, 0 }, { 50, 100 }, PTS_END_MARK },																// ⎥	U+23A5
	{ { 50, 0 }, { 50, 85 }, { 25, 85 }, { 50, 85 }, PTS_END_MARK },										// ⎦	U+23A6

	{ { 72, 15 }, { 61, 17 }, { 54, 21 }, { 50, 28 }, { 50, 100 }, PTS_END_MARK },							// ⎧	U+23A7
	{ { 50, 0 }, { 50, 36 }, { 45, 44 }, { 30, 50 }, { 45, 56 }, { 50, 64 }, { 50, 100 }, PTS_END_MARK },	// ⎨	U+23A8
	{ { 50, 0 }, { 50, 72 }, { 54, 79 }, { 61, 83 }, { 72, 85 }, { 61, 83 }, PTS_END_MARK },				// ⎩	U+23A9
	{ { 50, 0 }, { 50, 100 }, PTS_END_MARK },																// ⎪	U+23AA
	{ { 28, 15 }, { 39, 17 }, { 46, 21 }, { 50, 28 }, { 50, 100 }, PTS_END_MARK },							// ⎫	U+23AB
	{ { 50, 0 }, { 50, 36 }, { 55, 44 }, { 69, 50 }, { 55, 56 }, { 50, 64 }, { 50, 100 }, PTS_END_MARK },	// ⎬	U+23AC
	{ { 50, 0 }, { 50, 72 }, { 46, 79 }, { 39, 83 }, { 28, 85 }, { 39, 83 }, PTS_END_MARK },				// ⎭	U+23AD

	{ { 50, 0 }, { 50, 100 }, PTS_END_MARK },																// ⎮	U+23AE
	{ { 0, 50 }, { 100, 50 }, PTS_END_MARK },																// ⎯	U+23AF

	{ { 32, 92 }, { 42, 88 }, { 50, 78 }, { 50, 29 }, { 59, 18 }, { 69, 15 }, { 59, 18 }, PTS_END_MARK },	// ⎰	U+23B0
	{ { 68, 92 }, { 58, 88 }, { 50, 78 }, { 50, 29 }, { 41, 18 }, { 31, 15 }, { 41, 18 }, PTS_END_MARK },	// ⎱	U+23B1

	{ { 75, 14 }, { 28, 14 }, { 61, 100 }, PTS_END_MARK },													// ⎲	U+23B2
	{ { 61, 0 }, { 28, 86 }, { 75, 86 }, { 28, 86 }, PTS_END_MARK },										// ⎳	U+23B3

	{ { 20, 22 }, { 20, 10 }, { 80, 10 }, { 80, 22 }, { 80, 10 }, PTS_END_MARK },							// ⎴	U+23B4
	{ { 20, 77 }, { 20, 90 }, { 80, 90 }, { 80, 77 }, { 80, 90 }, { 80, 77 }, PTS_END_MARK },				// ⎵	U+23B5
	{ { 20, 27 }, { 20, 40 }, { 80, 40 }, { 80, 27 }, { 80, 40 }, { 80, 27 }, PTS_SEP_MARK, 
	  { 20, 73 }, { 20, 60 }, { 80, 60 }, { 80, 73 }, { 80, 60 }, { 80, 73 }, PTS_END_MARK },				// ⎶	U+23B6

	{ { 50, 0 }, { 50, 82 }, { 21, 45 }, { 10, 45 }, { 21, 45 }, PTS_END_MARK },							// ⎷	U+23B7
	{ { 20, 0 }, { 20, 100 }, PTS_END_MARK },																// ⎸	U+23B8
	{ { 80, 0 }, { 80, 100 }, PTS_END_MARK },																// ⎹	U+23B9

	{ { 0, 20 }, { 100, 20 }, PTS_END_MARK },																// ⎺		U+23BA
	{ { 0, 38 }, { 100, 38 }, PTS_END_MARK },																// ⎻		U+23BB
	{ { 0, 62 }, { 100, 62 }, PTS_END_MARK },																// ⎼		U+23BC
	{ { 0, 80 }, { 100, 80 }, PTS_END_MARK },																// ⎽		U+23BD
};
#endif
