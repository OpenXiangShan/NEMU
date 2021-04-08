def_REHelper(out) {
  rtl_hostcall(s, HOSTCALL_PIO, ddest, dsrc1, NULL, (0 << 4) | width);
}
