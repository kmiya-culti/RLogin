#! /bin/sh

echo -e '\eP#m xs @81,8 l8 q80 ';
echo $* | sed -e 'y/1234567890/ABCDEFGHIJ/;s/-//g;
s/A/[n77n87]/g;s/B/[n77n89]/g;s/C/[n77n90]/g;s/D/[n79n87]/g;s/E/[n79n89]/g;
s/F/[n79n90]/g;s/G/[n81n87]/g;s/H/[n81n89]/g;s/I/[n81n90]/g;s/J/[n82n89]/g'
echo -e '\e\\'

