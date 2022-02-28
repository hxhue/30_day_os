static unsigned long rand_seed_next = 1;

int rand() {
  rand_seed_next = rand_seed_next * 1103515245U + 12345U;
  return (int)((unsigned)rand_seed_next / 65536U % 32768U);
}

void srand(unsigned seed) { rand_seed_next = seed; }
