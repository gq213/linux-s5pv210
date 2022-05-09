
make 210_defconfig
make menuconfig

make zImage -j4
make 210-sate210.dtb
cat arch/arm/boot/zImage arch/arm/boot/dts/210-sate210.dtb > zImagedtb
make 210-tq210v4.dtb
cat arch/arm/boot/zImage arch/arm/boot/dts/210-tq210v4.dtb > zImagedtb

make modules -j4
make modules_install INSTALL_MOD_PATH=/mnt/fr/modules

make distclean
