
/*
 *
 * zle_bindings.c - commands and keymaps
 *
 * This file is part of zsh, the Z shell.
 *
 * This software is Copyright 1992 by Paul Falstad
 *
 * Permission is hereby granted to copy, reproduce, redistribute or otherwise
 * use this software as long as: there is no monetary profit gained
 * specifically from the use or reproduction of this software, it is not
 * sold, rented, traded or otherwise marketed, and this copyright notice is
 * included prominently in any copy made.
 *
 * The author make no claims as to the fitness or correctness of this software
 * for any use whatsoever, and it is provided as is. Any use of this software
 * is at the user's own risk.
 *
 */

#define ZLE
#include "zsh.h"

struct zlecmd zlecmds[] =
{
    {"accept-and-hold", acceptandhold, 0},
    {"accept-and-infer-next-history", acceptandinfernexthistory, 0},
    {"accept-and-menu-complete", acceptandmenucomplete, ZLE_MENUCMP},
    {"accept-line", acceptline, 0},
    {"accept-line-and-down-history", acceptlineanddownhistory, 0},
    {"backward-char", backwardchar, ZLE_MOVEMENT},
    {"backward-delete-char", backwarddeletechar, ZLE_DELETE},
    {"backward-delete-word", backwarddeleteword, ZLE_DELETE},
    {"backward-kill-line", backwardkillline, ZLE_KILL},
    {"backward-kill-word", backwardkillword, ZLE_KILL | ZLE_DELETE},
    {"backward-word", backwardword, ZLE_MOVEMENT},
    {"beginning-of-buffer-or-history", beginningofbufferorhistory, ZLE_MOVEMENT},
    {"beginning-of-history", beginningofhistory, 0},
    {"beginning-of-line", beginningofline, ZLE_MOVEMENT},
    {"beginning-of-line-hist", beginningoflinehist, ZLE_MOVEMENT},
    {"capitalize-word", capitalizeword, 0},
    {"clear-screen", clearscreen, 0},
    {"complete-word", completeword, ZLE_MENUCMP},
    {"copy-prev-word", copyprevword, 0},
    {"copy-region-as-kill", copyregionaskill, ZLE_KILL},
    {"delete-char", deletechar, ZLE_DELETE},
    {"delete-char-or-list", deletecharorlist, ZLE_MENUCMP},
    {"delete-word", deleteword, ZLE_DELETE},
    {"digit-argument", digitargument, ZLE_ARG},
    {"down-case-word", downcaseword, 0},
    {"down-history", downhistory, 0},
    {"down-line-or-history", downlineorhistory, ZLE_MOVEMENT | ZLE_LINEMOVE},
    {"end-of-buffer-or-history", endofbufferorhistory, ZLE_MOVEMENT},
    {"end-of-history", endofhistory, 0},
    {"end-of-line", endofline, ZLE_MOVEMENT},
    {"end-of-line-hist", endoflinehist, ZLE_MOVEMENT},
    {"exchange-point-and-mark", exchangepointandmark, ZLE_MOVEMENT},
    {"execute-last-named-cmd", (F) 0, 0},
    {"execute-named-cmd", (F) 0, 0},
    {"expand-history", expandhistory, 0},
    {"expand-or-complete", expandorcomplete, ZLE_MENUCMP},
    {"expand-word", expandword, 0},
    {"forward-char", forwardchar, ZLE_MOVEMENT},
    {"forward-word", forwardword, ZLE_MOVEMENT},
    {"get-line", getline, 0},
    {"gosmacs-transpose-chars", gosmacstransposechars, 0},
    {"history-incremental-search-backward", historyincrementalsearchbackward, 0},
    {"history-incremental-search-forward", historyincrementalsearchforward, 0},
    {"history-search-backward", historysearchbackward, ZLE_HISTSEARCH},
    {"history-search-forward", historysearchforward, ZLE_HISTSEARCH},
    {"infer-next-history", infernexthistory, 0},
    {"insert-last-word", insertlastword, ZLE_INSERT},
    {"kill-buffer", killbuffer, ZLE_KILL},
    {"kill-line", killline, ZLE_KILL},
    {"kill-region", killregion, ZLE_KILL},
    {"kill-whole-line", killwholeline, ZLE_KILL},
    {"list-choices", listchoices, ZLE_DELETE | ZLE_MENUCMP},	/* ZLE_DELETE fixes autoremoveslash */
    {"list-expand", listexpand, ZLE_MENUCMP},
    {"magic-space", magicspace, 0},
    {"menu-complete", menucompleteword, ZLE_MENUCMP},
    {"menu-expand-or-complete", menuexpandorcomplete, ZLE_MENUCMP},
    {"overwrite-mode", overwritemode, 0},
    {"push-line", pushline, 0},
    {"quoted-insert", quotedinsert, ZLE_INSERT},
    {"quote-line", quoteline, 0},
    {"quote-region", quoteregion, 0},
    {"redisplay", redisplay, 0},
    {"reverse-menu-complete", reversemenucomplete, ZLE_MENUCMP},
    {"run-help", processcmd, 0},
    {"self-insert", selfinsert, ZLE_INSERT},
    {"self-insert-unmeta", selfinsertunmeta, ZLE_INSERT},
    {"send-break", sendbreak, 0},
    {"send-string", sendstring, 0},
    {"prefix", (F) 0, 0},
    {"set-mark-command", setmarkcommand, 0},
    {"spell-word", spellword, 0},
    {"toggle-literal-history", toggleliteralhistory, 0},
    {"transpose-chars", transposechars, 0},
    {"transpose-words", transposewords, 0},
    {"undefined-key", undefinedkey, 0},
    {"undo", undo, ZLE_UNDO},
    {"universal-argument", universalargument, ZLE_ARG},
    {"up-case-word", upcaseword, 0},
    {"up-history", uphistory, 0},
    {"up-line-or-history", uplineorhistory, ZLE_LINEMOVE | ZLE_MOVEMENT},
    {"vi-add-eol", viaddeol, 0},
    {"vi-add-next", viaddnext, 0},
    {"vi-backward-blank-word", vibackwardblankword, ZLE_MOVEMENT},
    {"vi-backward-char", vibackwardchar, ZLE_MOVEMENT},
    {"vi-backward-delete-char", vibackwarddeletechar, ZLE_DELETE},
    {"vi-beginning-of-line", vibeginningofline, ZLE_MOVEMENT},
    {"vi-caps-lock-panic", vicapslockpanic, 0},
    {"vi-change", vichange, 0},
    {"vi-change-eol", vichangeeol, 0},
    {"vi-change-whole-line", vichangewholeline, 0},
    {"vi-cmd-mode", vicmdmode, 0},
    {"vi-delete", videlete, ZLE_KILL},
    {"vi-delete-char", videletechar, ZLE_DELETE},
    {"vi-digit-or-beginning-of-line", (F) 0, 0},
    {"vi-end-of-line", viendofline, ZLE_MOVEMENT},
    {"vi-fetch-history", vifetchhistory, 0},
    {"vi-find-next-char", vifindnextchar, ZLE_MOVEMENT},
    {"vi-find-next-char-skip", vifindnextcharskip, ZLE_MOVEMENT},
    {"vi-find-prev-char", vifindprevchar, ZLE_MOVEMENT},
    {"vi-find-prev-char-skip", vifindprevcharskip, ZLE_MOVEMENT},
    {"vi-first-non-blank", vifirstnonblank, ZLE_MOVEMENT},
    {"vi-forward-blank-word", viforwardblankword, ZLE_MOVEMENT},
    {"vi-forward-blank-word-end", viforwardblankwordend, ZLE_MOVEMENT},
    {"vi-forward-char", viforwardchar, ZLE_MOVEMENT},
    {"vi-forward-word-end", viforwardwordend, ZLE_MOVEMENT},
    {"vi-goto-column", vigotocolumn, ZLE_MOVEMENT},
    {"vi-goto-mark", vigotomark, ZLE_MOVEMENT},
    {"vi-goto-mark-line", vigotomarkline, ZLE_MOVEMENT},
    {"vi-history-search-backward", vihistorysearchbackward, 0},
    {"vi-history-search-forward", vihistorysearchforward, 0},
    {"vi-indent", viindent, 0},
    {"vi-insert", viinsert, 0},
    {"vi-insert-bol", viinsertbol, 0},
    {"vi-join", vijoin, 0},
    {"vi-match-bracket", vimatchbracket, ZLE_MOVEMENT},
    {"vi-open-line-above", viopenlineabove, 0},
    {"vi-open-line-below", viopenlinebelow, 0},
    {"vi-oper-swap-case", vioperswapcase, 0},
    {"vi-put-after", viputafter, ZLE_YANK},
    {"vi-repeat-change", virepeatchange, ZLE_ARG},
    {"vi-repeat-find", virepeatfind, ZLE_MOVEMENT},
    {"vi-repeat-search", virepeatsearch, ZLE_MOVEMENT},
    {"vi-replace", vireplace, 0},
    {"vi-replace-chars", vireplacechars, 0},
    {"vi-rev-repeat-find", virevrepeatfind, ZLE_MOVEMENT},
    {"vi-rev-repeat-search", virevrepeatsearch, ZLE_MOVEMENT},
    {"vi-set-buffer", visetbuffer, 0},
    {"vi-set-mark", visetmark, 0},
    {"vi-substitute", visubstitute, 0},
    {"vi-swap-case", viswapcase, 0},
    {"vi-undo-change", undo, 0},
    {"vi-unindent", viunindent, 0},
    {"vi-yank", viyank, 0},
    {"vi-yank-eol", viyankeol, 0},
    {"which-command", processcmd, 0},
    {"yank", yank, ZLE_YANK | ZLE_NAMEDBUFFER},
    {"yank-pop", yankpop, ZLE_YANK},
    {"emacs-backward-word", emacsbackwardword, ZLE_MOVEMENT},
    {"emacs-forward-word", emacsforwardword, ZLE_MOVEMENT},
    {"kill-word", killword, ZLE_KILL},
    {"vi-kill-line", vikillline, ZLE_KILL},
    {"vi-backward-kill-word", vibackwardkillword, ZLE_KILL},
    {"expand-cmd-path", expandcmdpath, 0},
    {"neg-argument", negargument, ZLE_NEGARG | ZLE_ARG},
    {"pound-insert", poundinsert, 0},
    {"vi-forward-word", viforwardword, ZLE_MOVEMENT},
    {"vi-backward-word", vibackwardword, ZLE_MOVEMENT},
    {"up-line-or-search", uplineorsearch, ZLE_MOVEMENT | ZLE_LINEMOVE | ZLE_HISTSEARCH},
    {"down-line-or-search", downlineorsearch, ZLE_MOVEMENT | ZLE_LINEMOVE | ZLE_HISTSEARCH},
    {"push-input", pushinput, 0},
    {"push-line-or-edit", pushpopinput, 0},
    {"history-beginning-search-backward", historybeginningsearchbackward, ZLE_HISTSEARCH},
    {"history-beginning-search-forward", historybeginningsearchforward, ZLE_HISTSEARCH},
    {"expand-or-complete-prefix", expandorcompleteprefix, ZLE_MENUCMP},
    {"", (F) 0, 0}
};

int emacsbind[256] =
{
    /* ^@ */ z_setmarkcommand,
    /* ^A */ z_beginningofline,
    /* ^B */ z_backwardchar,
    /* ^C */ z_undefinedkey,
    /* ^D */ z_deletecharorlist,
    /* ^E */ z_endofline,
    /* ^F */ z_forwardchar,
    /* ^G */ z_sendbreak,
    /* ^H */ z_backwarddeletechar,
    /* ^I */ z_expandorcomplete,
    /* ^J */ z_acceptline,
    /* ^K */ z_killline,
    /* ^L */ z_clearscreen,
    /* ^M */ z_acceptline,
    /* ^N */ z_downlineorhistory,
    /* ^O */ z_acceptlineanddownhistory,
    /* ^P */ z_uplineorhistory,
    /* ^Q */ z_pushline,
    /* ^R */ z_historyincrementalsearchbackward,
    /* ^S */ z_historyincrementalsearchforward,
    /* ^T */ z_transposechars,
    /* ^U */ z_killwholeline,
    /* ^V */ z_quotedinsert,
    /* ^W */ z_backwardkillword,
    /* ^X */ z_sequenceleadin,
    /* ^Y */ z_yank,
    /* ^Z */ z_undefinedkey,
    /* ^[ */ z_sequenceleadin,
    /* ^\ */ z_undefinedkey,
    /* ^] */ z_undefinedkey,
    /* ^^ */ z_undefinedkey,
    /* ^_ */ z_undo,
    /*   */ z_selfinsert,
    /* ! */ z_selfinsert,
    /* " */ z_selfinsert,
    /* # */ z_selfinsert,
    /* $ */ z_selfinsert,
    /* % */ z_selfinsert,
    /* & */ z_selfinsert,
    /* ' */ z_selfinsert,
    /* ( */ z_selfinsert,
    /* ) */ z_selfinsert,
    /* * */ z_selfinsert,
    /* + */ z_selfinsert,
    /* , */ z_selfinsert,
    /* - */ z_selfinsert,
    /* . */ z_selfinsert,
    /* / */ z_selfinsert,
    /* 0 */ z_selfinsert,
    /* 1 */ z_selfinsert,
    /* 2 */ z_selfinsert,
    /* 3 */ z_selfinsert,
    /* 4 */ z_selfinsert,
    /* 5 */ z_selfinsert,
    /* 6 */ z_selfinsert,
    /* 7 */ z_selfinsert,
    /* 8 */ z_selfinsert,
    /* 9 */ z_selfinsert,
    /* : */ z_selfinsert,
    /* ; */ z_selfinsert,
    /* < */ z_selfinsert,
    /* = */ z_selfinsert,
    /* > */ z_selfinsert,
    /* ? */ z_selfinsert,
    /* @ */ z_selfinsert,
    /* A */ z_selfinsert,
    /* B */ z_selfinsert,
    /* C */ z_selfinsert,
    /* D */ z_selfinsert,
    /* E */ z_selfinsert,
    /* F */ z_selfinsert,
    /* G */ z_selfinsert,
    /* H */ z_selfinsert,
    /* I */ z_selfinsert,
    /* J */ z_selfinsert,
    /* K */ z_selfinsert,
    /* L */ z_selfinsert,
    /* M */ z_selfinsert,
    /* N */ z_selfinsert,
    /* O */ z_selfinsert,
    /* P */ z_selfinsert,
    /* Q */ z_selfinsert,
    /* R */ z_selfinsert,
    /* S */ z_selfinsert,
    /* T */ z_selfinsert,
    /* U */ z_selfinsert,
    /* V */ z_selfinsert,
    /* W */ z_selfinsert,
    /* X */ z_selfinsert,
    /* Y */ z_selfinsert,
    /* Z */ z_selfinsert,
    /* [ */ z_selfinsert,
    /* \ */ z_selfinsert,
    /* ] */ z_selfinsert,
    /* ^ */ z_selfinsert,
    /* _ */ z_selfinsert,
    /* ` */ z_selfinsert,
    /* a */ z_selfinsert,
    /* b */ z_selfinsert,
    /* c */ z_selfinsert,
    /* d */ z_selfinsert,
    /* e */ z_selfinsert,
    /* f */ z_selfinsert,
    /* g */ z_selfinsert,
    /* h */ z_selfinsert,
    /* i */ z_selfinsert,
    /* j */ z_selfinsert,
    /* k */ z_selfinsert,
    /* l */ z_selfinsert,
    /* m */ z_selfinsert,
    /* n */ z_selfinsert,
    /* o */ z_selfinsert,
    /* p */ z_selfinsert,
    /* q */ z_selfinsert,
    /* r */ z_selfinsert,
    /* s */ z_selfinsert,
    /* t */ z_selfinsert,
    /* u */ z_selfinsert,
    /* v */ z_selfinsert,
    /* w */ z_selfinsert,
    /* x */ z_selfinsert,
    /* y */ z_selfinsert,
    /* z */ z_selfinsert,
    /* { */ z_selfinsert,
    /* | */ z_selfinsert,
    /* } */ z_selfinsert,
    /* ~ */ z_selfinsert,
    /* ^? */ z_backwarddeletechar,
    /* M-^@ */ z_undefinedkey,
    /* M-^A */ z_undefinedkey,
    /* M-^B */ z_undefinedkey,
    /* M-^C */ z_undefinedkey,
    /* M-^D */ z_listchoices,
    /* M-^E */ z_undefinedkey,
    /* M-^F */ z_undefinedkey,
    /* M-^G */ z_sendbreak,
    /* M-^H */ z_backwardkillword,
    /* M-^I */ z_selfinsertunmeta,
    /* M-^J */ z_selfinsertunmeta,
    /* M-^K */ z_undefinedkey,
    /* M-^L */ z_clearscreen,
    /* M-^M */ z_selfinsertunmeta,
    /* M-^N */ z_undefinedkey,
    /* M-^O */ z_undefinedkey,
    /* M-^P */ z_undefinedkey,
    /* M-^Q */ z_undefinedkey,
    /* M-^R */ z_undefinedkey,
    /* M-^S */ z_undefinedkey,
    /* M-^T */ z_undefinedkey,
    /* M-^U */ z_undefinedkey,
    /* M-^V */ z_undefinedkey,
    /* M-^W */ z_undefinedkey,
    /* M-^X */ z_undefinedkey,
    /* M-^Y */ z_undefinedkey,
    /* M-^Z */ z_undefinedkey,
    /* M-^[ */ z_undefinedkey,
    /* M-^\ */ z_undefinedkey,
    /* M-^] */ z_undefinedkey,
    /* M-^^ */ z_undefinedkey,
    /* M-^_ */ z_copyprevword,
    /* M-  */ z_expandhistory,
    /* M-! */ z_expandhistory,
    /* M-" */ z_quoteregion,
    /* M-# */ z_undefinedkey,
    /* M-$ */ z_spellword,
    /* M-% */ z_undefinedkey,
    /* M-& */ z_undefinedkey,
    /* M-' */ z_quoteline,
    /* M-( */ z_undefinedkey,
    /* M-) */ z_undefinedkey,
    /* M-* */ z_undefinedkey,
    /* M-+ */ z_undefinedkey,
    /* M-, */ z_undefinedkey,
    /* M-- */ z_negargument,
    /* M-. */ z_insertlastword,
    /* M-/ */ z_undefinedkey,
    /* M-0 */ z_digitargument,
    /* M-1 */ z_digitargument,
    /* M-2 */ z_digitargument,
    /* M-3 */ z_digitargument,
    /* M-4 */ z_digitargument,
    /* M-5 */ z_digitargument,
    /* M-6 */ z_digitargument,
    /* M-7 */ z_digitargument,
    /* M-8 */ z_digitargument,
    /* M-9 */ z_digitargument,
    /* M-: */ z_undefinedkey,
    /* M-; */ z_undefinedkey,
    /* M-< */ z_beginningofbufferorhistory,
    /* M-= */ z_undefinedkey,
    /* M-> */ z_endofbufferorhistory,
    /* M-? */ z_whichcommand,
    /* M-@ */ z_undefinedkey,
    /* M-A */ z_acceptandhold,
    /* M-B */ z_backwardword,
    /* M-C */ z_capitalizeword,
    /* M-D */ z_killword,
    /* M-E */ z_undefinedkey,
    /* M-F */ z_forwardword,
    /* M-G */ z_getline,
    /* M-H */ z_runhelp,
    /* M-I */ z_undefinedkey,
    /* M-J */ z_undefinedkey,
    /* M-K */ z_undefinedkey,
    /* M-L */ z_downcaseword,
    /* M-M */ z_undefinedkey,
    /* M-N */ z_historysearchforward,
    /* M-O */ z_undefinedkey,
    /* M-P */ z_historysearchbackward,
    /* M-Q */ z_pushline,
    /* M-R */ z_toggleliteralhistory,
    /* M-S */ z_spellword,
    /* M-T */ z_transposewords,
    /* M-U */ z_upcaseword,
    /* M-V */ z_undefinedkey,
    /* M-W */ z_copyregionaskill,
    /* M-X */ z_undefinedkey,
    /* M-Y */ z_undefinedkey,
    /* M-Z */ z_undefinedkey,
    /* M-[ */ z_undefinedkey,
    /* M-\ */ z_undefinedkey,
    /* M-] */ z_undefinedkey,
    /* M-^ */ z_undefinedkey,
    /* M-_ */ z_insertlastword,
    /* M-` */ z_undefinedkey,
    /* M-a */ z_acceptandhold,
    /* M-b */ z_backwardword,
    /* M-c */ z_capitalizeword,
    /* M-d */ z_killword,
    /* M-e */ z_undefinedkey,
    /* M-f */ z_forwardword,
    /* M-g */ z_getline,
    /* M-h */ z_runhelp,
    /* M-i */ z_undefinedkey,
    /* M-j */ z_undefinedkey,
    /* M-k */ z_undefinedkey,
    /* M-l */ z_downcaseword,
    /* M-m */ z_undefinedkey,
    /* M-n */ z_historysearchforward,
    /* M-o */ z_undefinedkey,
    /* M-p */ z_historysearchbackward,
    /* M-q */ z_pushline,
    /* M-r */ z_toggleliteralhistory,
    /* M-s */ z_spellword,
    /* M-t */ z_transposewords,
    /* M-u */ z_upcaseword,
    /* M-v */ z_undefinedkey,
    /* M-w */ z_copyregionaskill,
    /* M-x */ z_executenamedcmd,
    /* M-y */ z_yankpop,
    /* M-z */ z_executelastnamedcmd,
    /* M-{ */ z_undefinedkey,
    /* M-| */ z_vigotocolumn,
    /* M-} */ z_undefinedkey,
    /* M-~ */ z_undefinedkey,
    /* M-^? */ z_backwardkillword,
};

int viinsbind[32] =
{
    /* ^@ */ z_undefinedkey,
    /* ^A */ z_selfinsert,
    /* ^B */ z_selfinsert,
    /* ^C */ z_undefinedkey,
    /* ^D */ z_listchoices,
    /* ^E */ z_selfinsert,
    /* ^F */ z_selfinsert,
    /* ^G */ z_selfinsert,
    /* ^H */ z_vibackwarddeletechar,
    /* ^I */ z_expandorcomplete,
    /* ^J */ z_acceptline,
    /* ^K */ z_killline,
    /* ^L */ z_clearscreen,
    /* ^M */ z_acceptline,
    /* ^N */ z_selfinsert,
    /* ^O */ z_selfinsert,
    /* ^P */ z_selfinsert,
    /* ^Q */ z_selfinsert,
    /* ^R */ z_redisplay,
    /* ^S */ z_selfinsert,
    /* ^T */ z_selfinsert,
    /* ^U */ z_vikillline,
    /* ^V */ z_quotedinsert,
    /* ^W */ z_vibackwardkillword,
    /* ^X */ z_selfinsert,
    /* ^Y */ z_selfinsert,
    /* ^Z */ z_selfinsert,
    /* ^[ */ z_sequenceleadin,
    /* ^\ */ z_selfinsert,
    /* ^] */ z_selfinsert,
    /* ^^ */ z_selfinsert,
    /* ^_ */ z_selfinsert,
};

int vicmdbind[128] =
{
    /* ^@ */ z_undefinedkey,
    /* ^A */ z_beginningofline,
    /* ^B */ z_undefinedkey,
    /* ^C */ z_undefinedkey,
    /* ^D */ z_listchoices,
    /* ^E */ z_endofline,
    /* ^F */ z_undefinedkey,
    /* ^G */ z_listexpand,
    /* ^H */ z_backwarddeletechar,
    /* ^I */ z_completeword,
    /* ^J */ z_acceptline,
    /* ^K */ z_killline,
    /* ^L */ z_clearscreen,
    /* ^M */ z_acceptline,
    /* ^N */ z_downhistory,
    /* ^O */ z_undefinedkey,
    /* ^P */ z_uphistory,
    /* ^Q */ z_undefinedkey,
    /* ^R */ z_redisplay,
    /* ^S */ z_undefinedkey,
    /* ^T */ z_undefinedkey,
    /* ^U */ z_killbuffer,
    /* ^V */ z_undefinedkey,
    /* ^W */ z_backwardkillword,
    /* ^X */ z_expandorcomplete,
    /* ^Y */ z_undefinedkey,
    /* ^Z */ z_undefinedkey,
    /* ^[ */ z_sequenceleadin,
    /* ^\ */ z_undefinedkey,
    /* ^] */ z_undefinedkey,
    /* ^^ */ z_undefinedkey,
    /* ^_ */ z_undefinedkey,
    /*   */ z_viforwardchar,
    /* ! */ z_undefinedkey,
    /* " */ z_visetbuffer,
    /* # */ z_poundinsert,
    /* $ */ z_viendofline,
    /* % */ z_vimatchbracket,
    /* & */ z_undefinedkey,
    /* ' */ z_vigotomarkline,
    /* ( */ z_undefinedkey,
    /* ) */ z_undefinedkey,
    /* * */ z_undefinedkey,
    /* + */ z_downlineorhistory,
    /* , */ z_virevrepeatfind,
    /* - */ z_uplineorhistory,
    /* . */ z_virepeatchange,
    /* / */ z_vihistorysearchbackward,
    /* 0 */ z_vidigitorbeginningofline,
    /* 1 */ z_digitargument,
    /* 2 */ z_digitargument,
    /* 3 */ z_digitargument,
    /* 4 */ z_digitargument,
    /* 5 */ z_digitargument,
    /* 6 */ z_digitargument,
    /* 7 */ z_digitargument,
    /* 8 */ z_digitargument,
    /* 9 */ z_digitargument,
    /* : */ z_undefinedkey,
    /* ; */ z_virepeatfind,
    /* < */ z_viunindent,
    /* = */ z_listchoices,
    /* > */ z_viindent,
    /* ? */ z_vihistorysearchforward,
    /* @ */ z_undefinedkey,
    /* A */ z_viaddeol,
    /* B */ z_vibackwardblankword,
    /* C */ z_vichangeeol,
    /* D */ z_killline,
    /* E */ z_viforwardblankwordend,
    /* F */ z_vifindprevchar,
    /* G */ z_vifetchhistory,
    /* H */ z_vicapslockpanic,
    /* I */ z_viinsertbol,
    /* J */ z_historysearchforward,
    /* K */ z_historysearchbackward,
    /* L */ z_undefinedkey,
    /* M */ z_undefinedkey,
    /* N */ z_virevrepeatsearch,
    /* O */ z_viopenlineabove,
    /* P */ z_yank,
    /* Q */ z_undefinedkey,
    /* R */ z_vireplace,
    /* S */ z_vichangewholeline,
    /* T */ z_vifindprevcharskip,
    /* U */ z_undefinedkey,
    /* V */ z_undefinedkey,
    /* W */ z_viforwardblankword,
    /* X */ z_vibackwarddeletechar,
    /* Y */ z_viyankeol,
    /* Z */ z_undefinedkey,
    /* [ */ z_undefinedkey,
    /* \ */ z_completeword,
    /* ] */ z_undefinedkey,
    /* ^ */ z_vifirstnonblank,
    /* _ */ z_undefinedkey,
    /* ` */ z_vigotomark,
    /* a */ z_viaddnext,
    /* b */ z_vibackwardword,
    /* c */ z_vichange,
    /* d */ z_videlete,
    /* e */ z_viforwardwordend,
    /* f */ z_vifindnextchar,
    /* g */ z_undefinedkey,
    /* h */ z_vibackwardchar,
    /* i */ z_viinsert,
    /* j */ z_downlineorhistory,
    /* k */ z_uplineorhistory,
    /* l */ z_viforwardchar,
    /* m */ z_visetmark,
    /* n */ z_virepeatsearch,
    /* o */ z_viopenlinebelow,
    /* p */ z_viputafter,
    /* q */ z_undefinedkey,
    /* r */ z_vireplacechars,
    /* s */ z_visubstitute,
    /* t */ z_vifindnextcharskip,
    /* u */ z_viundochange,
    /* v */ z_undefinedkey,
    /* w */ z_viforwardword,
    /* x */ z_videletechar,
    /* y */ z_viyank,
    /* z */ z_undefinedkey,
    /* { */ z_undefinedkey,
    /* | */ z_vigotocolumn,
    /* } */ z_undefinedkey,
    /* ~ */ z_viswapcase,
    /* ^? */ z_backwarddeletechar,
};
