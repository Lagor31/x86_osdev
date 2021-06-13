extern void _system_call(int n);



int a(){
  return 1;
}

int _start() {
  _system_call(10);
  return 0;
}