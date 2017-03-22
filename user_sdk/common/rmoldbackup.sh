#!/bin/sh

filelist=`ls ${1}* | sort`

cnt=1;
for filetmp in ${filelist};
do
	((cnt++))
	if [ "${cnt}" -gt 10 ];then
		echo ${filetmp}
		rm ${filetmp}
	fi
done

