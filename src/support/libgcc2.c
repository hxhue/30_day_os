unsigned long long 
__udivmoddi4 (unsigned long long a, unsigned long long b, unsigned long long *p)
{
  *p = a % b;
  return a / b;
}