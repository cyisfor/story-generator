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

function uplink {
		source=$1/$2
		[[ -L $source ]] && return;
		ln -rs $source $2
}

dir=/extra/home/code/htmlish
remote=https://github.com/cyisfor/htmlish.git
dest=htmlish
sync

uplink ./htmlish/ html_when
uplink ./htmlish/html_when/ libxml2

dir=/extra/home/code/packages/git/ddate/
remote=https://github.com/cyisfor/ddate.git
dest=ddate
sync
