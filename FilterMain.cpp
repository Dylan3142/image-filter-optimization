#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

class Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double
applyFilter(class Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;

  const int inputWidth = (input -> width) - 1;    //setting variables to avoid repeated pointer chasing and arithmetic 
  const int inputHeight = (input -> height) - 1; 
  const int filterSize = filter -> getSize();     //pre-initializing variables to avoid repeated function calls within loop
  const int filterDiv = filter -> getDivisor(); 


  if(filter->getSize() == 3)
  {
    const int f00 = filter->get(0,0), f01 = filter->get(0,1), f02 = filter->get(0,2);
    const int f10 = filter->get(1,0), f11 = filter->get(1,1), f12 = filter->get(1,2);
    const int f20 = filter->get(2,0), f21 = filter->get(2,1), f22 = filter->get(2,2); 

    for(int row = 1; row < inputHeight; row++)
    {
      for(int plane = 0; plane < 3; plane++)
      {
        int *outRow = &output->color[plane][row][0]; 
        const int *r0 = &input->color[plane][row - 1][0]; 
        const int *r1 = &input->color[plane][row][0];
        const int *r2 = &input->color[plane][row + 1][0]; 

        for(int col = 1; col < inputWidth; col++)
        {
          const int c = col; 
          int sum = 
            r0[c-1]*f00 + r0[c]*f01 + r0[c+1]*f02 + 
            r1[c-1]*f10 + r1[c]*f11 + r1[c+1]*f12 + 
            r2[c-1]*f20 + r2[c]*f21 + r2[c+1]*f22; 

          if(filterDiv != 1)
          {
            if(filterDiv & (filterDiv - 1) == 0)
            {
              const int s = __builtin_ctz(filterDiv);
              int bias = 0; 
              if (sum < 0)
              {
                bias = (1 << s) - 1; 
              }

              sum = (sum + bias) >> s; 
            }
            else
            {
              sum /= filterDiv;
            }
          }

          if(sum < 0) {sum = 0;}
          if (sum > 255) {sum = 255;}
          outRow[c] = sum; 
        }
      }
    }
  }
  else
  {
    for(int row = 1; row < inputHeight; row++) {
      for(int plane = 0; plane < 3; plane++) {

        int *outRow = &output->color[plane][row][0];

        vector<int> coeff(filterSize * filterSize);       
        for (int i = 0; i < filterSize; i++)
        {
          for(int j = 0; j < filterSize; j++)
          {
            coeff[i * filterSize + j] = filter->get(i, j); 
          }
        }                                                           


        for(int col = 1; col < inputWidth; col++) {
          int sum = 0;  

          for (int i = 0; i < filterSize; i++)
          {
            int* inRow = &input->color[plane][row + i - 1][0];

            for(int j = 0; j < filterSize; j++)
            {
              const int c = col + j - 1; 
              sum += inRow[c] * coeff[i * filterSize + j];
            }
          }

          if(filterDiv != 1)
          {
            if(filterDiv & (filterDiv - 1) == 0)
            {
              const int s = __builtin_ctz(filterDiv);
              int bias = 0; 
              if (sum < 0)
              {
                bias = (1 << s) - 1; 
              }

              sum = (sum + bias) >> s; 
            }
            else
            {
              sum /= filterDiv;
            }
          } 

          if (sum < 0) sum = 0; 
          if(sum > 255) sum = 255; 

          outRow[col] = sum;
        }
      }
    }
  }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}


/*
Optimization Log
OPT 1 : initialize constant variable for function calling and pointers outside of functions
        to avoid repeated function calls and pointer dereferencing (input->width, input->height, 
        filterDiv = filter->getDiv() 
        Performance Boost - minimal 

OPT 2 : (lines 116 - 130) accumulate sum into local variable to avoid repeated array indexing
        and pointer dereferencing. instead of doing output->color[...]+= input->color[...] * filter->get(i,j);
        we just put that into a sum variable outside of the inner 2 loops that calculate
        output color. this also allows sum to be stored in a register for faster accessing. 
        after clamping (making sure sum is between 0 and 255), we write it back to the ouput->color
        just once instead of multiple times. 

OPT 3 : (lines 119 - 125) intead of dereferencing input->color[plane][row][col] every tap
        in the most inner for loop (most iterations), declare the ith row and find the col 
        inside the loop instead, and use the row variable created outside of that inner loop
        to add to sum. Fewer "load effective address", and few pointer chasing. Pre compute inRow
        once per i instead of once per i and once per j, and just find caluclaute column in inner 
        column iterating loop. 

OPT 4 : (lines 121 - 131) every iteration we were doing more pointer chasing and function calling
        within the intermost loop with filter->get(i, j). We can load a variable before going into 
        the function so it has an easier time tapping into the array. Sometimes gives coeffRow a 
        register which makes it easier to load on memory. 

OPT 5 : (line 140) initialize a output row pointer before entering into the inner most loop to avoid
        excesive address loading and array indexing, allows for simple base + index math inside loop 
        and allows us to cache outRow so its easier to access in memory 

OPT 6 : (lines 111 - 143) rearrange loops to have col be the inner most loop instead of plane as the inner
        most loop. Because our array is laid out as   int color[MAX_COLORS][MAX_DIM][MAX_DIM]; 
        or color[plane][row][col], if we match that order with the order of the loops, it allows us to 
        linearly access and manipulate the array because col vaires the fastest in memory (its being changed the most, 
        so having it as the inner most loop allows the next column index to be literally right next to the previus
        index in memory, linearly walking through the columns. We have plane and then row bcause plane is the least
        fastest changing index, only going up to 3, while row and column go up much more than that. This same 
        linear walking applies to the rows index as well. Essentially just following the why the array is initialized to 
        begin with. Temporal locality and spatial locality in action
  
OPT 7 : (lines 117 - 123) loading coefficients outside of the col loop helps to reduce number of times 
        filter->get is called, which makes for less pointer chasing and repeated function calling. instead
        we load the filter coefficiants for that column before entering the hot inner loop, and also allows
        for easier arithmetic inside hot loop. essentially when dealing with nested loops, try and have the most
        inner loops do the most easy, memory light work because that loop is being iterated through the most. 
        Loading coefficients and variables outside of loops is an example of temproal locality, so that coefficient / 
        variable is stored in cache for easier access because it keeps getting reused, and cache keeps the data
        that was most recently used 

OPT 8 : () Kept the outer row, plane, col loops, but unrolled the inner 2 loops that iterate over the filter,
           calculating the sum. Since we know that all of the filters are a 3x3 grid, we essentially dont need 
           the loops because if we already know the size, we can initliaze constants for filter->get() and for 
           each row. We replaced those inner i/j loops with 9 explicit multiplication / addition constants, which 
           is the sum variable. We also hoisted coefficients f00, f01, f02, etc, out of the loop. This allows the 
           compiler to avoid branching jumps for the loop, like checking if its still in bounds and adding iterations 
           to i or j. No LEA (load effective address) arithmetic for input->color, no loading of filter->get(), and no
           repeated addition to the sum variable for each tap of the loop. Unrolling the loop gives the compiler less 
           instructions, so faster perfromance. Previous code recomputes row/col offsets every tap of the loop. Putting all
           of these additions and indexes and offsets into coefficients allows the compiler to put those values in 
           registers, which are much quicker than pulling from memory. Compilers perfer straight-line, simple step by 
           step math instead of dealing with loops, conditional branches, pointer chasing, funciton calling, etc. The math
           is still the same, its just everything around the math that makes it so much faster because loops have so much 
           overhead when used. again like conditional branching, iteration calculating, and effective address loading. getting 
           rid of this overhead while ALSO putting those effective addresses that we are calculating into constants, allows
           the compiler to give registers to each of those values / addresses. 

OPT 9 : Replace the sum /= filterDiv divison in the hot loop (its the most expensive operation) with
        some cases. If filterDiv is 1, then there is no reason to divide, so we can skip the division entirely.
        If the filterDiv is a factor of 2(used builtin to do log2 of the filterDiv), then we can use bitwise shifting to divide the numbers, which is much 
        cheaper than regular integer division. Otherwise, we fall back to normal sum /= filterDiv. 
      
OPT 10 : Compiler optimization, -O3 and native to tune it correctly for my laptop specs. Use
         make clean
         make CXXFLAGS="-O3 -march=native -mtune=native -fomit-frame-pointer -funroll-loops -DNDEBUG -Wall"
         make judge

         changed makefile for github grading 
*/