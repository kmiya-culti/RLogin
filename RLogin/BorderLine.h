
#define	BD_SIZE			1
#define	BD_HALFSIZE		18

#define	BD_NONE			0x00
#define	BD_LINE1		0x01	// │
#define	BD_LINE2		0x02	// ┃
#define	BD_LINE3		0x03	// ║
#define	BD_LINE4		0x04	// ╭
#define	BD_LINE5		0x05	// ╳
#define	BD_DOT2			0x10
#define	BD_DOT3			0x20
#define	BD_DOT4			0x30

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