/*
Copyright (C) 2011 Jay Satiro <raysatiro@yahoo.com>
All rights reserved.

This file is part of GetHooks.

GetHooks is free software: you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

GetHooks is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with GetHooks.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
This file contains functions for comparing two snapshots for differences in hook information.
Each function is documented in the comment block above its definition.

-
match_gui_process_name()

Compare a GUI thread's process name to the passed in name.
-

-
match_gui_process_pid()

Compare a GUI thread's process id to the passed in process id.
-

-
is_hook_wanted()

Check the user-specified configuration to determine if the hook struct should be processed.
-

-
match_hook_process_pid()

Match a hook struct's associated GUI threads' process pids to the passed in pid.
-

-
match_hook_process_name()

Match a hook struct's associated GUI threads' process names to the passed in name.
-

-
print_diff_gui()

Compare two gui structs and print any significant differences.
-

-
print_hook_notice_begin()

Helper function to print a hook [begin] header with basic hook info.
-

-
print_hook_notice_end()

Helper function to print a hook [end] header.
-

-
print_diff_hook()

Compare two hook structs, both for the same HOOK object, and print any significant differences.
-

-
print_diff_desktop_hook_items()

Print the HOOKs that have been added/removed from a single attached to desktop between snapshots.
-

-
print_diff_desktop_hook_lists()

Print the HOOKs that have been added/removed from all attached to desktops between snapshots.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"

#include "diff.h"

/* the global stores */
#include "global.h"



/* match_gui_process_name()
Compare a GUI thread's process name to the passed in name.

returns nonzero on success ('name' matches the GUI thread's process name)
*/
int match_gui_process_name(
	const struct gui *const gui,   // in
	const WCHAR *const name   // in
)
{
	FAIL_IF( !gui );
	FAIL_IF( !name );
	
	
	if( gui->spi 
		&& gui->spi->ImageName.Buffer 
		&& !_wcsicmp( gui->spi->ImageName.Buffer, name )
	)
		return TRUE;
	else
		return FALSE;
}



/* match_gui_process_pid()
Compare a GUI thread's process id to the passed in process id.

returns nonzero on success ('pid' matches the GUI thread's process id)
*/
int match_gui_process_pid(
	const struct gui *const gui,   // in
	const int pid   // in
)
{
	FAIL_IF( !gui );
	FAIL_IF( !pid );
	
	
	if( gui->spi && ( pid == (int)( (DWORD)gui->spi->UniqueProcessId ) ) )
		return TRUE;
	else
		return FALSE;
}



/* is_hook_wanted()
Check the user-specified configuration to determine if the hook struct should be processed.

The user can filter hooks (eg WH_MOUSE) and programs (eg notepad.exe).

returns nonzero if the hook struct should be processed
*/
int is_hook_wanted( 
	const struct hook *const hook   // in
)
{
	FAIL_IF( !hook );
	
	
	/* if there is a list of programs to include/exclude */
	if( G->config->proglist->init_time 
		&& ( ( G->config->proglist->type == LIST_INCLUDE_PROG )
			|| ( G->config->proglist->type == LIST_EXCLUDE_PROG )
		)
	)
	{
		unsigned yes = 0;
		const struct list_item *item = NULL;
		
		
		for( item = G->config->proglist->head; ( item && !yes ); item = item->next )
		{
			if( item->name ) // match program name
				yes = !!match_hook_process_name( hook, item->name );
			else // match program id
				yes = !!match_hook_process_pid( hook, item->id );
		}
		
		if( ( yes && ( G->config->proglist->type == LIST_EXCLUDE_PROG ) )
			|| ( !yes && ( G->config->proglist->type == LIST_INCLUDE_PROG ) )
				return FALSE; // the hook is not wanted
	}
	
	
	/* if there is a list of hooks to include/exclude */
	if( G->config->hooklist->init_time 
		&& ( ( G->config->hooklist->type == LIST_INCLUDE_HOOK )
			|| ( G->config->hooklist->type == LIST_EXCLUDE_HOOK )
		)
	)
	{
		unsigned yes = 0;
		const struct list_item *item = NULL;
		
		
		for( item = G->config->hooklist->head; ( item && !yes ); item = item->next )
			yes = ( item->id == hook->object->iHook );
		
		if( ( yes && ( G->config->hooklist->type == LIST_EXCLUDE_HOOK ) )
			|| ( !yes && ( G->config->hooklist->type == LIST_INCLUDE_HOOK ) )
				return FALSE; // the hook is not wanted
	}
	
	return TRUE; // the hook is wanted
}



/* match_hook_process_pid()
Match a hook struct's associated GUI threads' process pids to the passed in pid.

returns nonzero on success ('pid' matched one of the hook struct's GUI thread process pids)
*/
int match_hook_process_pid(
	const struct hook *const hook,   // in
	const int pid   // in
)
{
	FAIL_IF( !hook );
	FAIL_IF( !name );
	
	
	if( ( hook->owner && match_gui_process_pid( hook->owner, pid ) )
		|| ( hook->origin && match_gui_process_pid( hook->origin, pid ) )
		|| ( hook->target && match_gui_process_pid( hook->target, pid ) )
	)
		return TRUE;
	else
		return FALSE;
}



/* match_hook_process_name()
Match a hook struct's associated GUI threads' process names to the passed in name.

returns nonzero on success ('name' matched one of the hook struct's GUI thread process names)
*/
int match_hook_process_name(
	const struct hook *const hook,   // in
	const WCHAR *const name   // in
)
{
	FAIL_IF( !hook );
	FAIL_IF( !name );
	
	
	if( ( hook->owner && match_gui_process_name( hook->owner, name ) )
		|| ( hook->origin && match_gui_process_name( hook->origin, name ) )
		|| ( hook->target && match_gui_process_name( hook->target, name ) )
	)
		return TRUE;
	else
		return FALSE;
}



/* print_diff_gui()
Compare two gui structs and print any significant differences.

'a' is the old gui thread info (optional)
'b' is the new gui thread info (optional)
'threadname' is the name of the gui thread as it applies to the HOOK. eg "target", "origin"

returns nonzero if there are significant differences (something is printed)
*/
int print_diff_gui(
	const struct gui *const a,   // in, optional
	const struct gui *const b,   // in, optional
	const char *const threadname   // in
)
{
	WCHAR empty1[] = L"<unknown>";
	WCHAR empty2[] = L"<unknown>";
	struct
	{
		void *pvWin32ThreadInfo;
		void *pvTeb;
		HANDLE tid;
		HANDLE pid;
		UNICODE_STRING ImageName;
	} oldstuff, newstuff;
	
	FAIL_IF( !threadname );
	
	
	ZeroMemory( &oldstuff, sizeof( oldstuff ) );
	ZeroMemory( &newstuff, sizeof( newstuff ) );
	
	oldstuff.ImageName.Buffer = empty1;
	oldstuff.ImageName.Length = wcslen( oldstuff.ImageName.Buffer ) * sizeof( WCHAR );
	oldstuff.ImageName.MaximumLength = oldstuff.ImageName.Length + sizeof( WCHAR );
	
	newstuff.ImageName.Buffer = empty2;
	newstuff.ImageName.Length = wcslen( newstuff.ImageName.Buffer ) * sizeof( WCHAR );
	newstuff.ImageName.MaximumLength = newstuff.ImageName.Length + sizeof( WCHAR );
	
	/* both oldstuff and newstuff have been initialized empty */
	
	if( a )
	{
		oldstuff.pvWin32ThreadInfo = a->pvWin32ThreadInfo;
		oldstuff.pvTeb = a->pvTeb;
		
		if( a->sti )
			oldstuff.tid = a->sti->ClientId.UniqueThread;
		
		if( a->spi )
		{
			oldstuff.pid = a->spi->UniqueProcessId;
			
			if( a->spi->ImageName.Buffer )
				oldstuff.ImageName = a->spi->ImageName;
		}
	}
	
	if( b )
	{
		newstuff.pvWin32ThreadInfo = b->pvWin32ThreadInfo;
		newstuff.pvTeb = b->pvTeb;
		
		if( b->sti )
			newstuff.tid = b->sti->ClientId.UniqueThread;
		
		if( b->spi )
		{
			newstuff.pid = b->spi->UniqueProcessId;
			
			if( b->spi->ImageName.Buffer )
				newstuff.ImageName = b->spi->ImageName;
		}
	}
	
	
	if( ( oldstuff.pvWin32ThreadInfo == newstuff.pvWin32ThreadInfo )
		&& ( oldstuff.pvTeb == newstuff.pvTeb )
		&& ( oldstuff.tid == newstuff.tid )
		&& ( oldstuff.pid == newstuff.pid )
		&& ( oldstuff.ImageName.Length == newstuff.ImageName.Length )
		&& !wcsncmp( 
			oldstuff.ImageName.Buffer, 
			newstuff.ImageName.Buffer, 
			oldstuff.ImageName.Length
		)
	)
		return FALSE;
	
	
	printf( "\nThe associated gui %s thread information has changed.\n", threadname );
	printf( "Old %s: ", threadname );
	print_gui_brief( a );
	printf( "\n" );
	
	printf( "New %s:", threadname );
	print_gui_brief( b );
	printf( "\n" );
	
	
	return TRUE;
}



/* print_hook_notice_begin()
Helper function to print a hook [begin] header with basic hook info.

'b' is the hook info
'deskname' is the desktop name
'difftype' is the reported action, eg HOOK_ADDED, HOOK_MODIFIED, HOOK_REMOVED
*/
static void print_hook_notice_begin(
	const struct hook *const b,   // in
	const WCHAR *const deskname,   // in
	const enum difftype difftype   // in
)
{
	const char *deskname = NULL;
	
	FAIL_IF( !b );
	FAIL_IF( !deskname );
	FAIL_IF( !difftype );
	
	
	PRINT_SEP_BEGIN( "" );
	
	if( difftype == HOOK_ADDED )
		diffname = "Added";
	else if( difftype == HOOK_REMOVED )
		diffname = "Removed";
	else if( difftype == HOOK_MODIFIED )
		diffname = "Modified";
	else
		FAIL_IF( TRUE );
	
	
	printf( "[%s HOOK ", diffname );
	PRINT_BARE_PTR( b->entry.pHead );
	printf( " on desktop %ls]\n", deskname );
	
	printf( "Name: " );
	print_HOOK_id( hook->object.iHook );
	printf( "\n" );
	
	printf( "Owner: " );
	print_gui_brief( b->owner );
	printf( "\n" );
	
	printf( "Origin: " );
	print_gui_brief( b->origin );
	printf( "\n" );
	
	printf( "Target: " );
	print_gui_brief( b->target );
	printf( "\n" );
	
	return;
}



/* print_hook_notice_end()
Helper function to print a hook [end] header.
*/
static void print_hook_notice_end( void )
{
	PRINT_SEP_END( "" );
	return;
}



/* print_diff_hook()
Compare two hook structs, both for the same HOOK object, and print any significant differences.

'a' is the old hook info
'b' is the new hook info
'deskname' is the name of the desktop the HOOK is on
*/
void print_diff_hook
	const struct hook *const a,   // in
	const struct hook *const b,   // in
	const WCHAR *const deskname   // in
)
{
	unsigned modified = FALSE;
	FAIL_IF( !a );
	FAIL_IF( !b );
	FAIL_IF( !deskname );
	
	
	if( a->entry.bFlags != b->entry.bFlags )
	{
		BYTE temp = 0;
		
		
		if( !modified )
		{
			modified = TRUE;
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
		}
		
		printf( "\nThe associated HANDLEENTRY's flags have changed.\n" );
		
		temp = (BYTE)( a->entry.bFlags & b->entry.bFlags );
		if( temp )
		{
			printf( "Flags same: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( a->entry.bFlags & ~b->entry.bFlags );
		if( temp )
		{
			printf( "Flags removed: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( b->entry.bFlags & ~a->entry.bFlags );
		if( temp )
		{
			printf( "Flags added: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
	}
	
	/* the gui->owner struct has the process and thread info for entry.pOwner */
	print_diff_gui( "owner", a->owner, b->owner );
	
	if( a->object.head.h != b->object.head.h )
	{
		printf( "\nThe HOOK's handle has changed.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.head.h );
		PRINT_NAME_FOR_PTR( "New", b->object.head.h );
	}
	
	if( a->object.head.cLockObj != b->object.head.cLockObj )
	{
		printf( "\nThe HOOK's lock count has changed.\n" );
		printf( "Old: %lu\n", a->object.head.cLockObj );
		printf( "New: %lu\n", b->object.head.cLockObj );
	}
	
	/* the gui->origin struct has the process and thread info for pti */
	print_diff_gui( "origin", a->origin, b->origin );
	
	if( a->object.rpdesk1 != b->object.rpdesk1 )
	{
		printf( "\nrpdesk1 has changed. The desktop that the HOOK is on has changed?\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.rpdesk1 );
		PRINT_NAME_FOR_PTR( "New", b->object.rpdesk1 );
	}
	
	if( a->object.pSelf != b->object.pSelf )
	{
		printf( "\nThe HOOK's kernel address has changed.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.pSelf );
		PRINT_NAME_FOR_PTR( "New", b->object.pSelf );
	}
	
	if( a->object.phkNext != b->object.phkNext )
	{
		printf( "\nThe HOOK's chain has been modified.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.phkNext );
		PRINT_NAME_FOR_PTR( "New", b->object.phkNext );
	}
	
	if( a->object.iHook != b->object.iHook )
	{
		printf( "\nThe HOOK's id has changed.\n" );
		
		printf( "Old: " );
		print_HOOK_id( a->object.iHook );
		printf( "\n" );
		
		printf( "New: " );
		print_HOOK_id( b->object.iHook );
		printf( "\n" );
	}
	
	if( a->object.offPfn != b->object.offPfn )
	{
		printf( "\nThe HOOK's function offset has changed.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.offPfn );
		PRINT_NAME_FOR_PTR( "New", b->object.offPfn );
	}
	
	if( a->object.flags != b->object.flags )
	{
		BYTE temp = 0;
		
		
		printf( "\nThe HOOK's flags have changed.\n" );
		
		temp = (BYTE)( a->object.flags & b->object.flags );
		if( temp )
		{
			printf( "Flags same: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( a->object.flags & ~b->object.flags );
		if( temp )
		{
			printf( "Flags removed: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( b->object.flags & ~a->object.flags );
		if( temp )
		{
			printf( "Flags added: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
	}
	
	if( a->object.ihmod != b->object.ihmod )
	{
		printf( "\nThe HOOK's function module atom index has changed.\n" );
		printf( "Old: %d\n", a->object.ihmod );
		printf( "New: %d\n", b->object.ihmod );
	}
	
	/* the gui->target struct has the process and thread info for ptiHooked */
	print_diff_gui( "target", a->target, b->target );
	
	if( a->object.rpdesk2 != b->object.rpdesk2 )
	{
		printf( "\nrpdesk2 has changed. HOOK locked, owner destroyed?\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.rpdesk2 );
		PRINT_NAME_FOR_PTR( "New", b->object.rpdesk2 );
	}
	
	
	return;
}



/* print_diff_desktop_hook_items()
Print the HOOKs that have been added/removed from a single attached to desktop between snapshots.
*/
void print_diff_desktop_hook_items( 
	const struct desktop_hook_item *const a,   // in
	const struct desktop_hook_item *const b   // in
)
{
	WCHAR *deskname = NULL;
	unsigned a_hi = 0, b_hi = 0;
	
	FAIL_IF( !a );
	FAIL_IF( !b );
	
	/* Both desktop hook items should have a pointer to the same desktop item */
	FAIL_IF( !a->desktop );
	FAIL_IF( !b->desktop );
	FAIL_IF( a->desktop != b->desktop );
	FAIL_IF( a->hook_max != b->hook_max );
	
	
	deskname = b->desktop->pwszDesktopName;
	
	a_hi = 0, b_hi = 0;
	while( ( a_hi < a->hook_count ) && ( b_hi < b->hook_count ) )
	{
		int ret = compare_hook( &a->hook[ a_hi ], &b->hook[ b_hi ] );
		
		if( ret < 0 ) // hook removed
		{
			if( match( &a->hook[ a_hi ] ) )
			{
				print_hook_notice_begin( &a->hook[ a_hi ], deskname, HOOK_REMOVED );
				print_hook_notice_end();
			}
			
			++a_hi;
		}
		else if( ret > 0 ) // hook added
		{
			if( match( &b->hook[ b_hi ] ) )
			{
				print_hook_notice_begin( &b->hook[ b_hi ], deskname, HOOK_ADDED );
				print_hook_notice_end();
			}
			
			++b_hi;
		}
		else
		{
			/* The hook info exists in both snapshots (same HOOK object).
			In this case check there is no reason to print the HOOK again unless certain 
			information has changed (like the hook is hung, etc).
			*/
			print_diff_hook( &a->hook[ a_hi ], &b->hook[ b_hi ], deskname );
			
			++a_hi;
			++b_hi;
		}
	}
	
	while( a_hi < a->hook_count ) // hooks removed
	{
		print_hook_notice_begin( &a->hook[ a_hi ], deskname, HOOK_REMOVED );
		print_hook_notice_end();
		
		++a_hi;
	}
	
	while( b_hi < b->hook_count ) // hooks added
	{
		print_hook_notice_begin( &b->hook[ b_hi ], deskname, HOOK_ADDED );
		print_hook_notice_end();
		
		++b_hi;
	}
	
	return;
}



/* print_diff_desktop_hook_lists()
Print the HOOKs that have been added/removed from all attached to desktops between snapshots.
*/
void print_diff_desktop_hook_lists( 
	const struct desktop_hook_list *const list1,   // in
	const struct desktop_hook_list *const list2   // in
)
{
	struct desktop_hook_item *a = NULL;
	struct desktop_hook_item *b = NULL;
	
	FAIL_IF( !list1 );
	FAIL_IF( !list2 );
	
	
	for( a = list1->head, b = list2->head; ( a && b ); a = a->next, b = b->next )
		print_diff_desktop_hook_items( a, b );
	
	if( a || b )
	{
		MSG_FATAL( "The desktop hook stores could not be fully compared." );
		exit( 1 );
	}
	
	return;
}


