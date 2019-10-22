#ifndef _POISSON_H_
#define _POISSON_H_

//===========================================================================
//=  Function to generate Poisson distributed random variables              =
//=    - Input: alpha and N, regenerate forces regeneration of zipf-dist    =
//=    - Output: Returns with Zipf distributed random variable              =
//===========================================================================
unsigned int poisson(double mu);
void poisson_init(void);
void poisson_fini(void);

#endif /* _POISSON_H_ */
