---
name: Bug report
about: Create a report to help us improve
title: ''
labels: ''
assignees: ''

---

**Describe the bug**

- What workload are you running? Bare-metal workload, bbl+Linux+application or checkpoints?
- What is the expected behavior? What is the actual behavior?
- Can this issue be consistently reproduced? If so, what are the steps to reproduce? Can this issue be reproduced in latest master branch?
- Can you upload your bare-metal image or checkpoint to somewhere to help us to reproduce the issue?

**To Reproduce**
Steps to reproduce the behavior:
- Build command: typically `make xxx_config; make -j`
- Running command: typically
```
./build/riscv64-nemu-interpreter /path/to/checkpoint-or-image -b
```
- The link to your image or checkpoint; Or the way to build the image (if not to complicated to build)

**Expected behavior**
A clear and concise description of what you expected to happen.

**Error log or Screenshots**

If applicable, plz provide you error log.

If applicable, add screenshots to help explain your problem.


**Necessary information on versions**
- NEMU version: [e.g. master branch or commid: deadbeaf]

**Additional information**
If you build the full-system image your self, you can optionally provide version information of following components:
- riscv-linux
- riscv-rootfs
    - initramfs.txt (attch it if possible)
- riscv-pk or OpenSBI
- GCPT restorer (checkpoint restorer), it might be NEMU in older versions or OpenXiangShan/LibCheckpointAlpha or OpenXiangShan/LibCheckpoint in newer versions

**Additional context**
Add any other context about the problem here.
