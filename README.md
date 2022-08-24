
1、编译集成initramfs( https://github.com/gq213/buildroot-s5pv210 )和dtb的kernel

make 210_defconfig 210-appended-dtb.config 210-enable-initrd.config

make zImage -j4

1)生成适用于sate210的zImagedtb_pad

make 210-sate210.dtb

./pack_kernel.sh 210-sate210.dtb

2)生成适用于tq210v4的zImagedtb_pad

make 210-tq210v4.dtb

./pack_kernel.sh 210-tq210v4.dtb

3)烧录zImagedtb_pad到sd卡相应分区


2、编译不含initramfs、独立dtb的kernel

make 210_defconfig

make zImage -j4

make 210-sate210.dtb

make 210-tq210v4.dtb

拷贝zImage、210-sate210.dtb、210-tq210v4.dtb到sd卡相应分区


3、编译驱动模块

make modules -j4

make modules_install INSTALL_MOD_PATH=/mnt/disk/modules
