/^[^{]*\/\*\*\/$/{
s/^\(.*\)(.*$/\1 DCLPROTO((/
h
:loop
n
s/;/,/
H
tloop
g
s/,\n{/));/
s/\n//g
s/{.*$/void));/
p
}

/^{.*\/\*\*\/$/{
g
s/^\([^(]*\)(.*$/\1 DCLPROTO((void));/
p
d
}

h
