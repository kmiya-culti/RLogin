#! /bin/sh

echo -e '\eP#m z2 @26 l2 ';
echo $* | gsed -r '
s/([A-G])M7/[\L\1s5i4s10i3s15i4s20i1]/g;
s/([A-G])m7/[\L\1s5i3s10i4s15i3s20i2]/g;
s/([A-G])m/[\L\1s5i3s10i4s15i5s20i3s25i4]/g;
s/([A-G])d7/[\L\1s5i3s10i3s15i4s20i2]/g;
s/([A-G])d/[\L\1s5i3s10i3s15i6s20i3s25i3]/g;
s/([A-G])7/[\L\1s5i4s10i3s15i3s20i2]/g;
s/([A-G])9/[\L\1s5i4s10i3s15i3s20i4]/g;
s/([A-G])/[\L\1s5i4s10i3s15i5s20i4s25i3]/g;
'
echo -e '\e\\'

