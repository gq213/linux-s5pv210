
make 210_defconfig

make menuconfig


make zImage -j4

make 210-sate210.dtb

./pack_kernel.sh 210-sate210.dtb

make 210-tq210v4.dtb

./pack_kernel.sh 210-tq210v4.dtb


make modules -j4

make modules_install INSTALL_MOD_PATH=/mnt/fr/modules


make distclean
