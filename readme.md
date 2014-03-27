CS3014 Lab 2: The Mandelbrot Set
Wednesday 19th March 2014

A Mandelbrot set refers to a set of complex numbers that correspond to
points in a plane. The Mandelbrot set is computed using a formula:
Z(0) = 0
Z(n+1) = (Z(n))^2 + C

The formula is applied recursively, and the result is compared to
a given bound. If the result is within the bound, then C is a member
of the Mandelbrot set. Otherwise it is not. When members of the set
are computed for a plane (where x and y co-ordinates in the plane
represent the real and imaginary parts of the number C), the result
is some very nice-looking pictures.

As part of this project, you are provided with an existing piece of
code that implements a Mandelbrot set. The code can be found here:
https://www.cs.tcd.ie/David.Gregg/cs3014/labs/Mandelbrot.tar.gz

The goal of the project is to speed up this code using the techniques
you have learned in lectures. That is you should vectorize the code
using SSE, and parallelise the code using OpenMP. You can also apply
other, machine-level optimizations to speed up the code. But you must
implement the same algorithm as shown here. You may not use mathematics
or other techniques to compute the Mandelbrot set in a completely
different way.

The project uses the open source Simple Directmedia Layer library (SDL)
to visualize the Mandelbrot set. Thus, to build the software you will
need access to a machine where that software is installed. SDL is
already installed on stoker.cs.tcd.ie, and the Mandelbrot software
should build out of the box on that machine.

On Windows/Cygwin you may need to download:
http://www.libsdl.org/release/SDL-devel-1.2.13-mingw32.tar.gz.

Unpack the archive and exectute the command "make native" while in a
cygwin environment to install the SDL development kit. The project
should then compile using "make" as on Linux. Note that timings should
be made on stoker, so it probably makes sense to also do your
development on that machine.

The Mandelbrot program causes a window to appear for the visualization.
In order to run the program, you need to be running from and environment
that allows an X-windows window to pop up. If you login to stoker from
any Unix machine, this should work transparently if you simply login in
the usual way:
    ssh -X bobby@stoker.cs.tcd.ie

Windows machines do not have an X-server installed by default. Therefore
you need to use some sort of X-windows software. On my laptop I use X-windows
under Cygwin and type:
    ssh -Y bobby@stoker.cs.tcd.ie

For this lab you should submit two things: (1) You should submit a copy of
the code, which should include comments and all the things we normally expect
from a program. (2) You should also submit a 2-3 page report describing
the optimization strategies you tried; what worked and what didn't; running
times of your code using various strategies; and a general discussion of the
parallelization, SIMDization, and other optimzations.

The lab should be submitted by 11.59pm on Wednesday 2nd April. Please
submit your program by email to me (David). Please attach your program
as C plain text file(s) and your report as pdf. Your report should
clearly state your name (yes, in previous years I have received
reports with no names).

The subject of your email should be in the following format:
PANDA 2014 lab 2: <your name>
