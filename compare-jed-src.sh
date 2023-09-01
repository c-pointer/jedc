#!/bin/bash

root_dir=/lanfs/src
js_dir=jed/src
nc_dir=jedc/src

function ztitle {
	local n
	local cols
	n=0
	cols=$(tput cols)
	echo -n -e "\033[32m"
	while [ $n -lt $cols ]; do
		n=$((n+1))
		echo -n '-'
	done
	echo -n -e "\r--- $* "
	echo -n -e "\033[0m  \b\b"
	}

todo=$1
#if [ -z "$todo" ]; then
#	exit 1
#fi

cd $root_dir
list=$js_dir/*.[ch]
for f in $list; do
	f=$(basename $f)
	echo $(printf '%040s' | tr '0' '=')
	ztitle $f
	echo
	if ! diff $js_dir/$f $nc_dir/$f; then 
		ztitle "copy to $nc_dir/$f (y/n/q/m)?"
		read -e choice
		[[ "$choice" == [Yy]* ]] && cp $js_dir/$f $nc_dir/$f
		[[ "$choice" == [Qq]* ]] && exit
		[[ "$choice" == [Mm]* ]] && meld $js_dir/$f $nc_dir/$f
		echo
	fi
done
