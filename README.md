# mutt-hacks
Hacked version of mutt. Following hacks are done.

1)	Sidebar jump-to mailbox

	Sidebar is great, but people have too many mailboxes and going next-prev
	is too slow. This hack adds a key-binding to jump to a configured set of 
	mailboxes. e.g. configuration

	bind index,pager + sidebar-jump-key1
	bind index,pager & sidebar-jump-key2

	set sidebar_jump_mailbox1="Important\ Mail"
	set sidebar_jump_mailbox2="INBOX"

	# sets + as a shortcut to mailbox1, where mailbox1 is "Important Mail".
	  and sets & as a shortcut to mailbox2, where mailbox2 is "INBOX"
	  sidebar_jump_mailbox3, sidebar-jump-key3 are allowed.
	  At-most 3 jump-keys and mailbox names can be given.

	  Does this functionality exist in mutt itself maybe ? Ability to set
	  a shortcut to jump to a mailbox ??

2)	Sidebar color hacks

	By default, sidebar indicator is same color as the indicator in scroll window.
	Minor hack to add a new color key "sidebar_indicator", to have a separate color
	for the currently active mailbox shown in sidebar.

	Minor hack to add a new color key "sidebar_spoolfile", to color the spoolfile folder
	in the sidebar listing.

