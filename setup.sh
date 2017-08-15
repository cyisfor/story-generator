. git-tools/funcs.sh

local=/home/code/htmlish
remote=https://github.com/cyisfor/htmlish.git
dest=htmlish
sync

uplink ./htmlish/ html_when
uplink ./htmlish/html_when/ libxml2

local=/extra/home/packages/git/ddate/
remote=https://github.com/cyisfor/ddate.git
dest=ddate
sync
