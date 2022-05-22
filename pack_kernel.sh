#!/bin/bash

SRC_KERNEL=./zImagedtb
DST_KERNEL=./zImagedtb_pad
PAD_HEAD=./head
TMP_IMG=./tmp_img

rm -rf ${SRC_KERNEL}

if [ ! -f arch/arm/boot/dts/$1 ]; then
	echo "not found arch/arm/boot/dts/$1"
	exit -1
fi

cat arch/arm/boot/zImage arch/arm/boot/dts/$1 > ${SRC_KERNEL}

rm -rf ${DST_KERNEL}

dec2hex(){
	printf "%08x" $1
}

b2l(){
	str="$1"
	#echo $str
	c1=${str::2}
	c2=${str:2:2}
	c3=${str:4:2}
	c4=${str:6:2}
	r=${c4}${c3}${c2}${c1}
	#echo $r
	printf "%s" $r
}

FILE_SIZE_STR=$(stat -c %s ${SRC_KERNEL})
echo ${FILE_SIZE_STR}

FILE_SIZE_STR_512=$(((($FILE_SIZE_STR)+(512)-1)&~((512)-1)))
echo ${FILE_SIZE_STR_512}

FILE_SIZE_BIG=$(dec2hex ${FILE_SIZE_STR_512})
echo ${FILE_SIZE_BIG}

FILE_SIZE_LITTLE=$(b2l ${FILE_SIZE_BIG})
echo ${FILE_SIZE_LITTLE}

echo ${FILE_SIZE_LITTLE} | xxd -r -ps > ${PAD_HEAD}
dd if=/dev/zero of=${PAD_HEAD} bs=4 seek=1 count=127

dd if=/dev/zero of=${TMP_IMG} bs=${FILE_SIZE_STR_512} count=1
dd if=${SRC_KERNEL} of=${TMP_IMG} conv=notrunc

cat ${PAD_HEAD} ${TMP_IMG} > ${DST_KERNEL}

rm -rf ${PAD_HEAD} ${TMP_IMG}

echo "done !!!"
