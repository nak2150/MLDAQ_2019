/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
Description:
    HLS pragmas can be used to optimize the design : improve throughput, reduce latency and 
    device resource utilization of the resulting RTL code
    This is a wrapper to be used with an hls4ml project to enable proper handling by SDAccel
*******************************************************************************/

#define PROJ_HDR <MYPROJ.h>
#define PROJ_CPP <MYPROJ.cpp>

#include PROJ_HDR
#include PROJ_CPP
#include "kernel_params.h"

#include "ap_int.h"
#include "ap_fixed.h"
#include "hls_stream.h"

/*
    HLS4ML Kernel Implementation 
    Arguments:
        in    (input)     --> Input Vector
        out   (output)    --> Output Vector
   */
/*static void makeIn(const bigdata_t* in_arr, hls::stream<input_t>& inStream) {
    unsigned int readi[IN_STREAM_LEN];
    for(int i0 = 0; i0 < IN_STREAM_LEN; i0++) { 
      #pragma HLS LOOP UNROLL
      readi[i0] = i0/COMPRESSION;
    }
    for(int i0 = 0; i0 < IN_STREAM_LEN; i0++) { 
      input_t tmpi;
      tmpi[0].range(15,0) = in_arr[readi[i0]].range(16*((i0%COMPRESSION)+1)-1,16*(i0%COMPRESSION));
      inStream << tmpi;
    }
}

static void makeOut(bigdata_t* out_arr, hls::stream<result_t>& outStream) {
    //all this is not yet very general (e.g. it breaks if the output is wider than bigdata_t and is required to loop)
    unsigned int writei[OUT_STREAM_LEN];
    for(int i0 = 0; i0 < OUT_STREAM_LEN; i0++) { 
      #pragma HLS LOOP UNROLL
      if (i0*DATA_SIZE_OUT%COMPRESSION==COMPRESSION-1)
        writei[i0] = 3; //write 3x + reset
      else if (i0*DATA_SIZE_OUT%COMPRESSION==COMPRESSION-2)
        writei[i0] = 2; //write 2x + reset + write
      else if (i0*DATA_SIZE_OUT%COMPRESSION==COMPRESSION-3)
        writei[i0] = 1; //write + reset + write 2x
      else
        writei[i0] = 0; //write 3x
    }
    bigdata_t bigtmp;
    unsigned int outi = 0;
    for(int i0 = 0; i0 < OUT_STREAM_LEN; i0++) { 
      result_t tmpo = outStream.read();
      bigtmp.range(48*((i0%COMPRESSION)+1)-33,48*(i0%COMPRESSION)) = tmpo[0].range(15,0);
      if (writei[i0]==1) {
        out_arr[outi] = bigtmp;
        outi++;
        bigtmp = 0;
      }
      bigtmp.range(48*((i0%COMPRESSION)+1)-17,48*(i0%COMPRESSION)+16) = tmpo[1].range(15,0);
      if (writei[i0]==2) {
        out_arr[outi] = bigtmp;
        outi++;
        bigtmp = 0;
      }
      bigtmp.range(48*((i0%COMPRESSION)+1)-1,48*(i0%COMPRESSION)+32) = tmpo[2].range(15,0);
      if (writei[i0]==3) {
        out_arr[outi] = bigtmp;
        outi++;
        bigtmp = 0;
      }
    }
    out_arr[0] = bigtmp;
    
}*/

extern "C" {

void alveo_hls4ml(
        const bigdata_t *in, // Read-Only Vector
        bigdata_t *out       // Output Result
        )
{
// SDAccel kernel must have one and only one s_axilite interface which will be used by host application to configure the kernel.
// Here bundle control is defined which is s_axilite interface and associated with all the arguments (in and out),
// control interface must also be associated with "return".
// All the global memory access arguments must be associated to one m_axi(AXI Master Interface). Here all two arguments(in, out) are 
// associated to bundle gmem which means that a AXI master interface named "gmem" will be created in Kernel and all these variables will be 
// accessing global memory through this interface.
// Multiple interfaces can also be created based on the requirements. For example when multiple memory accessing arguments need access to
// global memory simultaneously, user can create multiple master interfaces and can connect to different arguments.
#pragma HLS INTERFACE m_axi port=in  offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem
#pragma HLS INTERFACE s_axilite port=in   bundle=control
#pragma HLS INTERFACE s_axilite port=out  bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
    //necessary for hls4ml kernel, not used
    #pragma HLS DATAFLOW

    unsigned short const_size_in_1, const_size_out_1;

     bigdata_t in_bigbuf[BIGSTREAMSIZE_IN*STREAMSIZE];
     bigdata_t out_bigbuf[BIGSTREAMSIZE_OUT*STREAMSIZE];

     hls::stream<input_t> in_buf[STREAMSIZE];
     hls::stream<result_t> out_buf[STREAMSIZE];
#pragma HLS ARRAY_PARTITION   variable=in_buf  complete dim=0
     #pragma HLS ARRAY_PARTITION   variable=out_buf complete dim=0
     for (unsigned int istream = 0; istream < STREAMSIZE; istream++) {
       #pragma HLS STREAM   variable=in_buf[istream]  depth=1000
       #pragma HLS STREAM   variable=out_buf[istream] depth=1
     }

  for (int i = 0; i < STREAMSIZE*BIGSTREAMSIZE_IN; i++) {
       #pragma HLS LOOP UNROLL
       in_bigbuf[i] = in[i];
     }
     for (int i = 0; i < STREAMSIZE; i++) {
       std::cout<<"------------------"<<std::endl;
       for(int i0 = 0; i0 < IN_STREAM_LEN; i0++) { 
 	  for(int i1 = 0; i1 < DATA_SIZE_IN; i1++) { 
 	    #pragma HLS UNROLL
 	    int ib = (i0*DATA_SIZE_IN+i1)%COMPRESSION;
 	    int ia = i*BIGSTREAMSIZE_IN+( (i0*DATA_SIZE_IN+i1)/COMPRESSION);
 	    input_t tmp;
 	    tmp[i1].range(15,0) = in_bigbuf[ia].range(16*(ib+1)-1,16*ib);
 	    in_buf[i].write(tmp);
 	    std::cout<<"writing to ["<<i<<"]["<<i1<<"]"<<std::endl;
 	  }
 	}
       std::cout<<"inf start"<<std::endl;
       hls4ml: MYPROJ(in_buf[i],out_buf[i],const_size_in_1, const_size_out_1);
 	std::cout<<"inf end"<<std::endl;
       bigdata_t tmp;
       result_t tmp_small;
       for(int i0 = 0; i0 < OUT_STREAM_LEN; i0++) { 
        tmp_small = out_buf[i].read();
        for(int i1 = 0; i1 < DATA_SIZE_OUT; i1++) {
 	  #pragma HLS UNROLL
         int ib = (i0*DATA_SIZE_OUT+i1)%COMPRESSION;
         int ia = i*BIGSTREAMSIZE_OUT+((i0*DATA_SIZE_OUT+i1)/COMPRESSION);
         std::cout<<"reading from ["<<i<<"]["<<i1<<"]"<<std::endl;
         tmp((ib+1)*16-1,(ib)*16) = tmp_small[i1].range(15,0);
         if (((i0*DATA_SIZE_OUT+i1)%COMPRESSION == COMPRESSION-1) || (i0 == OUT_STREAM_LEN-1 && i1 == DATA_SIZE_OUT-1)) out_bigbuf[ia] = tmp;
        }
       }
     }

  //place output into DDR
       for (int i = 0; i < STREAMSIZE*BIGSTREAMSIZE_OUT; i++) {
              #pragma HLS LOOP UNROLL
                     out[i] = out_bigbuf[i];
        }

  }
}
