extern void _system_call(int num, int par);

int a() { return 1; }
int _start() {
  int i = 0;
  while (i++ < 20) {
    _system_call(3, 3000);
    _system_call(2, 3131);

    /*   char *nono = (char *)0x0804a000;
      nono[0] = 'F'; */
  }
  _system_call(1, 0);
  return 0;
}