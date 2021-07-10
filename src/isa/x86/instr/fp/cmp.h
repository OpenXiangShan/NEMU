def_EHelper(fucomi) {
  if (dfdest == dfsrc1) { 
    rtl_set_CF(s, rz);
    rtl_li(s, s0, 1);
    rtl_set_ZF(s, s0);
    rtl_set_PF(s, rz);
  } else {
    rtl_fcmp(s, dfsrc1, dfdest);
  }
}
