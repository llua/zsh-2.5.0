.nr PI 0
.de Ds
.LD
.ft C
..
.de De
.DE
.ft R
..
.de Sh
.SH
\\$1
.XS
\\$1
.XE
..
.TL
An Introduction to the Z Shell
.AU
Paul Falstad
pf@z-code.com
.AU
Bas de Bakker
bas@phys.uva.nl
.PP
.Sh "Introduction"
.PP
\fBzsh\fP is a shell designed for interactive use, although it is also
a powerful scripting language.  Many of the useful features of bash,
ksh, and tcsh were incorporated into \fBzsh\fP; many original features were
added.  This document details some of the unique features of \fBzsh\fP.  It
assumes basic knowledge of the standard UNIX shells; the intent is to
show a reader already familiar with one of the other major shells what
makes \fBzsh\fP more useful or more powerful.  This document is not at all
comprehensive; read the manual entry for a description of the shell
that is complete and concise, although somewhat overwhelming and
devoid of examples.
.PP
The text will frequently mention options that you can set to change
the behaviour of \fBzsh\fP.  You can set these options with the
command
.Ds
% setopt \fIoptionname\fC
.De
and unset them again with
.Ds
% unsetopt \fIoptionname\fC
.De
Case is ignored in option names, as are embedded underscores.
.Sh "Filename Generation"
.PP
Otherwise known as \fIglobbing\fP, filename generation
is quite extensive in \fBzsh\fP.  Of course, it has all the
basics:
.Ds
% ls
Makefile   file.pro   foo.o      main.o     q.c        run234     stuff
bar.o      foo        link       morestuff  run123     run240     sub
file.h     foo.c      main.h     pipe       run2       run303
% ls *.c
foo.c  q.c
% ls *.[co]
bar.o   foo.c   foo.o   main.o  q.c
% ls foo.?
foo.c  foo.o
% ls *.[^c]
bar.o   file.h  foo.o   main.h  main.o
% ls *.[^oh]
foo.c  q.c
.De
Also, if the \fIEXTENDEDGLOB\fP option is set,
some new features are activated.
For example, the \fC^\fP character negates the pattern following it:
.Ds
% setopt extendedglob
% ls -d ^*.c
Makefile   file.pro   link       morestuff  run2       run303
bar.o      foo        main.h     pipe       run234     stuff
file.h     foo.o      main.o     run123     run240     sub
% ls -d ^*.*
Makefile   link       pipe       run2       run240     stuff
foo        morestuff  run123     run234     run303     sub
% ls -d ^Makefile
bar.o      foo        link       morestuff  run123     run240     sub
file.h     foo.c      main.h     pipe       run2       run303
file.pro   foo.o      main.o     q.c        run234     stuff
% ls -d *.^c
\&.rhosts   bar.o     file.h    file.pro  foo.o     main.h    main.o
.De
An expression of the form
\fC<\fIx\fR\-\fIy\fC>\fR
matches a range of integers:
.Ds
% ls run<200-300>
run234  run240
% ls run<300-400>
run303
% ls run<-200>
run123  run2
% ls run<300->
run303
% ls run<>
run123  run2    run234  run240  run303
.De
The \fINUMERICGLOBSORT\fP option will sort files with numbers
according to the number.  This will not work with \fCls\fP as it
resorts its arguments:
.Ds
% setopt numericglobsort
% echo run<>
run2 run123 run234 run240 run303
.De
Grouping is possible:
.Ds
% ls (foo|bar).*
bar.o  foo.c  foo.o
% ls *.(c|o|pro)
bar.o     file.pro  foo.c     foo.o     main.o    q.c
.De
Also, the string \fC**/\fP forces a recursive search of
subdirectories:
.Ds
% ls -R
Makefile   file.pro   foo.o      main.o     q.c        run234     stuff
bar.o      foo        link       morestuff  run123     run240     sub
file.h     foo.c      main.h     pipe       run2       run303

morestuff:

stuff:
file  xxx   yyy

stuff/xxx:
foobar

stuff/yyy:
frobar
% ls **/*bar
stuff/xxx/foobar  stuff/yyy/frobar
% ls **/f*
file.h            foo               foo.o             stuff/xxx/foobar
file.pro          foo.c             stuff/file        stuff/yyy/frobar
% ls *bar*
bar.o
% ls **/*bar*
bar.o             stuff/xxx/foobar  stuff/yyy/frobar
% ls stuff/**/*bar*
stuff/xxx/foobar  stuff/yyy/frobar
.De
.PP
It is possible to exclude certain files from the patterns using
the ~ character.  A pattern of the form \fC*.c~bar.c\fP lists all
files matching \fC*.c\fP, except for the file \fCbar.c\fP.
.Ds
% ls *.c
foo.c    foob.c    bar.c
% ls *.c~bar.c
foo.c    foob.c
% ls *.c~f*
bar.c
.De
.PP
One can add a number of \fIqualifiers\fP to the end of
any of these patterns, to restrict matches to certain
file types.  A qualified pattern is of the form
.DS
\fIpattern\fC(\fR...\fC)\fR
.De
with single-character qualifiers inside the parentheses.
.Ds
% alias l='ls -dF'
% l *
Makefile    foo*        main.h      q.c         run240
bar.o       foo.c       main.o      run123      run303
file.h      foo.o       morestuff/  run2        stuff/
file.pro    link@       pipe        run234      sub
% l *(/)
morestuff/  stuff/
% l *(@)
link@
% l *(*)
foo*        link@       morestuff/  stuff/
% l *(x)
foo*        link@       morestuff/  stuff/
% l *(X)
foo*        link@       morestuff/  stuff/
% l *(R)
bar.o       foo*        link@       morestuff/  run123      run240
file.h      foo.c       main.h      pipe        run2        run303
file.pro    foo.o       main.o      q.c         run234      stuff/
.De
Note that \fC*(x)\fP and \fC*(*)\fP both match executables.
\fC*(X)\fP matches files executable by others, as opposed to
\fC*(x)\fP, which matches files executable by the owner.
\fC*(R)\fP and \fC*(r)\fP match readable files;
\fC*(W)\fP and \fC*(w)\fP, which checks for writable files.
\fC*(W)\fP is especially important, since it checks for world-writable
files:
.Ds
% l *(w)
bar.o       foo*        link@       morestuff/  run123      run240
file.h      foo.c       main.h      pipe        run2        run303
file.pro    foo.o       main.o      q.c         run234      stuff/
% l *(W)
link@   run240
% l -l link run240
lrwxrwxrwx  1 pfalstad       10 May 23 18:12 link -> /usr/bin/
-rw-rw-rw-  1 pfalstad        0 May 23 18:12 run240
.De
If you want to have all the files of a certain type as well as all
symbolic links pointing to files of that type, prefix the qualifier
with a \fC-\fP:
.Ds
% l *(-/)
link@       morestuff/  stuff/
.De
You can filter out the symbolic links with the \fC^\fP character:
.Ds
% l *(W^@)
run240
% l *(x)
foo*        link@       morestuff/  stuff/
% l *(x^@/)
foo*
.De
To find all plain files, you can use \fC.\fP:
.Ds
% l *(.)
Makefile  file.h    foo*      foo.o     main.o    run123    run234    run303
bar.o     file.pro  foo.c     main.h    q.c       run2      run240    sub
% l *(^.)
link@       morestuff/  pipe        stuff/
% l s*(.)
stuff/   sub
% l *(p)
pipe
% l -l *(p)
prw-r--r--  1 pfalstad        0 May 23 18:12 pipe
.De
\fC*(U)\fP matches all files owned by you.
To search for all files not owned by you, use \fC*(^U)\fP:
.Ds
% l -l *(^U)
-rw-------  1 subbarao       29 May 23 18:13 sub
.De
This searches for setuid files:
.Ds
% l -l *(s)
-rwsr-xr-x  1 pfalstad       16 May 23 18:12 foo*
.De
This checks for a certain user's files:
.Ds
% l -l *(u[subbarao])
-rw-------  1 subbarao       29 May 23 18:13 sub
.De
.Sh "Startup Files"
.PP
There are five startup files that \fBzsh\fP will read commands from:
.Ds
$ZDOTDIR/.zshenv
$ZDOTDIR/.zprofile
$ZDOTDIR/.zshrc
$ZDOTDIR/.zlogin
$ZDOTDIR/.zlogout
.De
If \fBZDOTDIR\fP is not set, then the value of \fBHOME\fP is used;
this is the usual case.
.\".KE    <--- missing .KS or .KF above
.PP
\&\fC.zshenv\fP is sourced on all invocations of the shell,
unless the \fC-f\fP option is set.  It should contain commands to set
the command search path, plus other important environment
variables.
\&\fC.zshenv\fP should not contain commands that produce output
or assume the shell is attached to a tty.
.PP
\&\fC.zshrc\fP is sourced in interactive shells.  It should contain
commands to set up aliases, functions, options, key bindings, etc.
.PP
\&\fC.zlogin\fP is sourced in login shells.  It should contain
commands that should be executed only in login shells.
\&\fC.zlogout\fP is sourced when login shells exit.
\&\fC.zprofile\fP is similar to \fC.zlogin\fP, except that it is sourced before
\&\fC.zshrc\fP.
\&\fC.zprofile\fP is meant as an alternative to \fC.zlogin\fP for
ksh fans;
the two are not intended to be used together, although this
could certainly be done if desired.
\&\fC.zlogin\fP is not the place for alias definitions, options, environment
variable settings, etc.;
as a general rule, it should not change the shell environment
at all.  Rather, it should be used to set the terminal type
and run a series of external commands (\fCfortune\fP, \fCmsgs\fP, etc).
.Sh "Shell Functions"
.PP
\fBzsh\fP also allows you to create your own commands by defining shell
functions.  For example:
.Ds
% yp () {
>       ypmatch $1 passwd.byname
> }
% yp pfalstad
pfalstad:*:3564:35:Paul John Falstad:/u/pfalstad:/usr/princeton/bin/zsh
.De
This function looks up a user in the NIS password map.
The \fC$1\fP expands to the first argument to \fCyp\fP.
The function could have been equivalently defined in one of the following
ways:
.Ds
% function yp {
>       ypmatch $1 passwd.byname
> }
% function yp () {
>       ypmatch $1 passwd.byname
> }
% function yp () ypmatch $1 passwd.byname
.De
Note that aliases are expanded when the function definition is
parsed, not when the function is executed.  For example:
.Ds
% alias ypmatch=echo
% yp pfalstad
pfalstad:*:3564:35:Paul John Falstad:/u/pfalstad:/usr/princeton/bin/zsh
.De
Since the alias was defined after the function was parsed, it has
no effect on the function's execution.
However, if we define the function again with the alias in place:
.Ds
% function yp () { ypmatch $1 passwd.byname }
% yp pfalstad
pfalstad passwd.byname
.De
it is parsed with the new alias definition in place.
Therefore, in general you must define aliases before functions.
.\".KE    <--- missing .KS or .KF above
.PP
We can make the function take multiple arguments:
.Ds
% unalias ypmatch
% yp () {
>       for i
>       do ypmatch $i passwd.byname
>       done
> }
% yp pfalstad subbarao sukthnkr
pfalstad:*:3564:35:Paul John Falstad:/u/pfalstad:/usr/princeton/bin/zsh
subbarao:*:3338:35:Kartik Subbarao:/u/subbarao:/usr/princeton/bin/zsh
sukthnkr:*:1267:35:Rahul Sukthankar:/u/sukthnkr:/usr/princeton/bin/tcsh
.De
The \fCfor i\fP loops through each of the function's arguments,
setting \fCi\fP equal to each of them in turn.
We can also make the function do something sensible
if no arguments are given:
.Ds
% yp () {
>       if (( $# == 0 ))
>       then echo usage: yp name ...; fi
>       for i; do ypmatch $i passwd.byname; done
> }
% yp
usage: yp name ...
% yp pfalstad sukthnkr
pfalstad:*:3564:35:Paul John Falstad:/u/pfalstad:/usr/princeton/bin/zsh
sukthnkr:*:1267:35:Rahul Sukthankar:/u/sukthnkr:/usr/princeton/bin/tcsh
.De
\fC$#\fP is the number of arguments supplied to the function.
If it is equal to zero, we print a usage message; otherwise,
we loop through the arguments, and \fCypmatch\fP all of them.
.\".KE    <--- missing .KS or .KF above
.PP
Here's a function that selects a random line from a file:
.Ds
% randline () {
>       integer z=$(wc -l <$1)
>       sed -n $[RANDOM % z + 1]p $1
> }
% randline /etc/motd
PHOENIX WILL BE DOWN briefly Friday morning, 5/24/91 from 8 AM to
% randline /etc/motd
SunOS Release 4.1.1 (PHOENIX) #19: Tue May 14 19:03:15 EDT 1991
% randline /etc/motd
| Please use the "msgs" command to read announcements.  Refer to the   |
% echo $z

%
.De
\fCrandline\fP has a local variable, \fCz\fP, that holds the number of
lines in the file.  \fC$[RANDOM % z + 1]\fP expands to a random number
between 1 and \fCz\fP.  An expression of the form \fC$[\fR...\fC]\fR
expands to the value of the arithmetic expression within the brackets,
and the \fBRANDOM\fP variable returns a random number each time it
is referenced.  \fC%\fP is the modulus operator, as in C.
Therefore, \fCsed -n $[RANDOM%z+1]p\fP picks a random line from its
input, from 1 to \fCz\fP.
.PP
Function definitions can be viewed with the \fCfunctions\fP builtin:
.Ds
% functions randline
randline () {
        integer z=$(wc -l <$1)
        sed -n $[RANDOM % z + 1]p $1

}
% functions
yp () {
        if let $# == 0 
        
        then
                echo usage: yp name ...
        
        fi
        for i
        do
                ypmatch $i passwd.byname
        
                done

}
randline () {
        integer z=$(wc -l <$1)
        sed -n $[RANDOM % z + 1]p $1

}
.De
Here's another one:
.Ds
% cx () { chmod +x $* }
% ls -l foo bar
-rw-r--r--  1 pfalstad       29 May 24 04:38 bar
-rw-r--r--  1 pfalstad       29 May 24 04:38 foo
% cx foo bar
% ls -l foo bar
-rwxr-xr-x  1 pfalstad       29 May 24 04:38 bar
-rwxr-xr-x  1 pfalstad       29 May 24 04:38 foo
.De
Note that this could also have been implemented as an alias:
.Ds
% chmod 644 foo bar
% alias cx='chmod +x'
% cx foo bar
% ls -l foo bar
-rwxr-xr-x  1 pfalstad       29 May 24 04:38 bar
-rwxr-xr-x  1 pfalstad       29 May 24 04:38 foo
.De
.PP
Instead of defining a lot of functions in your \fC.zshrc\fP,
all of which you may not use,
it is often better to use the \fCautoload\fP builtin.
The idea is, you create a directory where function
definitions are stored, declare the names in
your \fC.zshrc\fP, and tell the shell where to look for them.
Whenever you reference a function, the shell
will automatically load it into memory.
.Ds
% mkdir /tmp/funs
% cat >/tmp/funs/yp
ypmatch $1 passwd.byname
^D
% cat >/tmp/funs/cx
chmod +x $*
^D
% FPATH=/tmp/funs
% autoload cx yp
% functions cx yp
undefined cx ()
undefined yp ()
% chmod 755 /tmp/funs/{cx,yp}
% yp egsirer
egsirer:*:3214:35:Emin Gun Sirer:/u/egsirer:/bin/sh
% functions yp
yp () {
        ypmatch $1 passwd.byname
}
.De
This idea has other benefits.  By adding a \fC#!\fP header
to the files, you can make them double as shell scripts.
(Although it is faster to use them as functions, since a
separate process is not created.)
.Ds
% ed /tmp/funs/yp
25
i
#! /usr/local/bin/zsh
.
w
42
q
% </tmp/funs/yp
#! /usr/local/bin/zsh
ypmatch $1 passwd.byname
% /tmp/funs/yp sukthnkr
sukthnkr:*:1267:35:Rahul Sukthankar:/u/sukthnkr:/usr/princeton/bin/tcsh
.De
Now other people, who may not use \fBzsh\fP, or who don't want to
copy all of your \fC.zshrc\fP, may use these functions as shell
scripts.
.Sh "Directories"
.PP
One nice feature of \fBzsh\fP is the way it prints directories.
For example, if we set the prompt like this:
.Ds
phoenix% PROMPT='%~> '
~> cd src
~/src>
.De
the shell will print the current directory in the prompt,
using the \fC~\fP character.
However, \fBzsh\fP is smarter than most other shells in this respect:
.Ds
~/src> cd ~subbarao
~subbarao> cd ~maruchck
~maruchck> cd lib
~maruchck/lib> cd fun
~maruchck/lib/fun> foo=/usr/princeton/common/src
~maruchck/lib/fun> cd ~foo
~foo> cd ..
/usr/princeton/common> cd src
~foo> cd news/nntp
~foo/news/nntp> cd inews
~foo/news/nntp/inews>
.De
Note that \fBzsh\fP prints \fIother\fP users' directories 
in the form \fC~user\fP.  Also note that you can
set a parameter and use it as a directory name;
\fBzsh\fP will act as if \fCfoo\fP is a user
with the login directory \fC/usr/princeton/common/src\fP.
This is convenient, especially if you're sick of seeing
prompts like this:
.Ds
phoenix:/usr/princeton/common/src/X.V11R4/contrib/clients/xv/docs>
.De
If you get stuck in this position, you can give the current
directory a short name, like this:
.Ds
/usr/princeton/common/src/news/nntp/inews> inews=$PWD
/usr/princeton/common/src/news/nntp/inews> echo ~inews
/usr/princeton/common/src/news/nntp/inews
~inews>
.De
When you reference a directory in the form \fC~inews\fP,
the shell assumes that you want the directory displayed
in this form; thus simply typing \fCecho ~inews\fP or
\fCcd ~inews\fP causes the prompt to be shortened.
You can define a shell function for this purpose:
.Ds
~inews> namedir () { $1=$PWD ;  : ~$1 }
~inews> cd /usr/princeton/bin
/usr/princeton/bin> namedir pbin
~pbin> cd /var/spool/mail
/var/spool/mail> namedir spool
~spool> cd .msgs
~spool/.msgs>
.De
You may want to add this one-line function to your \fC.zshrc\fP.

\fBzsh\fP can also put the current directory in your title bar,
if you are using a windowing system.
One way to do this is with the \fCchpwd\fP function, which is
automatically executed by the shell whenever you change
directory.  If you are using xterm, this will work:
.Ds
chpwd () { print -Pn '^[]2;%~^G' }
.De
The \fC-P\fP option tells \fCprint\fP to treat its arguments like a prompt
string; otherwise the \fC%~\fP would not be expanded.
The \fC-n\fP option suppresses the terminating newline, as with \fCecho\fP.
.PP
If you are using an IRIS \fCwsh\fP, do this:
.Ds
chpwd () { print -Pn '\e2201.y%~\e234' }
.De
The \fCprint -D\fP command has other uses.  For example, to
print the current directory to standard output in short form,
you can do this:
.Ds
% print -D $PWD
~subbarao/src
.De
and to print each component of the path in short form:
.Ds
% print -D $path
/bin /usr/bin ~locbin ~locbin/X11 ~/bin
.De
.Sh "Directory Stacks"
.PP
If you use csh, you may know about directory stacks.
The \fCpushd\fP command puts the current directory on the
stack, and changes to a new directory; the \fCpopd\fP command
pops a directory off the stack and changes to it.
.Ds
phoenix% cd 
phoenix% PROMPT='Z %~> '
Z ~> pushd /tmp
/tmp ~
Z /tmp> pushd /usr/etc
/usr/etc /tmp ~
Z /usr/etc> pushd /usr/bin
/usr/bin /usr/etc /tmp ~
Z /usr/bin> popd
/usr/etc /tmp ~
Z /usr/etc> popd
/tmp ~
Z /tmp> pushd /etc
/etc /tmp ~
Z /etc> popd 
/tmp ~
.De
\fBzsh\fP's directory stack commands work similarly.  One
difference is the way \fCpushd\fP is handled if no arguments
are given.  As in csh, this exchanges the top two elements
of the directory stack:
.Ds
Z /tmp> dirs
/tmp ~
Z /tmp> pushd
~ /tmp
.De
unless the stack only has one entry:
.Ds
Z ~> popd
/tmp
Z /tmp> dirs
/tmp
Z /tmp> pushd
~ /tmp
Z ~>
.De
or unless the \fIPUSHDTOHOME\fP option is set:
.Ds
Z ~> setopt pushdtohome
Z ~> pushd
~ ~ /tmp
.De
.PP
As an alternative to using directory stacks in this manner,
we can get something like a \fIdirectory history\fP
by setting a few more options and parameters:
.Ds
~> DIRSTACKSIZE=8
~> setopt autopushd pushdminus pushdsilent pushdtohome
~> alias dh='dirs -v'
~> cd /tmp
/tmp> cd /usr
/usr> cd bin
/usr/bin> cd ../pub
/usr/pub> dh
0       /usr/pub
1       /usr/bin
2       /usr
3       /tmp
4       ~
/usr/pub> cd -3
/tmp> dh
0       /tmp
1       /usr/pub
2       /usr/bin
3       /usr
4       ~
/tmp> ls =2/df
/usr/bin/df
/tmp> cd -4
~>
.De
Note that \fC=2\fP expanded to the second directory in the
history list, and that \fCcd -3\fP recalled the third
directory in the list.
.PP
You may be wondering what all those options do.
\fIAUTOPUSHD\fP made \fCcd\fP act like \fCpushd\fP.
(\fCalias cd=pushd\fP is not sufficient, for various reasons.)
\fIPUSHDMINUS\fP swapped the meaning of \fCcd +1\fP and
\fCcd -1\fP; we want them to mean the opposite of what they mean in csh,
because it makes more sense in this scheme, and it's easier to type:
.Ds
~> dh
0       ~
1       /tmp
2       /usr/pub
3       /usr/bin
4       /usr
~> unsetopt pushdminus
~> cd +1
/tmp> dh
0       /tmp
1       ~
2       /usr/pub
3       /usr/bin
4       /usr
/tmp> cd +2
/usr/pub>
.De
\fIPUSHDSILENT\fP keeps the shell from printing
the directory stack each time we do a \fCcd\fP,
and \fIPUSHDTOHOME\fP we mentioned earlier:
.Ds
/usr/pub> unsetopt pushdsilent
/usr/pub> cd /etc
/etc /usr/pub /tmp ~ /usr/bin /usr
/etc> cd
~ /etc /usr/pub /tmp ~ /usr/bin /usr
~> unsetopt pushdtohome
~> cd
/etc ~ /usr/pub /tmp ~ /usr/bin /usr
/etc>
.De
\fBDIRSTACKSIZE\fP keeps the directory stack
from getting too large, much like \fIHISTSIZE\fP:
.Ds
/etc> setopt pushdsilent
/etc> cd /
/> cd /
/> cd /
/> cd /
/> cd /
/> cd /
/> cd /
/> cd /
/> dh
0       /
1       /
2       /
3       /
4       /
5       /
6       /
7       /
.De
.Sh "Command/Process Substitution"
.PP
Command substitution in \fBzsh\fP can take two forms.
In the traditional form, a command enclosed in
backquotes (\fC`\fP...\fC`\fP) is replaced on the command line with its output.
This is the form used by the older shells.
Newer shells (like \fBzsh\fP) also provide another form,
\fC$(\fR...\fC)\fR.  This form is much easier to nest.
.Ds
% ls -l `echo /vmunix`
-rwxr-xr-x  1 root      1209702 May 14 19:04 /vmunix
% ls -l $(echo /vmunix)
-rwxr-xr-x  1 root      1209702 May 14 19:04 /vmunix
% who | grep mad
subbarao ttyt7   May 23 15:02   (mad55sx15.Prince)
pfalstad ttyu1   May 23 16:25   (mad55sx14.Prince)
subbarao ttyu6   May 23 15:04   (mad55sx15.Prince)
pfalstad ttyv3   May 23 16:25   (mad55sx14.Prince)
% who | grep mad | awk '{print $2}'
ttyt7
ttyu1
ttyu6
ttyv3
% cd /dev; ls -l $(who |
> grep $(echo mad) |
> awk '{ print $2 }')
crwx-w----  1 subbarao  20,  71 May 23 18:35 ttyt7
crw--w----  1 pfalstad  20,  81 May 23 18:42 ttyu1
crwx-w----  1 subbarao  20,  86 May 23 18:38 ttyu6
crw--w----  1 pfalstad  20,  99 May 23 18:41 ttyv3
.De
Many common uses of command substitution, however, are
superseded by other mechanisms of \fBzsh\fP:
.Ds
% ls -l `tty`
crw-rw-rw-  1 root      20,  28 May 23 18:35 /dev/ttyqc
% ls -l $TTY
crw-rw-rw-  1 root      20,  28 May 23 18:35 /dev/ttyqc
% ls -l `which rn`
-rwxr-xr-x  1 root       172032 Mar  6 18:40 /usr/princeton/bin/rn
% ls -l =rn
-rwxr-xr-x  1 root       172032 Mar  6 18:40 /usr/princeton/bin/rn
.De
A command name with a \fC=\fP prepended is replaced with its full
pathname.  This can be very convenient.  If it's not convenient
for you, you can turn it off:
.Ds
% ls
=foo    =bar
% ls =foo =bar
zsh: foo not found
% setopt noequals
% ls =foo =bar
=foo    =bar
.De
.PP
Another nice feature is process substitution:
.Ds
% who | fgrep -f =(print -l root lemke shgchan subbarao)
root     console May 19 10:41
lemke    ttyq0   May 22 10:05   (narnia:0.0)
lemke    ttyr7   May 22 10:05   (narnia:0.0)
lemke    ttyrd   May 22 10:05   (narnia:0.0)
shgchan  ttys1   May 23 16:52   (gaudi.Princeton.)
subbarao ttyt7   May 23 15:02   (mad55sx15.Prince)
subbarao ttyu6   May 23 15:04   (mad55sx15.Prince)
shgchan  ttyvb   May 23 16:51   (gaudi.Princeton.)
.De
A command of the form \fC=(\fR...\fC)\fR is replaced with the name of a \fIfile\fP
containing its output.  (A command substitution, on the other
hand, is replaced with the output itself.)
\fCprint -l\fP is like \fCecho\fP, excepts that it prints its arguments
one per line, the way \fCfgrep\fP expects them:
.Ds
% print -l foo bar
foo
bar
.De
We could also have written:
.Ds
% who | fgrep -f =(echo 'root
> lemke
> shgchan
> subbarao')
.De
Using process substitution,
you can edit the output of a command:
.Ds
% ed =(who | fgrep -f ~/.friends)
355
g/lemke/d
w /tmp/filbar
226
q
% cat /tmp/filbar
root     console May 19 10:41
shgchan  ttys1   May 23 16:52   (gaudi.Princeton.)
subbarao ttyt7   May 23 15:02   (mad55sx15.Prince)
subbarao ttyu6   May 23 15:04   (mad55sx15.Prince)
shgchan  ttyvb   May 23 16:51   (gaudi.Princeton.)
.De
or easily read archived mail:
.Ds
% mail -f =(zcat ~/mail/oldzshmail.Z)
"/tmp/zsha06024": 84 messages, 0 new, 43 unread
>  1  U  TO: pfalstad, zsh (10)
   2  U  nytim!tim@uunet.uu.net, Re: Zsh on Sparc1 /SunOS 4.0.3
   3  U  JAM%TPN@utrcgw.utc.com, zsh fix (15)
   4  U  djm@eng.umd.edu, way to find out if running zsh? (25)
   5  U  djm@eng.umd.edu, Re: way to find out if running zsh? (17)
   6   r djm@eng.umd.edu, Meta . (18)
   7  U  jack@cs.glasgow.ac.uk, Re: problem building zsh (147)
   8  U  nytim!tim@uunet.uu.net, Re: Zsh on Sparc1 /SunOS 4.0.3
   9     ursa!jmd, Another fix... (61)
  10  U  pplacewa@bbn.com, Re: v18i084: Zsh 2.00 - A small complaint (36)
  11  U  lubkin@cs.rochester.edu, POSIX job control (34)
  12  U  yale!bronson!tan@uunet.UU.NET
  13  U  brett@rpi.edu, zsh (36)
  14  S  subbarao, zsh sucks!!!! (286)
  15  U  snibru!d241s008!d241s013!ala@relay.EU.net, zsh (165)
  16  U  nytim!tim@uunet.UU.NET, Re: Zsh on Sparc1 /SunOS 4.0.3
  17  U  subbarao, zsh is a junk shell (43)
  18  U  amaranth@vela.acs.oakland.edu, zsh (33)
43u/84 1: x
% ls -l /tmp/zsha06024
/tmp/zsha06024 not found
.De
Note that the shell creates a temporary file, and deletes it
when the command is finished.
.Ds
% diff =(ls) =(ls -F)
3c3
< fortune
---
> fortune*
10c10
< strfile
---
> strfile*
.De
If you read \fBzsh\fP's man page, you may notice that \fC<(\fR...\fC)\fR
is another form of process substitution which is similar to
\fC=(\fR...\fC)\fR.
There is an important difference between the two.
In the \fC<(\fR...\fC)\fR case, the shell creates a named pipe (FIFO)
instead of a file.  This is better, since it does not
fill up the file system; but it does not work in all cases.
In fact, if we had replaced \fC=(\fR...\fC)\fR with \fC<(\fR...\fC)\fR in 
the examples above, all of them would have stopped working
except for \fCfgrep -f <(\fR...\fC)\fR.
You can not edit a pipe, or open it as a mail folder;
\fCfgrep\fP, however, has no problem with reading
a list of words from a pipe.
You may wonder why \fCdiff <(foo) bar\fP doesn't work, since
\fCfoo | diff - bar\fP works; this is because \fCdiff\fP creates
a temporary file if it notices that one of its arguments
is \fC-\fP, and then copies its standard input to the temporary
file.
.PP
\fC>(\fR...\fC)\fR is just like \fC<(\fR...\fC)\fR except that the
command between the parentheses will get its input from the named
pipe.
.Ds
% dvips -o >(lpr) zsh.dvi
.De
.Sh "Redirection"
.PP
Apart from all the regular redirections like the Bourne shell has,
\fBzsh\fP can do more.  You can send the output of a command to more
than one file, by specifying more redirections like
.Ds
% echo Hello World >file1 >file2
.De
and the text will end up in both files.  Similarly, you can send the
output to a file and into a pipe:
.Ds
% make > make.log | grep Error
.De
The same goes for input.  You can make the input of a command come
from more than one file.
.Ds
% sort <file1 <file2 <file3
.De
The command will first get the contents of file1 as its standard
input, then those of file2 and finally the contents of file3.  This,
too, works with pipes.
.Ds
% cut -d: -f1 /etc/passwd | sort <newnames
.De
The sort will get as its standard input first the output of \fCcut\fP
and then the contents of \fCnewnames\fP.
.PP
Suppose you would like to watch the standard output of a command on
your terminal, but want to pipe the standard error to another command.
An easy way to do this in \fBzsh\fP is by redirecting the standard
error using \fC2> >(\fR...\fC)\fR.
.Ds
% find / -name games 2> >(grep -v 'Permission' > realerrors)
.De
The above redirection will actually be implemented with a regular
pipe, not a temporary named pipe.
.Sh "Aliasing"
.PP
Often-used commands can be abbreviated with an alias:
.Ds
% alias uc=uncompress
% ls
hanoi.Z
% uc hanoi
% ls
hanoi
.De
or commands with certain desired options:
.Ds
% alias fm='finger -m'
% fm root
Login name: root                        In real life: Operator
Directory: /                            Shell: /bin/csh
On since May 19 10:41:15 on console     3 days 5 hours Idle Time
No unread mail
No Plan.

% alias lock='lock -p -60000'
% lock
lock: /dev/ttyr4 on phoenix. timeout in 60000 minutes
time now is Fri May 24 04:23:18 EDT 1991
Key: 

% alias l='ls -AF'
% l /
\&.bash_history              kadb*
\&.bashrc                    lib@
\&.cshrc                     licensed/
\&.exrc                      lost+found/
\&.login                     macsyma
\&\fR...
.De
Aliases can also be used to replace old commands:
.Ds
% alias grep=egrep ps=sps make=gmake
% alias whoami='echo root'
% whoami
root
.De
or to define new ones:
.Ds
% cd /
% alias sz='ls -l | sort -n +3 | tail -10'
% sz
drwxr-sr-x  7 bin          3072 May 23 11:59 etc
drwxrwxrwx 26 root         5120 May 24 04:20 tmp
drwxr-xr-x  2 root         8192 Dec 26 19:34 lost+found
drwxr-sr-x  2 bin         14848 May 23 18:48 dev
-r--r--r--  1 root       140520 Dec 26 20:08 boot
-rwxr-xr-x  1 root       311172 Dec 26 20:08 kadb
-rwxr-xr-x  1 root      1209695 Apr 16 15:33 vmunix.old
-rwxr-xr-x  1 root      1209702 May 14 19:04 vmunix
-rwxr-xr-x  1 root      1209758 May 21 12:23 vmunix.new.kernelmap.old
-rwxr-xr-x  1 root      1711848 Dec 26 20:08 vmunix.org
% cd
% alias rable='ls -AFtrd *(R)' nrable='ls -AFtrd *(^R)'
% rable
README      func/       bin/        pub/        News/       src/
nicecolors  etc/        scr/        tmp/        iris/       zsh*
% nrable
Mailboxes/  mail/       notes
.De
(The pattern \fC*(R)\fP matches all readable files in the current
directory, and \fC*(^R)\fP matches all unreadable files.)
.PP
Most other shells have aliases of this kind (\fIcommand\fP aliases).
However, \fBzsh\fP also has \fIglobal\fP aliases, which are substituted
anywhere on a line.
Global aliases can be used to abbreviate frequently-typed
usernames, hostnames, etc.
.Ds
% alias -g me=pfalstad gun=egsirer mjm=maruchck
% who | grep me
pfalstad ttyp0   May 24 03:39   (mickey.Princeton)
pfalstad ttyp5   May 24 03:42   (mickey.Princeton)
% fm gun
Login name: egsirer                     In real life: Emin Gun Sirer
Directory: /u/egsirer                   Shell: /bin/sh
Last login Thu May 23 19:05 on ttyq3 from bow.Princeton.ED
New mail received Fri May 24 02:30:28 1991;
  unread since Fri May 24 02:30:27 1991
% alias -g phx=phoenix.princeton.edu warc=wuarchive.wustl.edu
% ftp warc
Connected to wuarchive.wustl.edu.
.De
Here are some more interesting uses.
.Ds
% alias -g M='| more' GF='| fgrep -f ~/.friends'
% who M   # \fIpipes the output of \fCwho\fI through \fCmore
% who GF  # \fIsee if your friends are on\fC
% w GF    # \fIsee what your friends are doing
.De
Another example makes use of \fBzsh\fP's process substitution.
If you run NIS, and you miss being able to do this:
.Ds
% grep pfalstad /etc/passwd
.De
you can define an alias that will seem more natural
than \fCypmatch pfalstad passwd\fP:
.Ds
% alias -g PASS='<(ypcat passwd)'
% grep pfalstad PASS
pfalstad:*:3564:35:Paul John Falstad:/u/pfalstad:/usr/princeton/bin/zsh
.De
If you're really crazy, you can even call it \fC/etc/passwd\fP:
.Ds
% alias -g /etc/passwd='<(ypcat passwd)'
% grep pfalstad /etc/passwd
pfalstad:*:3564:35:Paul John Falstad:/u/pfalstad:/usr/princeton/bin/zsh
.De
The last example shows one of the perils of global aliases;
they have a lot of potential to cause confusion.
For example, if you defined a global alias called \fC|\fP (which is
possible), \fBzsh\fP would begin to act very strangely; every pipe
symbol would be replaced with the text of your alias.
To some extent, global aliases are like macros in C;
discretion is advised in using them and in choosing names for them.
Using names in all caps is not a bad idea, especially
for aliases which introduce shell metasyntax (like \fCM\fP and \fCGF\fP
above).
.PP
Note that \fBzsh\fP aliases are not like csh aliases.  The syntax for
defining them is different, and they do not have arguments.
All your favorite csh aliases will probably not work under \fBzsh\fP.
For example, if you try:
.Ds
alias rm mv '\e!* /tmp/wastebasket'
.De
no aliases will be defined, but \fBzsh\fP will not report an error.
In csh, this line defines an alias that makes \fCrm\fP safe---files
that are \fCrm\fP'd will be moved to a temporary directory instead of
instantly destroyed.  In \fBzsh\fP's syntax, however, this line asks
the shell to print any existing alias definitions for \fCrm\fP,
\fCmv\fP, or \fC!*\ /tmp/wastebasket\fP.  Since there are none, most
likely, the shell will not print anything, although \fCalias\fP will
return a nonzero exit code.  The proper syntax is this:
.Ds
alias rm='mv \e!* /tmp/wastebasket'
.De
However, this won't work either:
.Ds
% rm foo.dvi
zsh: no matches found: !*
.De
While this makes \fCrm\fP safe, it is certainly not what the user
intended.  In \fBzsh\fP, you must use a shell function for this:
.Ds
% unalias rm
% rm () { mv $* /tmp/wastebasket }
% rm foo.dvi
% ls /tmp/wastebasket
foo.dvi
.De
While this is much cleaner and easier to read (I hope you will
agree), it is not csh-compatible.  Therefore, a script to convert
csh aliases and variables has been provided.  You should only need to use it
once, to convert all your csh aliases and parameters to \fBzsh\fP format:
.Ds
% csh
csh> alias
l       ls -AF
more    less
on      last -2 !:1 ; who | grep !:1
csh> exit
% c2z >neat_zsh_aliases
% cat neat_zsh_aliases
alias l='ls -AF'
alias more='less'
on () { last -2 $1 ; who | grep $1 }
\&...
.De
The first two aliases were converted to regular \fBzsh\fP aliases, while
the third, since it needed to handle arguments, was converted to
a function.  \fCc2z\fP can convert most aliases to \fBzsh\fP format without
any problems.  However, if you're using some really arcane csh tricks,
or if you have an alias with a name like \fCdo\fP (which is reserved
in \fBzsh\fP), you may have to fix some of the aliases by hand.
.PP
The \fCc2z\fP script checks your csh setup, and produces a list
of \fBzsh\fP commands which replicate your aliases and parameter settings
as closely as possible.  You could include its output in your
startup file, \fC.zshrc\fP.
.Sh "History"
.PP
There are several ways to manipulate history in \fBzsh\fP.
One way is to use csh-style \fC!\fP history:
.Ds
% /usr/local/bin/!:0 !-2*:s/foo/bar/ >>!$
.De
If you don't want to use this, you can turn it off
by typing \fCsetopt nobanghist\fP.  If you are afraid of accidentally
executing the wrong command you can set the \fIHISTVERIFY\fP option.
If this option is set, commands that result from history expansion
will not be executed immediately, but will be put back into the editor
buffer for further consideration.
.PP
If you're not familiar with \fC!\fP history, here follows some
explanation.  History substitutions always start with a \fC!\fP,
commonly called \*Qbang\*U.  After the \fC!\fP comes an (optional)
designation of which \*Qevent\*U (command) to use, then a colon, and
then a designation of what word of that command to use.  For example,
\fC!-\fIn\fR refers to the command \fIn\fP commands ago.
.Ds
% ls
foo  bar
% cd foo
% !-2
ls
baz  bam
.De
No word designator was used, which means that the whole command
referred to was repeated.  Note that the shell will echo the result of
the history substitution.  The word designator can, among other
things, be a number indicating the argument to use, where \fC0\fP is
the command.
.Ds
% /usr/bin/ls foo
foo
% !:0 bar
/usr/bin/ls bar
bar
.De
In this example, no event designator was used, which tells \fBzsh\fP
to use the previous command.  A \fC$\fP specifies the last argument
.Ds
% mkdir /usr/local/lib/emacs/site-lisp/calc
% cd !:$
cd /usr/local/lib/emacs/site-lisp/calc
.De
If you use more words of the same command, only the first \fC!\fP
needs an event designator.
.Ds
% make prig >> make.log
make: *** No rule to make target `prig'.  Stop.
% cd src
% !-2:0 prog >> !:$
make prog >> make.log
.De
This is different from csh, where a bang with no event designator
always refers to the previous command.  If you actually like this
behaviour, set the \fICSHJUNKIEHISTORY\fP option.
.Ds
% setopt cshjunkiehistory
% !-2:0 prog2 >> !:$
make prog2 >> cshjunkiehistory
.De
Another way to use history is to use the \fCfc\fP command.  For
example, if you type an erroneous command:
.Ds
% for i in `cat /etc/clients` 
 do 
 rpu $i 
 done
zsh: command not found: rpu
zsh: command not found: rpu
zsh: command not found: rpu
\&\fR...
.De
typing \fCfc\fP will execute an editor on this command, allowing
you to fix it.  (The default editor is \fCvi\fP, by the way,
not \fCed\fP).
.Ds
% fc
49
/rpu/s//rup/p
 rup $i 
w
49
q
for i in `cat /etc/clients` 
 do 
 rup $i 
 done
        beam    up  2 days, 10:17,    load average: 0.86, 0.80, 0.50
         bow    up  4 days,  8:41,    load average: 0.91, 0.80, 0.50
        burn    up          17:18,    load average: 0.91, 0.80, 0.50
       burst    up  9 days,  1:49,    load average: 0.95, 0.80, 0.50
         tan    up          11:14,    load average: 0.91, 0.80, 0.50
       bathe    up  3 days, 17:49,    load average: 1.84, 1.79, 1.50
        bird    up  1 day,   9:13,    load average: 1.95, 1.82, 1.51
      bonnet    up  2 days, 21:18,    load average: 0.93, 0.80, 0.50
\&\fR...
.De
A variant of the \fCfc\fP command is \fCr\fP, which redoes the last
command, with optional changes:
.Ds
% echo foo
foo
% r
echo foo
foo

% echo foo
foo
% r foo=bar
echo bar
bar
.De
.Sh "Command Line Editing"
.PP
\fBzsh\fP's command line editor, \fBZLE\fP, is quite powerful.
It is designed to emulate either emacs or vi; the default
is emacs.  To set the bindings for vi mode, type \fCbindkey -v\fP.  If
your \fBEDITOR\fP or \fBVISUAL\fP environment variable is vi,
\fBzsh\fP will use vi emulation by default.  You can then switch to
emacs mode with \fCbindkey -e\fP.
.PP
In addition to basic editing, the shell allows you to 
recall previous lines in the history.  In emacs mode,
this is done with \fI^P\fP (control-P) or (on many terminals) with the
cursor-up key:
.Ds
% ls ~
-           README      file        mail        pub         tmp
Mailboxes   bin         func        nicecolors  scr         zsh
News        etc         iris        notes       src
% echo foobar
foobar
% \fI^P\fC
% echo foobar\fI^P\fC
% ls ~_
.De
Pressing \fI^P\fP once brings up the previous line (\fCecho foobar\fP);
pressing it again brings up the line before that (\fCls ~\fP).
The cursor is left at the end of the line, allowing you to
edit the line if desired before executing it.
In many cases, \fBZLE\fP eliminates the need for the \fCfc\fP command,
since it is powerful enough to handle even multiline commands:
.Ds
% for i in a b c d e
> do
> echo $i
> done
a
b
c
d
e
% \fI^P\fC
% for i in a b c d e 
 do 
 echo $i 
 done_
.De
Now you can just move up to the part you want to change...
.Ds
% for i in \kxa\l'|\nxu\(ul' b c d e
 do 
 echo $i 
 done
.De
change it, and execute the new command.
.Ds
% for i in f g h i j
 do 
 echo $i 
 done
f
g
h
i
j
.De
Also, you can search the history for a certain command using
\fIESC-P\fP, this will look for the last command that started with the
(part of the) word at the beginning of the current line.  Hitting
\fIESC-P\fP another time gets you the command before that, etc.
.Ds
% set \fIESC-P\fC
% setopt autolist \fIESC-P\fC
% setopt nocorrect_
.De
Another way is to do an incremental search, emacs-style:
.Ds
% \fI^R\fC
% _
i-search:

% l\kxs\l'|\nxu\(ul' /usr/bin
i-search: l

% date > foofile\kx.\l'|\nxu\(ul'c
i-search: le
.De
Suppose you have retrieved an old history event in one of these ways
and would like to execute several consecutive old commands starting
with this one.  \fC^O\fP will execute the current command and then put
the next command from the history into the editor buffer.  Typing
\fC^O\fP several times will therefore reexecute several consecutive
commands from the history.  Of course, you can edit some of those
commands in between.
.PP
In addition to completion (see below), \fITAB\fP performs expansion if
possible.
.Ds
% ls *.c\fITAB\fC
% ls foofile.c fortune.c rnd.c strfile.c unstr.c_
.De
For example, suppose you have a bunch of weird files in an important
directory:
.Ds
% ls
  * * *       ; & % $??foo  dspfok        foo.c
  !"foo"!       ` \e `         foo           rrr
.De
You want to remove them, but you don't want to damage \fCfoo.c\fP.
Here is one way to do this:
.Ds
% rm *\fITAB\fC
% rm \e \e \e*\e \e*\e \e*\e \e \e  \e!\e"foo\e"\e! \e;\e \e&\e %\e \e$'
''
'foo \e`\e \e\e\e \e` dspfok foo foo.c rrr_
.De
When you expand \fC*\fP, \fBzsh\fP inserts the names of all the files
into the editing buffer, with proper shell quoting.
Now, just move back and remove \fCfoo.c\fP from the buffer:
.Ds
% rm \e \e \e*\e \e*\e \e*\e \e \e  \e!\e"foo\e"\e! \e;\e \e&\e %\e \e$'
''
'foo \e`\e \e\e\e \e` dspfok foo \kxr\l'|\nxu\(ul'rr
.De
and press return.
Everything except \fCfoo.c\fP will be deleted from the directory.  If
you do not want to actually expand the current word, but would like to
see what the matches are, type \fC^Xg\fP.
.Ds
% rm f*\fI^Xg\fP
foo    foo.c
% rm f*_
.De
Here's another trick; let's say you have typed this command in:
.Ds
% gcc -o x.out foob.c -g -Wpointer-arith -Wtrigraphs_
.De
and you forget which library you want.  You need to escape
out for a minute and check by typing
\fCls /usr/lib\fP, or some other such command;
but you don't want to retype the whole command again,
and you can't press return now because the current command
is incomplete.
In \fBzsh\fP, you can put the line on the \fIbuffer stack\fP, using
\fIESC-Q\fP, and type some other commands.  The next time a prompt is printed,
the \fCgcc\fP line will be popped off the stack and put
in the editing buffer automatically; you can then enter the
proper library name and press return (or, \fIESC-Q\fP again and look
for some other libraries whose names you forgot).
.PP
A similar situation: what if you forget the option to gcc that
finds bugs using AI techniques?  You could either use \fIESC-Q\fP
again, and type \fCman gcc\fP, or you could press \fIESC-H\fP, which
essentially does the same thing; it puts the current line on
the buffer stack, and executes the command \fCrun-help gcc\fP,
where \fCrun-help\fP is an alias for \fCman\fP.
.PP
Another interesting command is \fIESC-A\fP.  This executes the
current line, but retains it in the buffer, so that it appears
again when the next prompt is printed.
Also, the cursor stays in the same place.
This is useful for executing a series of similar commands:
.Ds
% cc grok.c -g -lc -lgl -lsun -lmalloc -Bstatic -o b.out
% cc fubar.c -g -lc -lgl -lsun -lmalloc -Bstatic -o b.out
% cc fooble.c -g -lc -lgl -lsun -lmalloc -Bstatic -o b.out
.De
.PP
The \fIESC-'\fP command is useful for managing the shell's quoting
conventions.  Let's say you want to print this string:
.Ds
don't do that; type 'rm -rf \e*', with a \e before the *.
.De
All that is necessary is to type it into the editing buffer:
.Ds
% don't do that; type 'rm -rf \e*', with a \e before the *.
.De
press \fIESC-'\fP (escape-quote):
.Ds
% 'don'\e''t do that; type '\e''rm -rf \e*'\e'', with a \e before the *.'
.De
then move to the beginning and add the \fCecho\fP command.
.Ds
% echo 'don'\e''t do that; type '\e''rm -rf \e*'\e'', with a \e before the *.'
don't do that; type 'rm -rf \e*', with a \e before the *.
.De
Let's say you want to create an alias to do this \fCecho\fP command.
This can be done by recalling the line with \fI^P\fP and pressing
\fIESC-'\fP again:
.Ds
% 'echo '\e''don'\e''\e'\e'''\e''t do that; type '\e''\e'\e'''\e''rm -rf
\e*'\e''\e'\e'''\e'', with a \e before the *.'\e'''
.De
and then move to the beginning and add the command to create
an alias.
.Ds
% alias zoof='echo '\e''don'\e''\e'\e'''\e''t do that; type '\e''\e'\e'''\e''rm
-rf \e*'\e''\e'\e'''\e'', with a \e before the *.'\e'''
% zoof
don't do that; type 'rm -rf \e*', with a \e before the *.
.De
If one of these fancy editor commands changes your command line in a
way you did not intend, you can undo changes with \fC^_\fP, if you can
get it out of your keyboard, or \fC^X^U\fP, otherwise.
.PP
Another use of the editor is to edit the value of variables.
For example, an easy way to change your path is to use
the \fCvared\fP command:
.Ds
% vared PATH
> /u/pfalstad/scr:/u/pfalstad/bin/sun4:/u/maruchck/scr:/u/subbarao/bin:/u/maruc
hck/bin:/u/subbarao/scripts:/usr/princeton/bin:/usr/ucb:/usr/bin:/bin:/usr/host
s:/usr/princeton/bin/X11:/./usr/lang:/./usr/etc:/./etc
.De
You can now edit the path.  When you press return, the contents
of the edit buffer will be assigned to \fBPATH\fP.
.Sh "Completion"
.PP
Another great \fBzsh\fP feature is completion.  If you hit \fITAB\fP, \fBzsh\fP
will complete all kinds of stuff.  Like commands or filenames:
.Ds
% comp\fITAB\fC
% compress _

% ls nic\fITAB\fC
% ls nicecolors _

% ls /usr/pr\fITAB\fC
% ls /usr/princeton/_

% ls -l =com\fITAB\fC
% ls -l =compress _
.De
If the completion is ambiguous, the editor will beep.  If you find
this annoying, you can set the \fINOLISTBEEP\fP option.  Completion
can even be done in the middle of words.  To use this, you will have
to set the \fICOMPLETEINWORD\fP option:
.Ds
% setopt completeinword
% ls /usr/p\kxt\l'|\nxu\(ul'on\fITAB\fC
% ls /usr/prince\kxt\l'|\nxu\(ul'on/
% setopt alwaystoend
% ls /usr/p\kxt\l'|\nxu\(ul'on\fITAB\fC
% ls /usr/princeton/_
.De
You can list possible completions by pressing \fI^D\fP:
.Ds
% ls /vmu\fITAB \(embeep\(em\fC
% ls /vmunix_
% ls /vmunix\fI^D\fC
vmunix                    vmunix.old                
vmunix.new.kernelmap.old  vmunix.org
.De
Or, you could just set the \fIAUTOLIST\fP option:
.Ds
% setopt autolist
% ls /vmu\fITAB \(embeep\(em\fC
vmunix                    vmunix.old                
vmunix.new.kernelmap.old  vmunix.org
% ls /vmunix_
.De
If you like to see the types of the files in these lists, like in
\fCls\ -F\fP, you can set the \fILISTTYPES\fP option.  Together with
\fIAUTOLIST\fP you can use \fILISTAMBIGUOUS\fP.  This will only list
the possibilities if there is no unambiguous part to add:
.Ds
% setopt listambiguous
% ls /vmu\fITAB \(embeep\(em\fC
% ls /vmunix_\fITAB \(embeep\(em\fC
vmunix                    vmunix.old                
vmunix.new.kernelmap.old  vmunix.org
.De
If you don't want several of these listings to scroll the screen so
much, the \fIALWAYSLASTPROMPT\fP option is useful.  If set, you can
continue to edit the line you were editing, with the completion
listing appearing beneath it.
.PP
Another interesting option is \fIMENUCOMPLETE\fP.  This affects the
way \fITAB\fP works.  Let's look at the \fC/vmunix\fP example again:
.Ds
% setopt menucomplete
% ls /vmu\fITAB\fC
% ls /vmunix\fITAB\fC
% ls /vmunix.new.kernelmap.old\fITAB\fC
% ls /vmunix.old_
.De
Each time you press \fITAB\fP, it displays the next possible completion.
In this way, you can cycle through the possible completions until
you find the one you want.
.PP
The \fIAUTOMENU\fP option makes a nice compromise between this method
of completion and the regular method.  If you set this option,
pressing \fITAB\fP once completes the unambiguous part normally,
pressing the \fITAB\fP key repeatedly after an ambiguous completion
will cycle through the possible completions.
.PP
Another option you could set is \fIRECEXACT\fP, which causes
exact matches to be accepted, even if there are other possible
completions:
.Ds
% setopt recexact
% ls /vmu\fITAB \(embeep\(em\fC
vmunix                    vmunix.old                
vmunix.new.kernelmap.old  vmunix.org
% ls /vmunix_\fITAB\fC
% ls /vmunix _
.De
To facilitate the typing of pathnames, a slash will be added whenever
a directory is completed.  Some computers don't like the spurious
slashes at the end of directory names.  In that case, the
\fIAUTOREMOVESLASH\fP option comes to rescue.  It will remove these
slashes when you type a space or return after them.
.PP
The \fIfignore\fP variable lists suffixes of files to ignore
during completion.
.Ds
% ls foo\fITAB \(embeep\(em\fC
foofile.c  foofile.o
% fignore=( .o \e~ .bak .junk )
% ls foo\fITAB\fP
% ls foofile.c _
.De
Since \fCfoofile.o\fP has a suffix that is in the \fCfignore\fP list,
it was not considered a possible completion of \fCfoo\fP.
.PP
Username completion is also supported:
.Ds
% ls ~pfal\fITAB\fC
% ls ~pfalstad/_
.De
and parameter name completion:
.Ds
% echo $ORG\fITAB\fC
% echo $ORGANIZATION _
% echo ${ORG\fITAB\fC
% echo ${ORGANIZATION _
.De
Note that in the last example a space is added after the completion as
usual.  But if you want to add a colon or closing brace, you probably
don't want this extra space.  Setting the \fIAUTOPARAMKEYS\fP option
will automatically remove this space if you type a colon or closing
brace after such a completion.
.PP
There is also option completion:
.Ds
% setopt nocl\fITAB\fC
% setopt noclobber _
.De
and binding completion:
.Ds
% bindkey '^X^X' pu\fITAB\fC
% bindkey '^X^X' push-line _
.De
The \fCcompctl\fP command is used to control completion of the
arguments of specific commands.  For example, to specify that certain
commands take other commands as arguments, you use \fCcompctl -c\fP:
.Ds
% compctl -c man nohup
% man upt\fITAB\fC
% man uptime _
.De
To specify that a command should complete filenames, you should use
\fCcompctl -f\fP.  This is the default.  It can be combined with \fC-c\fP,
as well.
.Ds
% compctl -cf echo
% echo upt\fITAB\fC
% echo uptime _

% echo fo\fITAB\fC
% echo foo.c
.De
Similarly, use \fC-o\fP to specify options, \fC-v\fP to specify
variables, and \fC-b\fP to specify bindings.
.Ds
% compctl -o setopt unsetopt
% compctl -v typeset vared unset export
% compctl -b bindkey
.De
You can also use \fC-k\fP to specify a custom list of keywords to use
in completion.  After the \fC-k\fP comes either the name of an array
or a literal array to take completions from.
.Ds
% ftphosts=(ftp.uu.net wuarchive.wustl.edu)
% compctl -k ftphosts ftp
% ftp wu\fITAB\fC
% ftp wuarchive.wustl.edu _

% compctl -k '(cpirazzi subbarao sukthnkr)' mail finger
% finger cp\fITAB\fC
% finger cpirazzi _
.De
To better specify the files to complete for a command, use the
\fC-g\fP option which takes any glob pattern as an argument.  Be sure
to quote the glob patterns as otherwise they will be expanded when the
\fCcompctl\fP command is run.
.Ds
% ls
letter.tex  letter.dvi  letter.aux  letter.log  letter.toc
% compctl -g '*.tex' latex
% compctl -g '*.dvi' xdvi dvips
% latex l\fITAB\fC
% latex letter.tex _
% xdvi l\fITAB\fC
% xdvi letter.dvi _
.De
Glob patterns can include qualifiers within parentheses.  To rmdir
only directories and cd to directories and symbolic links pointing to
them:
.Ds
% compctl -g '*(-/)' cd
% compctl -g '*(/)' rmdir
.De
RCS users like to run commands on files which are not in the current
directory, but in the RCS subdirectory where they all get \fC,v\fP
suffixes.  They might like to use
.Ds
% compctl -g 'RCS/*(:t:s/\e,v//)' co rlog rcs
% ls RCS
builtin.c,v  lex.c,v      zle_main.c,v
% rlog bu\fITAB\fC
% rlog builtin.c _
.De
The \fC:t\fP modifier keeps only the last part of the pathname and the
\fC:s/\e,v//\fP will replace any \fC,v\fP by nothing.
.PP
The \fC-s\fP flag is similar to \fC-g\fP, but it uses all expansions,
instead of just globbing, like brace expansion, parameter substitution
and command substitution.
.Ds
% compctl -s '$(setopt)' unsetopt
.De
will only complete options which are actually set to be arguments to
\fCunsetopt\fP.
.PP
Sometimes a command takes another command as its argument.  You can
tell \fBzsh\fP to complete commands as the first argument to such a
command and then use the completion method of the second command.  The
\fC-l\fP flag with a null-string argument is used for this.
.Ds
% compctl -l '' nohup exec
% nohup comp\fITAB\fC
% nohup compress _
% nohup compress fil\fITAB\fC
% nohup compress filename _
.De
Sometimes you would like to run really complicated commands to find
out what the possible completions are.  To do this, you can specify a
shell function to be called that will assign the possible completions
to a variable called reply.  Note that this variable must be an array.
Here's another (much slower) way to get the completions for \fCco\fP
and friends:
.Ds
% function getrcs {
> reply=()
> for i in RCS/*
>   do
>   reply=($reply[*] $(basename $i ,v))
>   done
> }
% compctl -K getrcs co rlog rcs
.De
Some command arguments use a prefix that is not a part of the things
to complete.  The kill builtin command takes a signal name after a
\fC-\fP.  To make such a prefix be ignored in the completion process,
you can use the \fC-P\fP flag.
.Ds
% compctl -P - -k signals kill
% kill -H\fITAB\fP
% kill -HUP _
.De
TeX is usually run on files ending in \fC.tex\fP, but also sometimes
on other files.  It is somewhat annoying to specify that the arguments
of TeX should end in \fC.tex\fP and then not be able to complete these
other files.  Therefore you can specify things like \*QComplete to
files ending in \fC.tex\fP if available, otherwise complete to any
filename.\*U.  This is done with \fIxor\fPed completion:
.Ds
% compctl -g '*.tex' + -f tex
.De
The \fC+\fP tells the editor to only take the next thing into account
if the current one doesn't generate any matches.  If you have not
changed the default completion, the above example is in fact
equivalent to
.Ds
% compctl -g '*.tex' + tex
.De
as a lone \fC+\fP at the end is equivalent to specifying the default
completion after the \fC+\fP.  This form of completion is also
frequently used if you want to run some command only on a certain type
of files, but not necessarily in the current directory.  In this case
you will want to complete both files of this type and directories.
Depending on your preferences you can use either of
.Ds
% compctl -g '*.ps' + -g '*(-/)' ghostview
% compctl -g '*.ps *(-/)' ghostview
.De
where the first one will only complete directories (and symbolic links
pointing to directories) if no postscript file matches the already
typed part of the argument.
.Sh "Extended completion"
.PP
If you play with completion, you will soon notice that you would like
to specify what to complete, depending on what flags you give to the
command and where you are on the command line.  For example, a command
could take any filename argument after a \fC-f\fP flag, a username
after a \fC-u\fP flag and an executable after a \fC-x\fP flag.  This
section will introduce you to the ways to specify these things.  To
many people it seems rather difficult at first, but taking the trouble
to understand it can save you lots of typing in the end.  Even I keep
being surprised when \fBzsh\fP manages to complete a small or even
empty prefix to the right file in a large directory.
.PP
To tell \fBzsh\fP about these kinds of completion, you use \*Qextended
completion\*U by specifying the \fC-x\fP flag to compctl.  The
\fC-x\fP flag takes a list of patterns/flags pairs.  The patterns
specify when to complete and the flags specify what.  The flags are
simply those mentioned above, like \fC-f\fP or \fC-g \fIglob
pattern\fR.
.PP
As an example, the \fCr[\fIstring1\fC,\fIstring2\fC]\fR pattern
matches if the cursor is after something that starts with
\fIstring1\fP and before something that starts with \fIstring2\fP.
The \fIstring2\fP is often something that you do not want to match
anything at all.
.Ds
% ls
foo1   bar1   foo.Z  bar.Z
% compctl -g '^*.Z' -x 'r[-d,---]' -g '*.Z' -- compress
% compress f\fITAB\fP
% compress foo1 _
% compress -d f\fITAB\fP
% compress -d foo.Z _
.De
In the above example, if the cursor is after the \fC-d\fP the pattern
will match and therefore \fBzsh\fP uses the \fC-g *.Z\fP flag that will only
complete files ending in \fC.Z\fP.  Otherwise, if no pattern matches,
it will use the flags before the \fC-x\fP and in this case complete
every file that does not end in \fC.Z\fP.
.PP
The \fCs[\fIstring\fC]\fR pattern matches if the current word starts
with \fIstring\fP.  The \fIstring\fP itself is not considered to be
part of the completion.
.Ds
% compctl -x 's[-]' -k signals -- kill
% kill -H\fITAB\fP
% kill -HUP _
.De
The \fCtar\fP command takes a tar file as an argument after the
\fC-f\fP option.  The \fCc[\fIoffset\fC,\fIstring\fC]\fR pattern
matches if the word in position \fIoffset\fP relative to the current
word is \fIstring\fP.  More in particular, if \fIoffset\fP is -1, it
matches if the previous word is \fIstring\fP.  This suggests
.Ds
% compctl -f -x 'c[-1,-f]' -g '*.tar' -- tar
.De
But this is not enough.  The \fC-f\fP option could be the last of a
longer string of options.  \fCC[\fR...\fC,\fR...\fC]\fR is just like
\fCc[\fR...\fC,\fR...\fC]\fR, except that it uses glob-like pattern
matching for \fIstring\fP.  So
.Ds
% compctl -f -x 'C[-1,-*f]' -g '*.tar' -- tar
.De
will complete tar files after any option string ending in an \fCf\fP.
But we'd like even more.  Old versions of tar used all options as the
first argument, but without the minus sign.  This might be
inconsistent with option usage in all other commands, but it is still
supported by newer versions of \fCtar\fP.  So we would also like to
complete tar files if the first argument ends in an \fCf\fP and we're
right behind it.
.PP
We can `and' patterns by putting them next to each other with a space
between them.  We can `or' these sets by putting comma's between them.
We will also need some new patterns.  \fCp[\fInum\fC]\fR will match if
the current argument (the one to be completed) is the \fInum\fPth
argument.  \fCW[\fIindex\fC,\fIpattern\fC]\fR will match if the
argument in place \fIindex\fP matches the \fIpattern\fP.  This gives
us
.Ds
% compctl -f -x 'C[-1,-*f] , W[1,*f] p[2]' -g '*.tar' -- tar
.De
In words: If the previous argument is an option string that ends in an
\fCf\fP, or the first argument ended in an \fCf\fP and it is now the
second argument, then complete only filenames ending in \fC.tar\fP.
.PP
All the above examples used only one set of patterns with one
completion flag.  You can use several of these pattern/flag pairs
separated by a \fC-\fP.  The first matching pattern will be used.
Suppose you have a version of \fCtar\fP that supports compressed files
by using a \fC-Z\fP option.  Leaving the old tar syntax aside for a
moment, we would like to complete files ending in \fC.tar.Z\fP if a
\fC-Z\fP option has been used and files ending in \fC.tar\fP
otherwise, all this only after a \fC-f\fP flag.  Again, the \fC-Z\fP
can be alone or it can be part of a longer option string, perhaps the
same as that of the \fC-f\fP flag.  Here's how to do it; note the
backslash and the secondary prompt which are not part of the
\fCcompctl\fP command.
.Ds
% compctl -f -x 'C[-1,-*Z*f] , R[-*Z*,---] C[-1,-*f]' -g '*.tar.Z' - \e
> 'C[-1,-*f]' -g '*.tar' -- tar
.De
The first pattern set tells us to match if either the previous
argument was an option string including a \fCZ\fP and ending in an
\fCf\fP or there was an option string with a \fCZ\fP somewhere and the
previous word was any option string ending in an \fCf\fP.  If this is
the case, we need a compressed tar file.  Only if this is not the case
the second pattern set will be considered.  By the way,
\fCR[\fIpattern1\fC,\fIpattern2\fC]\fR is just like
\fCr[\fR...\fC,\fR...\fC]\fR except that it uses pattern matching with
shell metacharacters instead of just strings.
.PP
You will have noticed the \fC--\fP before the command name.  This ends
the list of pattern/flag pairs of \fC-x\fP.  It is usually used just
before the command name, but you can also use an extended completion
as one part of a list of xored completions, in which case the \fC--\fP
appears just before one of the \fC+\fP signs.
.PP
Note the difference between using extended completion as part of a
list of xored completions as in
.Ds
% ls
foo  bar
% compctl -x 'r[-d,---]' -g '*.Z' -- + -g '^*.Z' compress
% compress -d f\fITAB\fP
% compress -d foo _
.De
and specifying something before the \fC-x\fP as in
.Ds
% compctl -g '^*.Z' -x 'r[-d,---]' -g '*.Z' -- compress
% compress -d f\fITAB\fP
% compress -d f_
.De
In the first case, the alternative glob pattern (\fC^*.Z\fP) will be
used if the first part does not generate any possible completions,
while in the second case the alternative glob pattern will only be
used if the \fCr[\fR...\fC]\fR pattern doesn't match.
.Sh "Bindings"
.PP
Each of the editor commands we have seen was actually a function bound
by default to a certain key.  The real names of the commands are:
.Ds
\fCexpand-or-complete   \fITAB\fR
\fCpush-line            \fIESC-Q\fR
\fCrun-help             \fIESC-H\fR
\fCaccept-and-hold      \fIESC-A\fR
\fCquote-line           \fIESC-'\fR
.De
These bindings are arbitrary; you could change them if you want.
For example, to bind \fCaccept-line\fP to \fI^Z\fP:
.Ds
% bindkey '^Z' accept-line
.De
Another idea would be to bind the delete key to \fCdelete-char\fP;
this might be convenient if you use \fI^H\fP for backspace.
.Ds
% bindkey '^?' delete-char
.De
Or, you could bind \fI^X\fP\fI^H\fP to \fCrun-help\fP:
.Ds
% bindkey '^X^H' run-help
.De
Other examples:
.Ds
% bindkey '^X^Z' universal-argument
% bindkey ' ' magic-space
% bindkey -s '^T' 'uptime
> '
% bindkey '^Q' push-line-or-edit
.De
\fCuniversal-argument\fP multiplies the next command by 4.
Thus \fI^X\fP\fI^Z\fP\fI^W\fP might delete the last four words on the line.
If you bind space to \fCmagic-space\fP, then csh-style history
expansion is done on the line whenever you press the space bar.
.PP
Something that often happens is that I am typing a multiline command
and discover an error in one of the previous lines.  In this case,
\fCpush-line-or-edit\fP will put the entire multiline construct into
the editor buffer.  If there is only a single line, it is equivalent
to \fCpush-line\fP.
.PP
The \fC-s\fP flag to \fCbindkey\fP specifies that you are binding the key
to a string, not a command.  Thus \fCbindkey -s '^T' 'uptime\en'\fP
lets you VMS lovers get the load average whenever you press \fI^T\fP.
.PP
If you have a NeXT keyboard, the one with the \fC|\fP and \fC\e\fP keys
very inconveniently placed, the following
bindings may come in handy:
.Ds
% bindkey -s '\ee/' '\e\e'
% bindkey -s '\ee=' '|'
.De
Now you can type \fIALT-/\fP to get a backslash, and \fIALT-=\fP to
get a vertical bar.  This only works inside \fBzsh\fP, of course;
\fCbindkey\fP has no effect on the key mappings inside \fCtalk\fP
or \fCmail\fP, etc.
.PP
Some people like to bind \fC^S\fP and \fC^Q\fP to editor commands.
Just binding these has no effect, as the terminal will catch them and
use them for flow control.  You could unset them as stop and start
characters, but most people like to use these for external commands.
The solution is to set the \fINOFLOWCONTROL\fP option.  This will
allow you to bind the start and stop characters to editor commands,
while retaining their normal use for external commands.
.Sh "Parameter Substitution"
.PP
In \fBzsh\fP, parameters are set like this:
.Ds
% foo=bar
% echo $foo
bar
.De
Spaces before or after the \fC=\fP are frowned upon:
.Ds
% foo = bar
zsh: command not found: foo
.De
Also, \fCset\fP doesn't work for setting parameters:
.Ds
% set foo=bar
% set foo = bar
% echo $foo

%
.De
Note that no error message was printed.  This is because both
of these commands were perfectly valid; the \fCset\fP builtin
assigns its arguments to the \fIpositional parameters\fP
(\fC$1\fP, \fC$2\fP, etc.).
.Ds
% set foo=bar
% echo $1
foo=bar
% set foo = bar
% echo $3 $2
bar =
.De
If you're really intent on using the csh syntax, define a
function like this:
.Ds
% set () {
>    eval "$1$2$3"
> }
% set foo = bar
% set fuu=brrr
% echo $foo $fuu
bar brrr
.De
But then, of course you can't use the form of \fCset\fP with
options, like \fCset -F\fP (which turns off filename generation).
Also, the \fCset\fP command by itself won't list all the parameters
like it should.
To get around that you need a \fCcase\fP statement:
.Ds
% set () {
>    case $1 in
>    -*|+*|'') builtin set $* ;;
>    *) eval "$1$2$3" ;;
>    esac
> }
.De
For the most part, this should make csh users happy.
.PP
The following sh-style operators are supported in \fBzsh\fP:
.Ds
% unset null
% echo ${foo-xxx}
bar
% echo ${null-xxx}
xxx
% unset null
% echo ${null=xxx}
xxx
% echo $null
xxx
% echo ${foo=xxx}
bar
% echo $foo
bar
% unset null
% echo ${null+set}

% echo ${foo+set}
set
.De
Also, csh-style \fC:\fP modifiers may be appended to a parameter
substitution.
.Ds
% echo $PWD
/home/learning/pf/zsh/zsh2.00/src
% echo $PWD:h
/home/learning/pf/zsh/zsh2.00
% echo $PWD:h:h
/home/learning/pf/zsh
% echo $PWD:t
src
% name=foo.c
% echo $name
foo.c
% echo $name:r
foo
% echo $name:e
c
.De
The equivalent constructs in ksh (which are also supported in \fBzsh\fP)
are a bit more general and easier to remember.
When the shell expands \fC${foo#\fR\fIpat\fR\fC}\fR,
it checks to see if \fIpat\fP matches a substring at the beginning
of the value
of \fCfoo\fP.  If so, it removes that portion of \fCfoo\fP, using the shortest
possible match.
With \fC${foo##\fR\fIpat\fR\fC}\fR, the longest possible match is removed.
\fC${foo%\fR\fIpat\fR\fC}\fR and \fC${foo%%\fR\fIpat\fR\fC}\fR remove the match
from the end.
Here are the ksh equivalents of the \fC:\fP modifiers:
.Ds
% echo ${PWD%/*}
/home/learning/pf/zsh/zsh2.00
% echo ${PWD%/*/*}
/home/learning/pf/zsh
% echo ${PWD##*/}
src
% echo ${name%.*}
foo
% echo ${name#*.}
c
.De
\fBzsh\fP also has upper/lowercase modifiers:
.Ds
% xx=Test
% echo $xx:u
TEST
% echo $xx:l
test
.De
and a substitution modifier:
.Ds
% echo $name:s/foo/bar/
bar.c
% ls
foo.c    foo.h    foo.o    foo.pro
% for i in foo.*; mv $i $i:s/foo/bar/
% ls
bar.c    bar.h    bar.o    bar.pro
.De
There is yet another syntax to modify substituted parameters.  You can
add certain modifiers in parentheses after the opening brace like:
.Ds
${(\fImodifiers\fC)\fIparameter\fC}
.De
For example, \fCo\fP sorts the words resulting from the expansion:
.Ds
% echo ${path}
/usr/bin /usr/bin/X11 /etc
% echo ${(o)path}
/etc /usr/bin /usr/bin/X11
.De
One possible source of confusion is the fact that in \fBzsh\fP,
the result of parameter substitution is \fInot\fP split into
words.  Thus, this will not work:
.Ds
% srcs='glob.c exec.c init.c'
% ls $srcs
glob.c exec.c init.c not found
.De
This is considered a feature, not a bug.
If splitting were done by default, as it is in most other shells,
functions like this would not work properly:
.Ds
$ ll () { ls -F $* }
$ ll 'fuu bar'
fuu not found
bar not found

% ll 'fuu bar'
fuu bar not found
.De
Of course, a hackish workaround is available in sh (and \fBzsh\fP):
.Ds
% setopt shwordsplit
% ll () { ls -F "$@" }
% ll 'fuu bar'
fuu bar not found
.De
If you like the sh behaviour, \fBzsh\fP can accomodate you:
.Ds
% ls ${=srcs}
exec.c  glob.c  init.c
% setopt shwordsplit
% ls $srcs
exec.c  glob.c  init.c
.De
Another way to get the \fC$srcs\fP trick to work is to use an array:
.Ds
% unset srcs
% srcs=( glob.c exec.c init.c )  
% ls $srcs
exec.c  glob.c  init.c
.De
or an alias:
.Ds
% alias -g SRCS='exec.c glob.c init.c'
% ls SRCS
exec.c  glob.c  init.c
.De
Another option that modifies parameter expansion is
\fIRCEXPANDPARAM\fP:
.Ds
% echo foo/$srcs
foo/glob.c exec.c init.c
% setopt rcexpandparam
% echo foo/$srcs
foo/glob.c foo/exec.c foo/init.c
% echo foo/${^srcs}
foo/glob.c foo/exec.c foo/init.c
% echo foo/$^srcs
foo/glob.c foo/exec.c foo/init.c
.De
.Sh "Shell Parameters"
.PP
The shell has many predefined parameters that may be
accessed.  Here are some examples:
.Ds
% sleep 10 &
[1] 3820
% echo $!
3820
% set a b c
% echo $#
3
% echo $ARGC
3
% ( exit 20 ) ; echo $?
20
% false; echo $status
1
.De
(\fC$?\fP and \fC$status\fP are equivalent.)
.Ds
% echo $HOST $HOSTTYPE
dendrite sun4
% echo $UID $GID
701 60
% cd /tmp
% cd /home
% echo $PWD $OLDPWD
/home /tmp
% ls $OLDPWD/.getwd 
/tmp/.getwd
.De
\fC~+\fP and \fC~-\fP are short for \fC$PWD\fP and \fC$OLDPWD\fP, respectively.
.Ds
% ls ~-/.getwd
/tmp/.getwd
% ls -d ~+/learning
/home/learning
% echo $RANDOM
4880
% echo $RANDOM
11785
% echo $RANDOM
2062
% echo $TTY
/dev/ttyp4
% echo $VERSION
zsh v2.00.03
% echo $USERNAME
pf
.De
.PP
The \fCcdpath\fP variable sets the search path for the \fCcd\fP command.
If you do not specify \fC.\fP somewhere in the path, it is assumed to
be the first component.
.Ds
% cdpath=( /usr ~ ~/zsh )
% ls /usr
5bin         dict         lang         net          sccs         sys
5include     etc          lector       nserve       services     tmp
5lib         export       lib          oed          share        ucb
adm          games        local        old          skel         ucbinclude
bin          geac         lost+found   openwin      spool        ucblib
boot         hosts        macsyma_417  pat          src          xpg2bin
demo         include      man          princeton    stand        xpg2include
diag         kvm          mdec         pub          swap         xpg2lib
% cd spool
/usr/spool
% cd bin
/usr/bin
% cd func
~/func
% cd 
% cd pub
% pwd
/u/pfalstad/pub
% ls -d /usr/pub
/usr/pub
.De
\fBPATH\fP and \fBpath\fP both set the search path for commands.
These two variables are equivalent, except that one is a string
and one is an array.  If the user modifies \fBPATH\fP, the shell
changes \fBpath\fP as well, and vice versa.
.Ds
% PATH=/bin:/usr/bin:/tmp:.
% echo $path
/bin /usr/bin /tmp .
% path=( /usr/bin . /usr/local/bin /usr/ucb )
% echo $PATH
/usr/bin:.:/usr/local/bin:/usr/ucb
.De
The same is true of \fBCDPATH\fP and \fBcdpath\fP:
.Ds
% echo $CDPATH
/usr:/u/pfalstad:/u/pfalstad/zsh
% CDPATH=/u/subbarao:/usr/src:/tmp
% echo $cdpath
/u/subbarao /usr/src /tmp
.De
In general, predefined parameters with names in all lowercase are
arrays; assignments to them take the form:
.Ds
\fIname\fR\fC=(\fR \fIelem\fR ...\ \fC)\fR
.De
Predefined parameters with names in all uppercase are strings.  If
there is both an array and a string version of the same parameter, the
string version is a colon-separated list, like \fBPATH\fP.
.PP
\fBHISTFILE\fP is the name of the history file, where the history
is saved when a shell exits.
.Ds
% zsh
phoenix% HISTFILE=/tmp/history
phoenix% SAVEHIST=20
phoenix% echo foo
foo
phoenix% date
Fri May 24 05:39:35 EDT 1991
phoenix% uptime
  5:39am  up 4 days, 20:02,  40 users,  load average: 2.30, 2.20, 2.00
phoenix% exit
% cat /tmp/history
HISTFILE=/tmp/history
SAVEHIST=20
echo foo
date
uptime
exit
% HISTSIZE=3
% history
   28  rm /tmp/history
   29  HISTSIZE=3
   30  history
.De
If you have several incantations of \fBzsh\fP running at the same
time, like when using the X window system, it might be preferable to
append the history of each shell to a file when a shell exits instead
of overwriting the old contents of the file.  You can get this
behaviour by setting the \fIAPPENDHISTORY\fP option.
.PP
In \fBzsh\fP, if you say
.Ds
% >file
.De
the command \fCcat\fP is normally assumed:
.Ds
% >file
foo!    
^D
% cat file
foo!
.De
Thus, you can view a file simply by typing:
.Ds
% <file
foo!
.De
However, this is not csh or sh compatible.  To correct this,
change the value of the parameter \fBNULLCMD\fP,
which is \fCcat\fP by default.
.Ds
% NULLCMD=:
% >file
% ls -l file
-rw-r--r--  1 pfalstad        0 May 24 05:41 file
.De
If \fCNULLCMD\fP is unset, the shell reports an error if no
command is specified (like csh).
.Ds
% unset NULLCMD
% >file
zsh: redirection with no command
.De
Actually, \fBREADNULLCMD\fP is used whenever you have a null command
reading input from a single file.  Thus, you can set \fBREADNULLCMD\fP
to \fCmore\fP or \fCless\fP rather than \fCcat\fP.  Also, if you
set \fBNULLCMD\fP to \fC:\fP for sh compatibility, you can still read
files with \fC< file\fP if you leave \fBREADNULLCMD\fP set to \fCmore\fP.
.Sh "Prompting"
.PP
The default prompt for \fBzsh\fP is:
.Ds
phoenix% echo $PROMPT
%m%# 
.De
The \fC%m\fP stands for the short form of the current hostname,
and the \fC%#\fP stands for a \fC%\fP or a \fC#\fP, depending on whether
the shell is running as root or not.
\fBzsh\fP supports many other control sequences
in the \fBPROMPT\fP variable.
.Ds
% PROMPT='%/> '
/u/pfalstad/etc/TeX/zsh>

% PROMPT='%~> '   
~/etc/TeX/zsh> 

% PROMPT='%h %~> '
6 ~/etc/TeX/zsh> 
.De
\fC%h\fP represents the number of current history event.
.Ds
% PROMPT='%h %~ %M> '
10 ~/etc/TeX/zsh apple-gunkies.gnu.ai.mit.edu> 

% PROMPT='%h %~ %m> '
11 ~/etc/TeX/zsh apple-gunkies> 

% PROMPT='%h %t> '
12 6:11am> 

% PROMPT='%n %w tty%l>'
pfalstad Fri 24 ttyp0>
.De
\fBPROMPT2\fP is used in multiline commands, like for-loops.  The
\fC%_\fP escape sequence was made especially for this prompt.  It is
replaced by the kind of command that is being entered.
.Ds
% PROMPT2='%_> '
% for i in foo bar
for>

% echo 'hi
quote>
.De
Also available is the \fBRPROMPT\fP parameter.
If this is set, the shell puts a prompt on the \fIright\fP side
of the screen.
.Ds
% RPROMPT='%t'
%                                                      6:14am

% RPROMPT='%~'
%                                               ~/etc/TeX/zsh

% PROMPT='%l %T %m[%h] ' RPROMPT=' %~'
p0 6:15 phoenix[5]                              ~/etc/TeX/zsh
.De
These special escape sequences can also be used with the
\fC-P\fP option to \fCprint\fP:
.Ds
% print -P %h tty%l
15 ttyp1
.De
.PP
The \fBPOSTEDIT\fP parameter is printed whenever the editor exits.
This can be useful for termcap tricks.  To highlight the prompt
and command line while leaving command output unhighlighted, try this:
.Ds
% POSTEDIT=`echotc se`
% PROMPT='%S%% '
.De
.Sh "Login/logout watching"
.PP
You can specify login or logout events to monitor
by setting the \fBwatch\fP variable.
Normally, this is done by specifying a list of usernames.
.Ds
% watch=( pfalstad subbarao sukthnkr egsirer )
.De
The \fClog\fP command reports all people logged in
that you are watching for.
.Ds
% log
pfalstad has logged on p0 from mickey.
pfalstad has logged on p5 from mickey.
% \fR...\fC
subbarao has logged on p8 from phoenix.
% \fR...\fC
subbarao has logged off p8 from phoenix.
% \fR...\fC
sukthnkr has logged on p8 from dew.
% \fR...\fC
sukthnkr has logged off p8 from dew.
.De
If you specify hostnames with an \fC@\fP prepended,
the shell will watch for all users logging in from
the specified host.
.Ds
% watch=( @mickey @phoenix )
% log
djthongs has logged on q2 from phoenix.
pfalstad has logged on p0 from mickey.
pfalstad has logged on p5 from mickey.
.De
If you give a tty name with a \fC%\fP prepended, the shell
will watch for all users logging in on that tty.
.Ds
% watch=( %ttyp0 %console )
% log
root has logged on console from .
pfalstad has logged on p0 from mickey.
.De
The format of the reports may also be changed.
.Ds
% watch=( pfalstad gettes eps djthongs jcorr bdavis )
% log
jcorr has logged on tf from 128.112.176.3:0.
jcorr has logged on r0 from 128.112.176.3:0.
gettes has logged on p4 from yo:0.0.
djthongs has logged on pe from grumpy:0.0.
djthongs has logged on q2 from phoenix.
bdavis has logged on qd from BRUNO.
eps has logged on p3 from csx30:0.0.
pfalstad has logged on p0 from mickey.
pfalstad has logged on p5 from mickey.
% WATCHFMT='%n on tty%l from %M'
% log
jcorr on ttytf from 128.112.176.3:0.
jcorr on ttyr0 from 128.112.176.3:0.
gettes on ttyp4 from yo:0.0
djthongs on ttype from grumpy:0.0
djthongs on ttyq2 from phoenix.Princeto
bdavis on ttyqd from BRUNO.pppl.gov
eps on ttyp3 from csx30:0.0
pfalstad on ttyp0 from mickey.Princeton
pfalstad on ttyp5 from mickey.Princeton
% WATCHFMT='%n fm %m'
% log
jcorr fm 128.112.176.3:0
jcorr fm 128.112.176.3:0
gettes fm yo:0.0
djthongs fm grumpy:0.0
djthongs fm phoenix
bdavis fm BRUNO
eps fm csx30:0.0
pfalstad fm mickey
pfalstad fm mickey
% WATCHFMT='%n %a at %t %w.'
% log
jcorr logged on at 3:15pm Mon 20.
jcorr logged on at 3:16pm Wed 22.
gettes logged on at 6:54pm Wed 22.
djthongs logged on at 7:19am Thu 23.
djthongs logged on at 7:20am Thu 23.
bdavis logged on at 12:40pm Thu 23.
eps logged on at 4:19pm Thu 23.
pfalstad logged on at 3:39am Fri 24.
pfalstad logged on at 3:42am Fri 24.
.De
If you have a \fC.friends\fP file in your home directory,
a convenient way to make \fBzsh\fP watch for all your friends
is to do this:
.Ds
% watch=( $(< ~/.friends) )
% echo $watch
subbarao maruchck root sukthnkr \fR...
.De
If watch is set to \fCall\fP, then all users logging in or out
will be reported.
.Sh "Options"
.PP
Some options have already been mentioned; here are a few more:
.PP
Using the \fIAUTOCD\fP option, you can simply type the name
of a directory, and it will become the current directory.
.Ds
% cd /
% setopt autocd
% bin
% pwd
/bin
% ../etc
% pwd
/etc
.De
With \fICDABLEVARS\fP, if the argument to \fCcd\fP is the name of a
parameter whose value is a valid directory, it will become
the current directory.
.Ds
% setopt cdablevars
% foo=/tmp
% cd foo
/tmp
.De
\fICORRECT\fP turns on spelling correction for commands,
and the \fICORRECTALL\fP option turns on spelling correction
for all arguments.
.Ds
% setopt correct
% sl
zsh: correct `sl' to `ls' [nyae]? y
% setopt correctall
% ls x.v11r4
zsh: correct `x.v11r4' to `X.V11R4' [nyae]? n
/usr/princton/src/x.v11r4 not found
% ls /etc/paswd
zsh: correct to `/etc/paswd' to `/etc/passwd' [nyae]? y
/etc/passwd
.De
If you press \fCy\fP
when the shell asks you if you want to correct a word, it will
be corrected.  If you press \fCn\fP, it will be left alone.
Pressing \fCa\fP aborts the command, and pressing \fCe\fP brings the line
up for editing again, in case you agree the word is spelled wrong
but you don't like the correction.
.PP
Normally, a quoted expression may contain a newline:
.Ds
% echo '
> foo
> '

foo

%
.De
With \fICSHJUNKIEQUOTES\fP set, this is illegal, as it is
in csh.
.Ds
% setopt cshjunkiequotes
% ls 'foo
zsh: unmatched '
.De
\fIGLOBDOTS\fP lets files beginning with a \fC.\fP be matched without
explicitly specifying the dot.
.Ds
% ls -d *x*
Mailboxes
% setopt globdots
% ls -d *x*
\&.exrc         .pnewsexpert  .xserverrc
\&.mushexpert   .xinitrc      Mailboxes
.De
\fIHISTIGNOREDUPS\fP prevents the current line from being
saved in the history if it is the same as the previous one;
\fIHISTIGNORESPACE\fP prevents the current line from being
saved if it begins with a space.
.Ds
% PROMPT='%h> '
39> setopt histignoredups
40> echo foo
foo
41> echo foo
foo
41> echo foo
foo
41> echo bar
bar
42> setopt histignorespace
43>  echo foo
foo
43>  echo fubar
fubar
43>  echo fubar
fubar
.De
\fIIGNOREBRACES\fP turns off csh-style brace expansion.
.Ds
% echo x{y{z,a},{b,c}d}e
xyze xyae xbde xcde
% setopt ignorebraces
% echo x{y{z,a},{b,c}d}e
x{y{z,a},{b,c}d}e
.De
\fIIGNOREEOF\fP forces the user to type \fCexit\fP or \fClogout\fP,
instead of just pressing \fI^D\fP.
.Ds
% setopt ignoreeof
% ^D
zsh: use 'exit' to exit.
.De
\fIINTERACTIVECOMMENTS\fP turns on interactive comments;
comments begin with a \fC#\fP.
.Ds
% setopt interactivecomments
% date # this is a comment
Fri May 24 06:54:14 EDT 1991
.De
\fINOBEEP\fP makes sure the shell never beeps.
.PP
\fINOCLOBBER\fP prevents you from accidentally
overwriting an existing file.
.Ds
% setopt noclobber
% cat /dev/null >~/.zshrc
zsh: file exists: /u/pfalstad/.zshrc
.De
If you really do want to clobber a file, you can use the
\fC>!\fP operator.
To make things easier in this case, the \fC>\fP is stored in
the history list as a \fC>!\fP:
.Ds
% cat /dev/null >! ~/.zshrc
% cat /etc/motd > ~/.zshrc
zsh: file exists: /u/pfalstad/.zshrc
% !!
cat /etc/motd >! ~/.zshrc
% \fR...
.De
\fIRCQUOTES\fP lets you use a more elegant method for including
single quotes in a singly quoted string:
.Ds
% echo '"don'\e''t do that."'
"don't do that."
% echo '"don''t do that."'
"dont do that."
% setopt rcquotes
% echo '"don''t do that."'
"don't do that."
.De
Finally,
\fISUNKEYBOARDHACK\fP wins the award for the strangest option.
If a line ends with \fC`\fP, and there are an odd number of them
on the line, the shell will ignore the trailing \fC`\fP.  This
is provided for keyboards whose RETURN key is too small,
and too close to the \fC`\fP key.
.Ds
% setopt sunkeyboardhack
% date`
Fri May 24 06:55:38 EDT 1991
.De
.Sh "Closing Comments"
.PP
I (Bas de Bakker) would be happy to receive mail if anyone has any
tricks or ideas to add to this document, or if there are some points
that could be made clearer or covered more thoroughly.  Please notify
me of any errors in this document.
.bp
.PX
