#include <stdio.h>
#include <locale.h>
#include <wchar.h>

void PrintCode(wchar_t dc, wchar_t dx, int dn)
{
    printf("%X", dc);
    if ( (dx - dc) > 1 )
    	printf("+%X", dx - dc - 1);
    printf("=%d,", dn);
}

int main()
{
    int n;
    wchar_t c;
    int dn = 0;
    wchar_t dc = 0;
    wchar_t dx = 0;

    setlocale(LC_CTYPE, "");

    printf("\033P2#u");

    for ( c = 0x20 ; c <= 0x10FFFF ;  c++ ) {
	if ( c >= 0xD800 && c <= 0xDFFF )
	    continue;

	n = wcwidth(c);

	if ( n <= 0 )
	    continue;

	if ( c == dx && n == dn ) {
	    dx = c + 1;
	    continue;
	}

	if ( dc != 0 )
	    PrintCode(dc, dx, dn);

	dc = c;
	dx = c + 1;
	dn = n;
    }

    PrintCode(dc, dx, dn);
    printf("\033\\");

    return 0;
}
