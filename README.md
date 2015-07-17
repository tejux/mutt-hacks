# mutt-hacks
Hacked version of mutt. Following hacks are done.

1)	Sidebar jump-to mailbox
	sidebar is great, but people have too many mailboxes and going next-prev
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

