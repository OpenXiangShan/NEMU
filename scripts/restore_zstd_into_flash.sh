#***************************************************************************************
# Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/

mem_cpt=`find output_top/test/linux/50000000 -name "*_memory_.zstd"`
flash_cpt=`find output_top/test/linux/50000000 -name "*_flash_.zstd"`

./build/riscv64-nemu-interpreter -b \
    --diff ${SPIKE_SO} \
    --flash-image $flash_cpt -I 100000000 \
    --cpt-restorer $NEMU_HOME/resource/gcpt_restore/build/gcpt.bin \
    $mem_cpt

