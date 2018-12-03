#ifndef CRYPT_RAND_HEAD_FILE
#define CRYPT_RAND_HEAD_FILE

#include <Bcrypt.h>
#pragma comment(lib, "Bcrypt.lib")

// // it is a singleton class
// // which aim to provide cryptographic random sequence or number
class CryptRand {
  public:
    static CryptRand& Instance()
    {
      static CryptRand instance;
      return instance;
    }

    unsigned int Number(unsigned int max, unsigned int min=0)
    {
      unsigned int n;
      BCryptGenRandom(hRng, (PUCHAR)&n, sizeof(n), 0);
      return min+(n%(max-min+1));
    }

    unsigned int Number()
    {
      unsigned int n;
      BCryptGenRandom(hRng, (PUCHAR)&n, sizeof(n), 0);
      return n;
    }

    void Read(char* src, int size) {
      BCryptGenRandom(hRng, (PUCHAR)src, size, 0);
    }

  private:
    CryptRand():hRng(0)
    {
      BCryptOpenAlgorithmProvider(&hRng, BCRYPT_RNG_ALGORITHM, NULL, 0);
    }

    // prevent copies
    CryptRand(CryptRand const&);
    void operator=(CryptRand const&);

    BCRYPT_ALG_HANDLE hRng;
};


static void RandVerifyNumbers(char * seq, int count) {
  if (!seq || count < 1) return ;

  int wrap = sizeof(unsigned int);
  int times = count/wrap + ((count%wrap) > 0 ? 1:0);

  unsigned int n;
  for (int i=0; i<times; ++i) {
    n = CryptRand::Instance().Number();
    for (int j=0; j<wrap; ++j) {
      if ((i*wrap+j) >= count) {
        return ;
      }
      seq[i*wrap+j] = ((unsigned char *)&n)[j]%10 + '0';
    }
  }
}

static void NewRpcSessionID(char* seq) {
  if (!seq) return ;
  CryptRand::Instance().Read(seq, 16);
  for (int i=0; i<16; ++i) {
    char a = ((unsigned char)seq[i])%36;
    seq[i] = a < 10 ? a+'0':a+'A'-10;
  }
}

#endif
