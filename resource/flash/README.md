# NEMU preset flash

flash device 默认数据

## 使用方法

- 将环境变量`NEMU_HOME`设置为[NEMU project](https://github.com/OpenXiangShan/NEMU)的绝对路径.
- 将`config/`目录下的`config`文件重命名为`.config`文件，复制到`.`目录下
- `make menuconfig`
- `make`