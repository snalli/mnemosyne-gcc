#!/bin/bash

THIS=deploy.sh
REMOTE_HOST=adsl-q07
REMOTE_ROOTDIR=/home/hvolos/workspace/mnemosyne_build
LOCAL_ROOTDIR=/scratch/workspace/mnemosyne/src/usermode

function execute_remote {
	deploy_dir=${REMOTE_ROOTDIR}/$1
	mkdir $deploy_dir
	cd $deploy_dir
	tar -xf /tmp/mnemosyne.tar
}

function execute_local {
	if [ -z $1 ] 
	then
		return
	fi
	deploy_name=$1
	tar -cf /tmp/mnemosyne.tar -C $LOCAL_ROOTDIR .
	scp /tmp/mnemosyne.tar $REMOTE_HOST:/tmp
	scp $THIS $REMOTE_HOST:/tmp
	ssh $REMOTE_HOST /tmp/$THIS --remote $deploy_name
}



if [ "$1" = "-r" -o  "$1" = "--remote" ] 
then
	execute_remote $2
else
	execute_local $1
fi
