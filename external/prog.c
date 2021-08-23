extern void _system_call(int n, int par);

int a() { return 1; }

int _start() {
  while (1) {
    _system_call(3, 3000);
    _system_call(2, 3131);

    /* char *nono = (char *)0xC0100000;
    char r = nono[0]; */
  }
  _system_call(1, 0);
  return 0;
}