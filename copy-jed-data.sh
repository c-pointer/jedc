#!/bin/sh

js_dir=../jed

list=$js_dir/*
for s in $list; do
	if [ -d $s ]; then
		base=$(basename $s)
		if [ $base != "src" ]; then
			echo "Directory copying $s"
			cp -r $s .
		else
			echo "Directory $s ignored"
		fi
	else
		echo "Copying file $s"
		cp $s .
	fi
done
