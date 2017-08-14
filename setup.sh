function sync {
		if [[ -d $dir ]]; then
				source=$dir
				adjremote=1
		else
				source=$remote
		fi
		if [[ -d $dest ]]; then
				cd $dest
				git pull $source
				cd ..
		else
				git clone $source $dest
				if [[ -n "$adjremote" ]]; then
						cd $dest
						git remote origin set-url $remote
						cd ..
				fi
		fi
}

set -e

dir=/extra/home/code/htmlish
remote=https://github.com/cyisfor/htmlish.git
dest=htmlish
sync

ln -rs ./htmlish/html_when .
ln -rs ./htmlish/html_when/libxml2 .

dir=/extra/home/code/packages/git/ddate/
remote=https://github.com/cyisfor/ddate.git
dest=ddate
sync
