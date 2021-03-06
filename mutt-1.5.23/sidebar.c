/*
 * Copyright (C) ????-2004 Justin Hibbits <jrh29@po.cwru.edu>
 * Copyright (C) 2004 Thomer M. Gil <mutt@thomer.com>
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mutt.h"
#include "mutt_menu.h"
#include "mutt_curses.h"
#include "sidebar.h"
#include "buffy.h"
#include <libgen.h>
#include "keymap.h"
#include <stdbool.h>

/*BUFFY *CurBuffy = 0;*/
static BUFFY *TopBuffy = 0;
static BUFFY *BottomBuffy = 0;
static int known_lines = 0;

#define SCRATCH_BUFSZ	(512)
char sidebar_scratch[SCRATCH_BUFSZ];

char *sidebar_get_jump_mailbox_name(int op, char *base_box) {
	int box_ix;

	switch(op) {
	case OP_SIDEBAR_JUMP_MAILBOX1:	box_ix = 0; break; 
	case OP_SIDEBAR_JUMP_MAILBOX2:	box_ix = 1; break; 
	case OP_SIDEBAR_JUMP_MAILBOX3:	box_ix = 2; break; 
	}
	if((snprintf(sidebar_scratch, SCRATCH_BUFSZ, "%s%s", base_box, SidebarJMailbox[box_ix])) >= SCRATCH_BUFSZ)
		return NULL;
	return &sidebar_scratch[0];
}

static int quick_log10(int x) {
    // this is either a fun exercise in optimization 
    // or it's extremely premature optimization.
	// !But better than what was used before! 
    if(x>=100000) {
        if(x>=10000000) {
            if(x>=1000000000) return 10;
            if(x>=100000000) return 9;
            return 8;
        }
        if(x>=1000000) return 7;
        return 6;
    } else {
        if(x>=1000) {
            if(x>=10000) return 5;
            return 4;
        } else {
            if(x>=100) return 3;
            if(x>=10) return 2;
            return 1;
        }
    }
}

void calc_boundaries (int menu)
{
        BUFFY *tmp = Incoming;

        if ( known_lines != LINES ) {
                TopBuffy = BottomBuffy = 0;
                known_lines = LINES;
        }
        for ( ; tmp->next != 0; tmp = tmp->next )
                tmp->next->prev = tmp;

        if ( TopBuffy == 0 && BottomBuffy == 0 )
                TopBuffy = Incoming;
        if ( BottomBuffy == 0 ) {
                int count = LINES - 2 - (menu != MENU_PAGER || option(OPTSTATUSONTOP));
                BottomBuffy = TopBuffy;
                while ( --count && BottomBuffy->next )
                        BottomBuffy = BottomBuffy->next;
        }
        else if ( TopBuffy == CurBuffy->next ) {
                int count = LINES - 2 - (menu != MENU_PAGER);
                BottomBuffy = CurBuffy;
                tmp = BottomBuffy;
                while ( --count && tmp->prev)
                        tmp = tmp->prev;
                TopBuffy = tmp;
        }
        else if ( BottomBuffy == CurBuffy->prev ) {
                int count = LINES - 2 - (menu != MENU_PAGER);
                TopBuffy = CurBuffy;
                tmp = TopBuffy;
                while ( --count && tmp->next )
                        tmp = tmp->next;
                BottomBuffy = tmp;
        }
}


char *make_sidebar_entry(char *box, int size, int new, int flagged)
{
        static char *entry = 0;
        char *c;
        int i = 0;
        int delim_len = strlen(SidebarDelim);

        c = realloc(entry, SidebarWidth - delim_len + 2);
        if ( c ) entry = c;
        entry[SidebarWidth - delim_len + 1] = 0;
        for (; i < SidebarWidth - delim_len + 1; entry[i++] = ' ' );
        i = strlen(box);
        strncpy( entry, box, i < (SidebarWidth - delim_len + 1) ? i : (SidebarWidth - delim_len + 1) );

        if (size == -1)
                sprintf(entry + SidebarWidth - delim_len - 3, "?");
        else if ( new ) {
                if (flagged > 0) {
                        sprintf(
                                entry + SidebarWidth - delim_len - 5 - quick_log10(size) - quick_log10(new) - quick_log10(flagged),
                                "% d(%d)[%d]", size, new, flagged);
                }
                else {
                        sprintf(
                                entry + SidebarWidth - delim_len - 3 - quick_log10(size) - quick_log10(new),
                                "% d(%d)", size, new);
                }
        }
        else if (flagged > 0) {
                sprintf( entry + SidebarWidth - delim_len - 3 - quick_log10(size) - quick_log10(flagged), "% d[%d]", size, flagged);
        }
        else {
                sprintf( entry + SidebarWidth - delim_len - 1 - quick_log10(size), "% d", size);
        }
        return entry;
}


void set_curbuffy(char buf[LONG_STRING])
{
        BUFFY* tmp = CurBuffy = Incoming;

        if (!Incoming)
                return;

        while(1) {
                if(!strcmp(tmp->path, buf)) {
                        CurBuffy = tmp;
                        break;
                }

                if(tmp->next)
                        tmp = tmp->next;
                else
                        break;
        }
}


int draw_sidebar(int menu)
{

        int lines = option(OPTHELP) ? 1 : 0;
        BUFFY *tmp;
#ifndef USE_SLANG_CURSES
        attr_t attrs;
#endif
        short delim_len = strlen(SidebarDelim);
        short color_pair;

        static bool initialized = false;
        static int prev_show_value;
        static short saveSidebarWidth;

/* initialize first time */
        if(!initialized) {
                prev_show_value = option(OPTSIDEBAR);
                saveSidebarWidth = SidebarWidth;
                if(!option(OPTSIDEBAR)) SidebarWidth = 0;
                initialized = true;
        }

/* save or restore the value SidebarWidth */
        if(prev_show_value != option(OPTSIDEBAR)) {
                if(prev_show_value && !option(OPTSIDEBAR)) {
                        saveSidebarWidth = SidebarWidth;
                        SidebarWidth = 0;
                }
                else if(!prev_show_value && option(OPTSIDEBAR)) {
                        SidebarWidth = saveSidebarWidth;
                }
                prev_show_value = option(OPTSIDEBAR);
        }

//	if ( SidebarWidth == 0 ) return 0;
        if (SidebarWidth > 0 && option (OPTSIDEBAR)
        && delim_len >= SidebarWidth) {
                unset_option (OPTSIDEBAR);
/* saveSidebarWidth = SidebarWidth; */
                if (saveSidebarWidth > delim_len) {
                        SidebarWidth = saveSidebarWidth;
                        mutt_error (_("Value for sidebar_delim is too long. Disabling sidebar."));
                        sleep (2);
                }
                else {
                        SidebarWidth = 0;
                        mutt_error (_("Value for sidebar_delim is too long. Disabling sidebar. Please set your sidebar_width to a sane value."));
                        sleep (4);                /* the advise to set a sane value should be seen long enough */
                }
                saveSidebarWidth = 0;
                return (0);
        }

        if ( SidebarWidth == 0 || !option(OPTSIDEBAR)) {
                if (SidebarWidth > 0) {
                        saveSidebarWidth = SidebarWidth;
                        SidebarWidth = 0;
                }
                unset_option(OPTSIDEBAR);
                return 0;
        }

/* get attributes for divider */
        SETCOLOR(MT_COLOR_STATUS);
#ifndef USE_SLANG_CURSES
        attr_get(&attrs, &color_pair, 0);
#else
        color_pair = attr_get();
#endif
        SETCOLOR(MT_COLOR_NORMAL);

/* draw the divider */

        for ( ; lines < LINES-1-(menu != MENU_PAGER || option(OPTSTATUSONTOP)); lines++ ) {
                move(lines, SidebarWidth - delim_len);
                addstr(NONULL(SidebarDelim));
#ifndef USE_SLANG_CURSES
                mvchgat(lines, SidebarWidth - delim_len, delim_len, 0, color_pair, NULL);
#endif
        }

        if ( Incoming == 0 ) return 0;
        lines = option(OPTHELP) ? 1 : 0;          /* go back to the top */

        if ( known_lines != LINES || TopBuffy == 0 || BottomBuffy == 0 )
                calc_boundaries(menu);
        if ( CurBuffy == 0 ) CurBuffy = Incoming;

        tmp = TopBuffy;

        SETCOLOR(MT_COLOR_NORMAL);

        for ( ; tmp && lines < LINES-1 - (menu != MENU_PAGER || option(OPTSTATUSONTOP)); tmp = tmp->next ) {
                if (tmp == CurBuffy)
                        SETCOLOR(MT_COLOR_SB_INDICATOR);
				else if (strcmp(tmp->path, Spoolfile) == 0) 
						SETCOLOR(MT_COLOR_SB_SPOOLFILE);
                else if ( tmp->msg_unread > 0 )
                        SETCOLOR(MT_COLOR_NEW);
                else if ( tmp->msg_flagged > 0 )
                        SETCOLOR(MT_COLOR_FLAGGED);
                else
                        SETCOLOR(MT_COLOR_NORMAL);

                move( lines, 0 );
                if ( Context && !strcmp( tmp->path, Context->path ) ) {
                        tmp->msg_unread = Context->unread;
                        tmp->msgcount = Context->msgcount;
                        tmp->msg_flagged = Context->flagged;
                }
// check whether Maildir is a prefix of the current folder's path
                short maildir_is_prefix = 0;
                if ( (strlen(tmp->path) > strlen(Maildir)) &&
                        (strncmp(Maildir, tmp->path, strlen(Maildir)) == 0) )
                        maildir_is_prefix = 1;
// calculate depth of current folder and generate its display name with indented spaces
                int sidebar_folder_depth = 0;
                char *sidebar_folder_name;
                sidebar_folder_name = basename(tmp->path);
                if ( maildir_is_prefix ) {
                        char *tmp_folder_name;
                        int i;
                        tmp_folder_name = tmp->path + strlen(Maildir);
                        for (i = 0; i < strlen(tmp->path) - strlen(Maildir); i++) {
                                if (tmp_folder_name[i] == '/') sidebar_folder_depth++;
                        }
                        if (sidebar_folder_depth > 0) {
                                sidebar_folder_name = malloc(strlen(basename(tmp->path)) + sidebar_folder_depth + 1);
                                for (i=0; i < sidebar_folder_depth; i++)
                                        sidebar_folder_name[i]=' ';
                                sidebar_folder_name[i]=0;
                                strncat(sidebar_folder_name, basename(tmp->path), strlen(basename(tmp->path)) + sidebar_folder_depth);
                        }
                }
                printw( "%.*s", SidebarWidth - delim_len + 1,
                        make_sidebar_entry(sidebar_folder_name, tmp->msgcount,
                        tmp->msg_unread, tmp->msg_flagged));
                if (sidebar_folder_depth > 0)
                        free(sidebar_folder_name);
                lines++;
        }
        SETCOLOR(MT_COLOR_NORMAL);
        for ( ; lines < LINES-1 - (menu != MENU_PAGER || option(OPTSTATUSONTOP)); lines++ ) {
                int i = 0;
                move( lines, 0 );
                for ( ; i < SidebarWidth - delim_len; i++ )
                        addch(' ');
        }
        return 0;
}


void set_buffystats(CONTEXT* Context)
{
        BUFFY *tmp = Incoming;
        while(tmp) {
                if(Context && !strcmp(tmp->path, Context->path)) {
                        tmp->msg_unread = Context->unread;
                        tmp->msgcount = Context->msgcount;
                        break;
                }
                tmp = tmp->next;
        }
}


void scroll_sidebar(int op, int menu)
{
        if(!SidebarWidth) return;
        if(!CurBuffy) return;

        switch (op) {
                case OP_SIDEBAR_NEXT:
                        if ( CurBuffy->next == NULL ) return;
                        CurBuffy = CurBuffy->next;
                        break;
                case OP_SIDEBAR_PREV:
                        if ( CurBuffy->prev == NULL ) return;
                        CurBuffy = CurBuffy->prev;
                        break;
                case OP_SIDEBAR_SCROLL_UP:
                        CurBuffy = TopBuffy;
                        if ( CurBuffy != Incoming ) {
                                calc_boundaries(menu);
                                CurBuffy = CurBuffy->prev;
                        }
                        break;
                case OP_SIDEBAR_SCROLL_DOWN:
                        CurBuffy = BottomBuffy;
                        if ( CurBuffy->next ) {
                                calc_boundaries(menu);
                                CurBuffy = CurBuffy->next;
                        }
                        break;
                default:
                        return;
        }
        calc_boundaries(menu);
        draw_sidebar(menu);
}
