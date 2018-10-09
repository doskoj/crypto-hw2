#include <string>
#include <vector>
#include "toydes.h"


std::string Toydes::cts(unsigned int c, unsigned int length) {
	std::string s = std::string(length, '0');
	for (unsigned int i = 0; i < length; i++) {
		if ((c>>i)&1) s[length-1-i] = '1';
	}
	return s;
}

/*
 * Function to get the output of an S box Specified in the argument, taking the input number
 */
unsigned int Toydes::sbox(unsigned int in, unsigned int s[4][4]) {
	unsigned int tmp1 = ((in & 0x8) >> 2) + (in & 0x1); // First and last bits form the row
	unsigned int tmp2 = (in & 0x6) >> 1; // Second and third bits form the column
	return s[tmp1][tmp2];
}

/*
 * Helper function to do the permutaion from 4 bits to 4 bits
 */
unsigned int Toydes::p4t4(unsigned int in){
	unsigned int out = 0;
	std::vector<unsigned int> k = {2,4,3,1}; // Vector of new bit order, can be easily changed if needed
	for (unsigned int i = 0; i < 4; i++) {
		out|=((in>>(4-k[i]))&1)<<(3-i);
	}
	return out;
}

/*
 * Helper Functiont to reverse the above permutation from 4 bits to 4 bits
 */
unsigned int Toydes::r4t4(unsigned int in){
	unsigned int out = 0;
	std::vector<unsigned int> k = {4,1,3,2};
	for (unsigned int i = 0; i < 4; i++) {
		out|=((in>>(4-k[i]))&1)<<(3-i);
	}
	return out;
}

/*
 * Helper function to do the permutation expansion from 4 bits to 8 bits
 */
unsigned int Toydes::p4t8(unsigned int in) {
	unsigned int out = 0;
	std::vector<unsigned int> k = {4,1,2,3,2,3,4,1};
	for (unsigned int i = 0; i < 8; i++) {
		out|=((in>>(4-k[i]))&1)<<(7-i);
	}
	return out;
}

/*
 * Helper function to do the permutaion from 8 bits to 8 bits
 */
unsigned int Toydes::p8t8(unsigned int in) {
	unsigned int out = 0;
	std::vector<unsigned int> k = {2,6,3,1,4,8,5,7};
	for (unsigned int i = 0; i < 8; i++) {
		out|=((in>>(8-k[i]))&1)<<(7-i);
	}
	return out;
}


/*
 * Helper function to reverse the above permutaion from 8 bits to 8 bits
 */
unsigned int Toydes::r8t8(unsigned int in) {
	unsigned int out = 0;
	std::vector<unsigned int> k = {4,1,3,5,7,2,8,6};
	for (unsigned int i = 0; i < 8; i++) {
		out|=((in>>(8-k[i]))&1)<<(7-i);
	}
	return out;
}


/*
 * Helper function to do the permutaion from 10 bits to 8 bits
 */
unsigned int Toydes::p10t8(unsigned int in) {
	unsigned int out = 0;
	std::vector<unsigned int> k = {6,3,7,4,8,5,10,9};
	for (unsigned int i = 0; i < 8; i++) {
		out|=((in>>(10-k[i]))&1)<<(7-i);
	}
	return out;
}


/*
 * Helper function to do the permutaion from 10 bits to 10 bits
 */
unsigned int Toydes::p10t10(unsigned int in) {
	int out = 0;
	std::vector<unsigned int> k = {3,5,2,7,4,10,1,9,8,6};
	for (unsigned int i = 0; i < 10; i++) {
		out|=((in>>(10-k[i]))&1)<<(9-i);
	}
	return out;
}

/*
 * F function
 */
unsigned int Toydes::f(unsigned int in, unsigned int k) {
	unsigned int out = p4t8(in); // 4 bit to 8 bit expansion
	out = out^k; // Bitwise xor the permutated input and key
	unsigned int l = (out>>4)&0x0f; // Take the left half of the 8 bit string
	unsigned int r = out&0x0f; // And the right half
	l = sbox(l, s1); // Put the left half through s1
	r = sbox(r, s2); // And the right half through s2
	out = (r&0x3) + ((l&0x03)<<2); // Recombine the two halves
	return p4t4(out); // Return after reversing the permutation
}

/*
 * Helper function that takes the 10 bit key as input, and returns a vector of all the 8 bit keys to be used for each
 * round of the f function
 */
std::vector<unsigned int> Toydes::keyman(unsigned int key){
	unsigned int k = p10t10(key); // Perform 10 bit to 10 bit permutation
	unsigned int kl = (k>>5)&0x1f; // Left half of key
	unsigned int kr = k&0x1f; // Rigt half of key
	kl = ((kl<<1)&0x1e) + ((kl>>4)&0x1); // Rotate the left bits by one
	kr = ((kr<<1)&0x1e) + ((kr>>4)&0x1); // Rotate the right bits by one
	unsigned int k1 = (kl<<5) + kr; // Recombine rotated bits
	k1 = p10t8(k1); // k1 is the 10 bit to 8 bit permutation of this combination
	kl = ((kl<<1)&0x1e) + ((kl>>4)&0x1); // Rotate the left bits again by one
	kr = ((kr<<1)&0x1e) + ((kr>>4)&0x1); // Rotate the right bits again by one
	unsigned int k2 = (kl<<5) + kr; // Recombine
	k2 = p10t8(k2); // k2 is the 10 bit to 8 bit permutation of this combination
	std::vector<unsigned int> keys = {k1, k2}; // Return vector of keys
	return keys;
}
/*
std::vector<unsigned int> Toydes::r(std::vector<unsigned int> k) {
	std::vector<unsigned int> r(k.size());
	for (unsigned int i = 0; i < k.size(); i++) {
		r[k[i]-1] = i+1;
	}
	return r;
}
*/

/*
 * Public function to encrypt a single byte using an input key
 */
unsigned char Toydes::encryptByte(unsigned char msg, unsigned int key) {
	unsigned char cyp = p8t8(msg); // Perform the 8 bit to 8 bit permutaion on the message
	std::vector<unsigned int> keys = keyman(key); // Get the 8 bit keys to be used for the f function
	unsigned char l0 = (cyp>>4)&0x0f; // Take the left half of the initial permutated message
	unsigned char r0 = cyp&0x0f; // And the right half
	unsigned char l1 = l0^f(r0, keys[0]); // First iteration of left half from r0 and f function output
	unsigned char r1 = f(l1, keys[1])^r0; // First iteration of right half from l1 and f function output 
	cyp = r1 + (l1<<4); // Recombine halves
	cyp = r8t8(cyp); // Reverse the 8 bit permutation
	return cyp;
}

/*
 * Public function to decrypt a single byte using an input key
 */
unsigned char Toydes::decryptByte(unsigned char cyp, unsigned int key) {
	unsigned char msg = p8t8(cyp); // Perform 8 bit ot 8 bit permutation on the cypher text
	std::vector<unsigned int> keys = keyman(key); // Get the 8 bit keys to be used for the f function
	unsigned char l1 = (msg>>4)&0x0f; // Left half of the permutated cypher text
	unsigned char r1 = msg&0x0f; // Right half of the permutated cypher text
	unsigned char r0 = r1^f(l1, keys[1]); // Get the inital right half using l1 and f function output
	unsigned char l0 = f(r0, keys[0])^l1; // Get the inital left hald using r0 and f function output
	msg = r0 + (l0<<4); // Recombine the two halves
	msg = r8t8(msg); // Reverse the 8 bit permutation
	return msg;
}