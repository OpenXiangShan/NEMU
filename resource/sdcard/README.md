
# NEMU sdhost驱动

本驱动裁剪自`linux/drivers/mmc/host/bcm2835.c`, 去除了DMA和中断, 改成直接轮询, 处理器无需支持DMA和中断即可运行.
不支持写操作, 可以以只读方式启动debian.
若需要测试文件系统的写操作, 可在启动debian后挂载ramdisk, 在ramdisk上进行写入.

使用方法:
* 将本目录下的`nemu.c`复制到`linux/drivers/mmc/host/`目录下
* 在`linux/drivers/mmc/host/Makefile`中添加一行`obj-y += nemu.o`
* 在menuconfig中取消`General setup -> Initial RAM filesystem and RAM disk (initramfs/initrd) support`
* 在menuconfig中选中`Device Drivers -> MMC/SD/SDIO card support`
* 在dts中加入以下节点
```
/ {
  soc {
    sdhci: mmc {
      compatible = "nemu-sdhost";
      reg = <0x0 0xa3000000 0x0 0x1000>;
    };
  };

  chosen {
    bootargs = "root=/dev/mmcblk0 rootfstype=ext4 ro rootwait earlycon";
  };
};
```
