#ifndef _RANDOMPRIMEGENERATOR_H_
#define _RANDOMPRIMEGENERATOR_H_

class RandomPrimeGenerator
{
      public:
      RandomPrimeGenerator(){mode=NONE; srand(std::time(0));}
      ~RandomPrimeGenerator(){}

      void init_fast(unsigned int _max_number);
      void init_cheap(unsigned int _max_prime_count);

      unsigned int get();
      
      private:
      unsigned int max_number;
      unsigned int max_prime_count;
      enum {NONE=0,FAST=1,CHEAP=2} mode;
      std::vector<unsigned int> prime_list;
      void find_primes();
};

#endif