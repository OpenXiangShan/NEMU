
# riscv64 debian镜像制作

制作需要`qemu-riscv64-static`, 建议在debian 10或ubuntu 19.04的系统(可尝试使用docker)中进行操作.

* 创建ext4空镜像
```
dd if=/dev/zero of=debian.img bs=1G count=4  # 此处镜像大小为4GB
sudo mkfs.ext4 debian.img
```

* 挂载空镜像
```
sudo mount debian.img /mnt -o loop
```

* 安装debian base system.
下面两条命令的操作来自[debian社区的安装指南](https://wiki.debian.org/RISC-V#debootstrap).
```
sudo apt-get install debootstrap qemu-user-static binfmt-support debian-ports-archive-keyring
sudo debootstrap --arch=riscv64 --keyring /usr/share/keyrings/debian-ports-archive-keyring.gpg --include=debian-ports-archive-keyring unstable /mnt http://deb.debian.org/debian-ports
```

* 进入镜像
```
sudo chroot /mnt /bin/bash
```
此时实际上是通过`qemu-riscv64-static`来执行镜像中的riscv64可执行文件.

* 安装所需工具(根据实际情况选择)
```
apt-get update
apt-get install gcc build-essential
apt-get install tmux libreadline-dev
agt-get install net-tools openssh-server
# fix long delay of openssh server
apt-get install haveged
agt-get install sbt
```

* 删除登录密码, 登录时输入`root`后可直接登录
```
passwd -d root
```

* 添加/root/目录的写和执行权限, 使得host上的普通用户可以访问
```
chmod +w,+x /root
```

* 在/root/目录下提前写入所需的测试文件, 如hello.c等. 将来以只读方式挂载时, 无法写入文件.

* 若要创建文件, 可在/run/目录下创建, 它是个tmpfs

* 在/root/.bashrc中添加如下内容, 可以实现登录后自动运行命令(根据实际情况修改测试的命令):
```
TMP_DIR=/run/mytest

cmd=(
# show system information
  "uname -a"
  "cat /etc/issue"
  "cat /proc/cpuinfo"
  "df -ah"
  "free -h"

# show time
  "date"
  "uptime"

# create and switch to tmp directory
  "mkdir $TMP_DIR"
  "cd $TMP_DIR"

# compile and run hello
  "ls /root"
  "ls /root/hello"
  "cat /root/hello/hello.c"
  "gcc -time /root/hello/hello.c -o $TMP_DIR/hello"
  "ls -lh $TMP_DIR"
  "$TMP_DIR/hello"

# compile and run x86-nemu
  "ls /root/nemu"
  "cp -r /root/nemu $TMP_DIR"
  "export NEMU_HOME=$TMP_DIR/nemu"
  "make -C $TMP_DIR/nemu ISA=x86"
  "ls -lh /root/nemu-prog"
  "file /root/nemu-prog/amtest-x86-nemu.elf"
  "$TMP_DIR/nemu/build/x86-nemu --batch --mainargs=h /root/nemu-prog/amtest-x86-nemu.bin"
  "file /root/nemu-prog/microbench-x86-nemu.elf"
  "$TMP_DIR/nemu/build/x86-nemu --batch --mainargs=test /root/nemu-prog/microbench-x86-nemu.bin"

# compile and run riscv64-nemu
  "make -C $TMP_DIR/nemu clean"
  "make -C $TMP_DIR/nemu ISA=riscv64"
  "$TMP_DIR/nemu/build/riscv64-nemu --batch /root/nemu-prog/linux-hello-riscv64-nemu.bin"
)

prompt="`whoami`@`hostname`:`pwd`#"

echo -e "\n============ Now running preset commands =============\n"

for ((i = 0; i < ${#cmd[@]}; i++)); do
  c=${cmd[$i]}
  echo "$prompt $c"
  $c
  echo ""
done

echo -e "\n============ End of preset commands =============\n"

/root/nemutrap/good-trap
```

* 若在不方便输入的环境(如NEMU, verilator仿真等)中测试, 可采用如下方式避免登录时输入
```
cd /lib/systemd/system
# 通过紧急模式登录, 不启动非必须的服务, 节省将近一半的登录时间
ln -sf emergency.target default.target
# 跳过登录提示符, 直接运行bash
vim emergency.service
  -ExecStart=-/lib/systemd/systemd-sulogin-shell emergency
  +ExecStart=-/bin/bash
```

* 退出并卸载镜像
```
exit  # 之前通过`chroot`方式进入
sudo umount /mnt  # 记得卸载! 在未卸载镜像的情况下通过可写方式再次打开`debian.img`(如作为qemu的文件系统), 镜像将会损坏!
```

* 修改`nemu/src/device/sdcard.c`中`init_sdcard()`中打开的镜像文件路径, 即可使用制作的镜像.
在i9-9900k上测试, 约90s后看到debian的登录提示符.

* 根据实际情况修改`nemu/src/device/serial.c`中允许串口输入的等待时间,
使得出现登录提示符后自动从串口读入"root\n"进行登录, 然后运行`.bashrc`中预设的命令,
实现一键运行自动测试.
