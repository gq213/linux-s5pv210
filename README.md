
make 210_defconfig
make menuconfig

make zImage -j4
make 210-sate210.dtb
cat arch/arm/boot/zImage arch/arm/boot/dts/210-sate210.dtb > zImagedtb

make distclean
