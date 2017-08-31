. git-tools/funcs.sh

local=/home/code/git-tools
remote=https://github.com/cyisfor/git-tools.git
dest=git-tools
clonepull

local=/home/code/htmlish
remote=https://github.com/cyisfor/htmlish.git
dest=htmlish
clonepull

uplink ./htmlish/ html_when
uplink ./htmlish/html_when/ libxml2

local=/extra/home/packages/git/ddate/
remote=https://github.com/cyisfor/ddate.git
dest=ddate
clonepull

local=/home/code/src_to_enum_trie/
remote=https://github.com/cyisfor/src_to_enum_trie.git
dest=src_to_enum_trie
clonepull
