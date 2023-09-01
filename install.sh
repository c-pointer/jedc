#!/bin/sh
#
# JED Linux installation script to be used with CBRIEF
# by Nicholas Christopoulos
#
# void packages required: base-devel slang slang-devel xclip
# slackware --//--      : xclip
# alpine packages -//-  : base-build slang2 slang2-dev xclip
# debianoids --//--     : build-essential libslang2 libslang2-dev slsh xclip
#

# fix /usr/jed link
if [ ! -e /usr/jed ]; then
	if [ ! -d /usr/share/jed ]; then
		mkdir /usr/share/jed
	fi
	if [ ! -d /usr/share/jed/bin ]; then
		mkdir /usr/share/jed/bin
	fi
	ln -s /usr/share/jed /usr/jed
	if [ -d /usr/share/doc ]; then
		ln -s /usr/share/jed/doc /usr/share/doc/jed
	fi
fi

# build
export CFLAGS="-g -O"
if ./configure --prefix=/usr --disable-gpm; then
	if make jed; then
		make rgrep
		if [ -e src/objs/rgrep ]; then
			if [ -d /usr/share/jed/bin ]; then
				cp src/objs/rgrep /usr/share/jed/bin
			fi
			if [ -d /usr/local/bin ]; then
				cp src/objs/rgrep /usr/local/bin
			fi
		fi
		make xjed
		rm -rf /usr/share/jed/lib/*
		
		# install
		make install
		if [ -d desktop ]; then
			[ -d /usr/share/icons ] && cp desktop/*.svg /usr/share/icons
			[ -d /usr/share/applications ] && cp desktop/*.desktop /usr/share/applications
		fi
		if [ -d etc/profile.d ]; then
			[ -d /etc/profile.d ] && cp etc/profile.d/* /etc/profile.d
		fi
	fi
fi
