#***************************************************************************************
# Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#!/bin/bash

ISA=${1#*ISA=}
CPUTEST_PATH=$NEMU_HOME/../am-kernels/tests/cpu-tests

echo "compiling NEMU..."
if make -C $NEMU_HOME ISA=$ISA; then
  echo "NEMU compile OK"
else
  echo "NEMU compile error... exit..."
  exit
fi

echo "compiling testcases..."
if make -C $CPUTEST_PATH ARCH=$ISA-nemu &> /dev/null; then
  echo "testcases compile OK"
else
  echo "testcases compile error... exit..."
  exit
fi

files=`ls $CPUTEST_PATH/build/*-$ISA-nemu.bin`
ori_log="$NEMU_HOME/build/nemu-log.txt"

for file in $files; do
  base=`basename $file | sed -e "s/-$ISA-nemu.bin//"`
  printf "[%14s] " $base
  logfile=$NEMU_HOME/build/$base-log.txt
  make -C $NEMU_HOME ISA=$ISA run ARGS="-b -l $ori_log $file" &> $logfile

  if (grep 'nemu: .*HIT GOOD TRAP' $logfile > /dev/null) then
    echo -e "\033[1;32mPASS!\033[0m"
    rm $logfile
  else
    echo -e "\033[1;31mFAIL!\033[0m see $logfile for more information"
    if (test -e $ori_log) then
      echo -e "\n\n===== the original log.txt =====\n" >> $logfile
      cat $ori_log >> $logfile
    fi
  fi
done
