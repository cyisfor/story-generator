function sync {
		if [[ -d $dir ]]; then
				source=$dir
				adjremote=1
		else
				echo $dir
				source=$remote
		fi
		if [[ -d $dest ]]; then
				[[ -n "$nocheck" ]] && return
				cd $dest
				git pull local master
				git pull origin  master
				cd ..
		else
				git clone $source $dest
				if [[ -n "$adjremote" ]]; then
						cd $dest
						git remote set-url origin $remote
						git remote add local $source
						#git pull
						cd ..
				fi
		fi
}

function uplink {
		source=$1/$2
		[[ -L $2 ]] && return;
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
