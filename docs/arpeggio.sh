#! /bin/sh

echo -e '\eP#m @26 l1 < ';
echo $* | gsed -r '
s/([A-G])M7/{\L\1,200i4i3i4i1i-1i-4i-3}/g;
s/([A-G])m7/{\L\1,200i3i4i3i2i-2i-3i-4}/g;
s/([A-G])m/{\L\1,200i3i4i-4i9i-9i4i-4}/g;
s/([A-G])d7/{\L\1,200i3i3i4i2i-2i-4i-3}/g;
s/([A-G])d/{\L\1,200i3i3i-3i9i-9i3i-3}/g;
s/([A-G])7/{\L\1,200i4i3i3i2i-2i-3i-3}/g;
s/([A-G])9/{\L\1,200i4i3i3i4i-4i-3i-3}/g;
s/([A-G])/{\L\1,200i4i3i-3i8i-8i3i-3}/g;
'
echo -e '\e\\'

