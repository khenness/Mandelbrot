
/*
 * CS3014 Mandelbrot Project
 * 
 * Using techniques we've covered in class, accelerate the rendering of
 * the M set.
 * 
 * Hints
 * 
 * 1) Vectorize
 * 2) Use threads
 * 3) Load Balance
 * 4) Profile and Optimise
 * 
 * Potential FAQ.
 *
 * Q1) Why when I zoom in far while palying with the code, why does the image begin to render all blocky?
 * A1) In order to render at increasing depths we must use increasingly higher precision floats
 * 	   We quickly run out of precision with 32 bits floats. Change all floats to doubles if you want
 * 	   dive deeper. Eventually you will however run out of precision again and need to integrate an
 * 	   infinite precision math library or use other techniques.
 * 
 * Q2) Why do some frames render much faster than others?
 * A2) Frames with a lot of black, i.e, frames showing a lot of set M, show pixels that ran until the 
 *     maximum number of iterations was reached before bailout. This means more CPU time was consumed
 */

/*Change these to try different paralellization methods*/
const bool OMP_ON = false;
const int PTHREADS_ON = 1;
int NUM_PTHREADS = 300;  //starting number

const int SSE_ON = 1;


#include <iostream>
#include <cmath>

// header file for sleep system call
#include <unistd.h> 

#define TIMING
#ifdef TIMING
#include <sys/time.h>
#endif

// OpenMP
#include <omp.h>

//SSE
#include <xmmintrin.h>

//pthreads
#include <pthread.h> 


#include "Screen.h"


/*
 * You can't change these values to accelerate the rendering.
 * Feel free to play with them to render different images though.
 */
const int 	MAX_ITS = 1000;			//Max Iterations before we assume the point will not escape
const int 	HXRES = 700; 			// horizontal resolution	
const int 	HYRES = 700;			// vertical resolution
const int 	MAX_DEPTH = 40;		// max depth of zoom
const float ZOOM_FACTOR = 1.02;		// zoom between each frame
int screendata [HXRES][HYRES] ={0};
/* Change these to zoom into different parts of the image */
const float PX = -0.702295281061;	// Centre point we'll zoom on - Real component
const float PY = +0.350220783400;	// Imaginary component



/*
 * The palette. Modifying this can produce some really interesting renders.
 * The colours are arranged R1,G1,B1, R2, G2, B2, R3.... etc.
 * RGB values are 0 to 255 with 0 being darkest and 255 brightest
 * 0,0,0 is black
 * 255,255,255 is white
 * 255,0,0 is bright red
 */
unsigned char pal[]={
	255,180,4,
	240,156,4,
	220,124,4,
	156,71,4,
	72,20,4,
	251,180,4,
	180,74,4,
	180,70,4,
	164,91,4,
	100,28,4,
	191,82,4,
	47,5,4,
	138,39,4,
	81,27,4,
	192,89,4,
	61,27,4,
	216,148,4,
	71,14,4,
	142,48,4,
	196,102,4,
	58,9,4,
	132,45,4,
	95,15,4,
	92,21,4,
	166,59,4,
	244,178,4,
	194,121,4,
	120,41,4,
	53,14,4,
	80,15,4,
	23,3,4,
	249,204,4,
	97,25,4,
	124,30,4,
	151,57,4,
	104,36,4,
	239,171,4,
	131,57,4,
	111,23,4,
	4,2,4};
const int PAL_SIZE = 40;  //Number of entries in the palette 



/*
 * Return true if the point cx,cy is a member of set M.
 * iterations is set to the number of iterations until escape.
 */
bool member(float cx, float cy, int& iterations)
{
        float x = 0.0;
        float y = 0.0;
        iterations = 0;
      while ((x*x + y*y < (4)) && (iterations < MAX_ITS)) {
                float xtemp = x*x - y*y + cx;
                y = 2*x*y + cy;
                x = xtemp;
                iterations++;
        }



        return (iterations == MAX_ITS);
}




//Member function rewritten with SSE - Takes in 4 points at a time and returns the number of iterations.
__m128 sse_member(__m128 cx, __m128 cy)
{
   //initialize variables
   __m128 x = _mm_set1_ps(0.0);
   __m128 y = _mm_set1_ps(0.0);
   __m128 iterations = _mm_set1_ps(0.0);
   __m128 iterations_values;
   __m128 mask = _mm_set1_ps(1.0);
   __m128 temp;
   int iterations_count = 0;
   while( (_mm_movemask_ps(iterations_values =(
		  // x^2 + y^2 < 4?
	   	  _mm_cmplt_ps( 
		  // x^2 + y^2
		   _mm_add_ps( _mm_mul_ps(x,x),_mm_mul_ps(y,y)), 
	           _mm_set1_ps(4.0) )))
	   	)!= 0&&
		iterations_count < MAX_ITS)
   {
      iterations = _mm_add_ps(iterations, _mm_and_ps(iterations_values, mask)); 

      // temp =  x^2 - y^2 + cx
      temp = _mm_add_ps( _mm_sub_ps( _mm_mul_ps(x,x), _mm_mul_ps(y,y)), cx);
      // y = 2*x*y + cy
      y = _mm_add_ps( _mm_mul_ps( _mm_mul_ps(x,y), _mm_set1_ps(2.0)), cy);

      x = temp;
      iterations_count++;
    
   }

   return iterations;
}





//Pthreads require you to use a struct to pass parameters.
struct PThreadParams {
   //struct semaphore mySemaphore;
   void * threadid;
   int SubSquareSide;
   int xpos;
   int ypos;
   float m; //magnification
   Screen * screen;
   unsigned char * pal;
};



/*A Pthread function that draws a single square on the grid*/
void *PThreadFunction(void *context) {
   //read parameters
   struct PThreadParams *readParams = context;
   int SubSquareSide = readParams->SubSquareSide;
   int threadid = readParams->threadid;
   int xpos = readParams->xpos;
   int ypos = readParams->ypos;
   float m = readParams->m;
   Screen * screen = readParams->screen;
   unsigned char * pal = readParams->pal;


   int count =0;

   for (int hy=ypos; hy<(ypos+SubSquareSide); hy++) {

      for (int hx=xpos; hx<(xpos+SubSquareSide); hx++) {

           int iterations;
            // Translate pixel coordinates to complex plane coordinates centred on PX, PY

               //potential optimization

               float cx = ((((float)hx/(float)HXRES) -0.5 + (PX/(4.0/m)))*(4.0f/m));
               float cy = ((((float)hy/(float)HYRES) -0.5 + (PY/(4.0/m)))*(4.0f/m));
               if(SSE_ON == 1){
	         __m128 vector_cx = _mm_set1_ps(cx);
	          __m128 vector_cy = _mm_set1_ps(cy);  //my

                 float temp_member[4];
                 _mm_store_ps(temp_member, sse_member(vector_cx,vector_cy));

                  for(int i = 0; i < 4; i++){
                  if(temp_member[i] != MAX_ITS){
            		/* Point is not a member, colour based on number of iterations before escape */
			   int col =((int)temp_member[i]%40) - 1;
			   int b = col*3;
		       	   screen->putpixel(hx+i, hy, pal[b], pal[b+1], pal[b+2]);
          	   }else{
		        /* Point is a member, colour it black */
	         	screen->putpixel(hx+i, hy, 0, 0, 0);
              	   }
                 }
              }else{

                  if (!member(cx, cy, iterations)) {
                 // Point is not a member, colour based on number of iterations before escape 

                   count = count+iterations;

                   int i=(iterations%40) - 1;
                   int b = i*3;

                    screen->putpixel(hx, hy, pal[b], pal[b+1], pal[b+2]);
                 }else {
                    // Point is a member, colour it black 
                       screen->putpixel(hx, hy, 0, 0, 0);
                 }
               }
           }
         }
	pthread_exit(NULL); 
} 


int main()
{

	int hx=0;
        int hy=0;

	float m=1.0; /* initial  magnification		*/

	/* Create a screen to render to */
	Screen *screen;
	screen = new Screen(HXRES, HYRES);


	int depth=0;

#ifdef TIMING
  struct timeval start_time;
  struct timeval stop_time;
  long long total_time = 0;
#endif
        int threadcount =0;

	while (depth < MAX_DEPTH) {
#ifdef TIMING
	        /* record starting time */
	        gettimeofday(&start_time, NULL);
#endif


                 if(PTHREADS_ON == 1){

                    int rc;
                   double SubSquareSide =  ceil(sqrt((HXRES*HYRES)/(NUM_PTHREADS)));
                        /*As the Mandelbrot set gets harder to compute, we give it more threads. (The squares in the grid get smaller)*/
                         if(depth %20 == 0  ){
                            NUM_PTHREADS = NUM_PTHREADS +300; // we add on 300 threads for each multiple of 20.
                            SubSquareSide =  ceil(sqrt((HXRES*HYRES)/(NUM_PTHREADS)));
                         }
                         pthread_t threads[NUM_PTHREADS];


                  /*Create a pthread for each square on the grid*/
                   int threadcount =0;
                   while(hy<HYRES-SubSquareSide){
                      while(hx<HXRES-SubSquareSide){
                           
                            //pthreads require you to make a struct to pass parameters
                            struct PThreadParams readParams;
                            readParams.SubSquareSide = SubSquareSide;
                            int xpos = hx;
                            int ypos = hy;
                            readParams.xpos = xpos;
                            readParams.ypos = ypos;
                            readParams.m =m;
                            readParams.screen = screen;
                            readParams.pal = pal;

                            //create the threads. (See PthreadFunction for their behavior)
                            rc = pthread_create(&threads[threadcount],NULL,PThreadFunction,&readParams);
                            threadcount++;
                            if (rc) {
                               printf("ERROR return code from pthread_create(): %d\n",rc);
                               exit(-1);
                            }
                         hx= hx+SubSquareSide;
                      }
                      hy= hy+SubSquareSide;
                      hx =0;

                   }

                    //wait for all the pthreads to finish
                    for(int i=0;i<threadcount;i++) {
                         pthread_join( threads[i], NULL);
                    }
                    hx =0;
                   hy =0;

                 }else{



                  //This is the comparison version using OMP

                    #pragma omp parallel for collapse(2) 

		    for (hy=0; hy<HYRES; hy++) {
	               for (hx=0; hx<HXRES; hx++) {
		          int iterations;

		    	   /*
		            * Translate pixel coordinates to complex plane coordinates centred
		            * on PX, PY
		            */

                             //potential optimization
		            float cx = ((((float)hx/(float)HXRES) -0.5 + (PX/(4.0/m)))*(4.0f/m));
			    float cy = ((((float)hy/(float)HYRES) -0.5 + (PY/(4.0/m)))*(4.0f/m));
		            if (!member(cx, cy, iterations)) {
				/* Point is not a member, colour based on number of iterations before escape */
				int i=(iterations%40) - 1;
				int b = i*3;

                                //potential optimization
			        screen->putpixel(hx, hy, pal[b], pal[b+1], pal[b+2]);
			    }else {
			        /* Point is a member, colour it black */
				screen->putpixel(hx, hy, 0, 0, 0);
		            }
			}
		    }
		}
#ifdef TIMING
		gettimeofday(&stop_time, NULL);
		total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
#endif
		/* Show the rendered image on the screen */
		screen->flip();
		std::cout << "Render done " << depth++ << " " << m << std::endl;

		/* Zoom in */
		m *= ZOOM_FACTOR;
	}
#ifdef TIMING

	std::cout << "Total executing time " << total_time << " microseconds\n";
#endif
	std::cout << "Clean Exit"<< std::endl;

}
