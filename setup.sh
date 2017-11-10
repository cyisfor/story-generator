. git-tools/funcs.sh

local=/home/code/htmlish
remote=https://github.com/cyisfor/htmlish.git
dest=htmlish
clonepull

uplink ./htmlish/ html_when
uplink ./htmlish/html_when/ libxml2

local=/home/code/git-tools
remote=https://github.com/cyisfor/git-tools.git
dest=git-tools
clonepull

local=/extra/home/packages/git/ddate/
remote=https://github.com/cyisfor/ddate.git
dest=ddate
clonepull

local=/home/code/str_to_enum_trie/
remote=https://github.com/cyisfor/str_to_enum_trie.git
dest=str_to_enum_trie
clonepull

local=/home/code/ctemplate/
remote=https://github.com/cyisfor/ctemplate.git
dest=ctemplate
clonepull
