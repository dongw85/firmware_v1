/* Copyright 2014, ACSE & CADIEEL
 *    ACSE   : http://www.sase.com.ar/asociacion-civil-sistemas-embebidos/ciaa/
 *    CADIEEL: http://www.cadieel.org.ar
 * All rights reserved.
 *
 *    or
 *
 * Copyright 2014, Your Name <youremail@domain.com>
 * All rights reserved.
 *
 *    or
 *
 * Copyright 2014, ACSE & CADIEEL & Your Name <youremail@domain.com
 *    ACSE   : http://www.sase.com.ar/asociacion-civil-sistemas-embebidos/ciaa/
 *    CADIEEL: http://www.cadieel.org.ar
 * All rights reserved.
 *
 * This file is part of CIAA Firmware.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** \brief Short description of this file
 **
 ** Long description of this file
 **
 **/

/** \addtogroup CIAA_Firmware CIAA Firmware
 ** @{ */
/** \addtogroup Template Template to start a new module
 ** @{ */

/*
 * Initials     Name
 * ---------------------------
 *
 */

/*
 * modification history (new versions first)
 * -----------------------------------------------------------
 * yyyymmdd v0.0.1 initials initial version
 */

/*==================[inclusions]=============================================*/
#include "speechrec_pdm2pcm.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data declaration]==============================*/

/** \brief
 *
 */
static int32_t Sigma1, Sigma2, Sigma3, Sigma4, Sigma5;
//static int16_t Sigma1, Sigma2, Sigma3, Sigma4, Sigma5;

/** \brief
 *
 */
static int32_t Delta1, Delta2, Delta3, Delta4;
//static int16_t Delta1, Delta2, Delta3, Delta4;

/** \brief
 *
 */
static int32_t Result;
//static int16_t Result;

/** \brief
 *
 */
static int32_t OldDelta1, OldDelta2, OldDelta3, OldDelta4, OldSigma5;
//static int16_t OldDelta1, OldDelta2, OldDelta3, OldDelta4, OldSigma5;

/** \brief
 *
 */
static int16_t memCICout[MEMCICSIZE];

/** \brief Q15 FIR decimator 1 structure
 *
 */
static arm_fir_decimate_instance_q15 S1;

/** \brief Q15 FIR decimator 2 structure
 *
 */
static arm_fir_decimate_instance_q15 S2;

/** \brief FIR decimator 1 state buffer
 *
 */
static int16_t StateBuff1[NCOEFFS1 + MEMCICSIZE - 1];

/** \brief FIR decimator 2 state buffer
 *
 */
static int16_t StateBuff2[NCOEFFS2 + MEMFIR1SIZE - 1];

/** \brief Memory buffer for FIR decimator 1
 */
static int16_t memFIR1out[MEMFIR1SIZE];

///** \brief PCM memory buffer for window 1
// * One PCM buffer corresponds to two PDM memory buffers, because of DMA transference size limitations
// * Equivalent to two consecutive memFIR2out buffers, each of size MEMFIR2SIZE
// */
//extern int16_t memPCM1out[MEMPCMSIZE];
//
///** \brief PCM memory buffer for window 2
// * One PCM buffer corresponds to two PDM memory buffers, because of DMA transference size limitations
// * Equivalent to two consecutive memFIR2out buffers, each of size MEMFIR2SIZE
// */
//extern int16_t memPCM2out[MEMPCMSIZE];

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/** \brief Coefficients for FIR filter decimator 1
 * Low-pass FIR equiripple
 * Order: 31
 * fs: 64 KHz
 * fpass: 12 KHz
 * fstop: 16 KHz
 * pass band: 0 dB
 * stop band: -38 dB
 */
static int16_t FIR1coeffs[NCOEFFS1] = {
       96,    309,   -120,   -357,    -46,    548,    328,   -668,   -817,
      621,   1571,   -210,  -2801,  -1173,   6070,  13215,  13215,   6070,
    -1173,  -2801,   -210,   1571,    621,   -817,   -668,    328,    548,
      -46,   -357,   -120,    309,     96
};

/** \brief Coeficients for FIR filter decimator 2
 * Low-pass FIR equiripple
 * Order: 31
 * fs: 32 KHz
 * fpass: 7 KHz
 * fstop: 8 KHz
 * pass band: +/-1 dB
 * stop band: -24 dB
 */
static int16_t FIR2coeffs[NCOEFFS2] = {
     -628,    996,    820,   -117,   -612,    235,    952,    -44,  -1263,
     -219,   1751,    756,  -2640,  -2124,   5563,  14005,  14005,   5563,
    -2124,  -2640,    756,   1751,   -219,  -1263,    -44,    952,    235,
     -612,   -117,    820,    996,   -628
};

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

static void check_freqs()
{
	if((BITRATE/DECIM_FACT_CIC/DECIM_FACT_FIR) != (SAMPLE_RATE/1000))
	{
		while(1){} //indicar error
	}
}

/* Extraction of each bit from the received PDM data in the buffer */
static int8_t GetPDMbit(uint8_t *PDMbuff,uint8_t word, uint8_t bit)
{
	uint16_t current_word;
	int8_t current_bit;

	current_word = PDMbuff[word*(DECIM_FACT_CIC/8)];

	if (DECIM_FACT_CIC == 16)
		current_word |= PDMbuff[word*(DECIM_FACT_CIC/8) + 1] << 8;

	current_word >>= bit;
	current_bit = current_word & 0x1;
	current_bit = current_bit * 2;
	current_bit = current_bit - 1;

	return current_bit;

}

/* 5th order CIC filter */
static void CIC_filter(uint8_t *PDMbuff)
{
   uint16_t i,j;

for(j=0;j<MEMCICSIZE;j++)

{
	OldSigma5 = Sigma5;
	OldDelta1 = Delta1;
	OldDelta2 = Delta2;
	OldDelta3 = Delta3;
	OldDelta4 = Delta4;

	//Sigma operations for the PDM data from Microphone
		for(i=0; i<DECIM_FACT_CIC; i++){
		   Sigma1 += GetPDMbit(PDMbuff,j,i);
		   Sigma1 += 1;
		   Sigma2 += Sigma1;
		   Sigma3 += Sigma2;
		   Sigma4 += Sigma3;
		   Sigma5 += Sigma4;
		}

	//Delta operations for the PDM data from Microphone
	Delta1 = Sigma5 - OldSigma5;
	Delta2 = Delta1 - OldDelta1;
	Delta3 = Delta2 - OldDelta2;
	Delta4 = Delta3 - OldDelta3;
	Result = Delta4 - OldDelta4;

/* Framed data from microphone to be sent for FIR decimation is stored in Result.
Since the operation is 5 stage divided by 16 decimation this field can grow up to
21 bits. */
	memCICout[j] = (int16_t) (Result>>SHIFT_RES);


	}

}


static void FIR_decimator(int16_t *PCMbuff)
{
	arm_status ret;

	ret = arm_fir_decimate_init_q15(	&S1,
								NCOEFFS1,
								(uint8_t) DECIM_FACT_FIR1,
								FIR1coeffs,
								StateBuff1,
								(uint32_t) MEMCICSIZE);

	if (ret == ARM_MATH_LENGTH_ERROR)
		while(1){}

	ret = arm_fir_decimate_init_q15(	&S2,
								NCOEFFS2,
								(uint8_t) DECIM_FACT_FIR2,
								FIR2coeffs,
								StateBuff2,
								(uint32_t) MEMFIR1SIZE);

	if (ret == ARM_MATH_LENGTH_ERROR)
		while(1){}

	arm_fir_decimate_q15	(	&S1,
								memCICout,
								memFIR1out,
								(uint32_t) MEMCICSIZE);

	arm_fir_decimate_q15	(	&S2,
								memFIR1out,
								PCMbuff,
								(uint32_t) MEMFIR1SIZE);
}


/*==================[external functions definition]==========================*/

extern void PDM2PCM_Init(void)
{

   check_freqs();

   Sigma1 = 0;
   Sigma2 = 0;
   Sigma3 = 0;
   Sigma4 = 0;
   Sigma5 = 0;
   Delta1 = 0;
   Delta2 = 0;
   Delta3 = 0;
   Delta4 = 0;
   Result = 0;
   OldDelta1 = 0;
   OldDelta2 = 0;
   OldDelta3 = 0;
   OldDelta4 = 0;
   OldSigma5 = 0;
}

extern void PDM2PCM(uint8_t *PDMbuff, int16_t *PCMbuff) // cuando llamo a esta funcion tengo que asegurarme de pasar el inicio o la mitad de memPCM1out o memPCM2out
{

	CIC_filter(PDMbuff);

	FIR_decimator(PCMbuff);

}



/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/