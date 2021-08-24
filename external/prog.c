extern void _system_call(int n);

int a() { return 1; }

int _start() {
  while (1) {
    _system_call(3);
    _system_call(2);

    /* char *nono = (char *)0xC8048000;
    char r = nono[0]; */
  }
  _system_call(1);
  return 0;
}