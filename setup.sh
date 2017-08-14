function sync {
		echo $dir
		if [[ -d $dir ]]; then
				source=$dir
				adjremote=1
		else
				exit 23
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
						git remote set-url origin $remote
						cd ..
				fi
		fi
}

function uplink {
		source=$1/$2
		[[ -L $source ]] && return;
		ln -rs $source $2
}

dir=/home/code/htmlish
remote=https://github.com/cyisfor/htmlish.git
dest=htmlish
sync

uplink ./htmlish/ html_when
uplink ./htmlish/html_when/ libxml2

dir=/extra/home/packages/git/ddate/
remote=https://github.com/cyisfor/ddate.git
dest=ddate
sync
