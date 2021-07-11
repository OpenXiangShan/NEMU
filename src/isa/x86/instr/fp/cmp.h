def_EHelper(fucomi) {
  rtl_fcmp(s, dfsrc1, dfdest);
}

def_EHelper(fucomip) {
  rtl_fcmp(s, dfsrc1, dfdest);
  ftop_pop();
}

def_EHelper(fcomi) {
  rtl_fcmp(s, dfsrc1, dfdest);
}

def_EHelper(fcomip) {
  rtl_fcmp(s, dfsrc1, dfdest);
  ftop_pop();
}
