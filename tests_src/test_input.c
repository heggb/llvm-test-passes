__attribute__((noreturn)) void boom(void) { __builtin_trap(); }
void foo(void);

int base(int c) {

  if (c) return 1;
  boom(); 
}

void if_merge(int c) {
  if (c) {          
    (void)0;        
  } else {
    (void)0;        
  }
  (void)0;          
}

int sw(int x) {
  switch (x) {                 
    case 1: return 1;          
    case 2: return 2;          
    default: return 0;         
  }
}

void plain_br(void) {
  goto L;                      
L:
  (void)0;
}

void cbr(int cond) {
  asm goto ("" : : : : L1, L2);
  return;
L1:
  foo();                       
  return;
L2:
  foo();                       
  return;
}

void indirect(int s) {
  static void *tbl[] = { &&A, &&B };
  void *p = tbl[s & 1];
  goto *p;                     
A:
  (void)0;                  
  return;
B:
  (void)0;                 
  return;
}

void unr(int c) {
  if (!c) boom();          
  return;                    
}

void loop_preheader(int n) {
  for (int i = 0; i < n; ++i) {  
    (void)0;
  }
}

void pseudo(int c) {
  goto HDR;
HDR:                             
  if (c--) goto BODY; else goto EX;
BODY:
  if (c) goto HDR; else goto EX;
EX:
  return;                         
}

void nested(int n, int m) {
  for (int i = 0; i < n; ++i) {     
    for (int j = 0; j < m; ++j) {    
      (void)0;                      
    }                                
  }                                
}

int stable(int x) {
  if (x == 0) {                      
    (void)0;                       
  } else {
    switch (x) {                   
      case 1: (void)0; break;      
      default: (void)0;            
    }
  }
  (void)0;                          
  if (0) { U: ; }                   
  return x;
}

void auto_bb(int k) {
  if (k) goto Z;                    
  (void)0;                          
Z:
  return;                          
}

void island(void) {
  return;
isle:
  return;                          
}

void foo(void) { }

void if_chain(int x) {
  if (x < 0) { (void)0; }    
  else if (x == 0) { (void)0; }  
  else { (void)0; }
  (void)0;
}

int if_ret(int x) {
  if (x == 42) { return 1; }
  (void)0;
  return 0;
}

int sw_alias(int x) {
  int r = 0;
  switch (x) {
    case 1:
    case 2:
    case 3: r = 7; break;   
    default: r = -1; break;
  }
  return r;
}

int sw_nodef(int x) {
  switch (x) {
    case 10: return 1;
    case 20: return 2;
  }
  return 0;
}

void loop_multi(int n, int *a) {
  int i = 0;
  while (i < n) {
    if (a[i] == 0) break;  
    if (a[i] < 0) { ++i; continue; }
    ++i;                      
  }                       
}

void pseudo2(int c) {
  if (c) goto A;
  else   goto A;   
A:               
  for (int i = 0; i < 1; ++i) { (void)0; }
}

void cbr0(void) {
  asm goto ("" : : : : LDEF);
  return;            
LDEF:                
  return;
}

void ibr0(void *p) {
  (void)p;
}

int dead_with_pred(int c) {
  if (c) {
    goto DEAD;
  }
  return 0;
DEAD:             
  return 1;
}