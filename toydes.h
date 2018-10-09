#ifndef _TOYDES_H_
#define _TOYDES_H_


class Toydes
{
public:
	Toydes() {};
	std::string cts(unsigned int c, unsigned int length);
	unsigned char encryptByte(unsigned char msg, unsigned int key);
	unsigned char decryptByte(unsigned char cyp, unsigned int key);

private:
	unsigned int p4t4(unsigned int in);
	unsigned int r4t4(unsigned int in);
	unsigned int p4t8(unsigned int in);
	unsigned int p8t8(unsigned int in);
	unsigned int r8t8(unsigned int in);
	unsigned int p10t8(unsigned int in);
	unsigned int p10t10(unsigned int in);
	unsigned int sbox(unsigned int in, unsigned int s[4][4]);
	unsigned int f(unsigned int in, unsigned int k);
	std::vector<unsigned int> keyman(unsigned int key);
	//std::vector<unsigned int> r(std::vector<unsigned int> k);

	unsigned int s1[4][4] = {{1,0,3,2}, {3,2,1,0}, {0,2,1,3}, {3,1,3,2}};
	unsigned int s2[4][4] = {{0,1,2,3}, {2,0,1,3}, {3,0,1,0}, {2,1,0,3}};
};

#endif