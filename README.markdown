# llvm2KITTeL

llvm2KITTeL is a converter from LLVM's intermediate representation
into a format that can be handled by the automatic termination prover
[KITTeL](https://github.com/s-falke/kittel-koat).

This fork is a further extension to translate the LLVM-IR into the 
T2 file format(.t2) to be handled by the automatic temporal property
verifier [T2](https://github.com/mmjb/T2).


##T2 Extension 

After following the INSTALL instructions, running the following commands below
will allow you to extract a .t2 file. From the build directory:

$ clang -Wall -Wextra -c -emit-llvm -O0 <INPUT> -o <INPUT>.bc

$ ./llvm2kittel --dump-ll --no-slicing --eager-inline --t2 <INPUT>.bc > <INPUT>.t2


## Papers

Stephan Falke, Deepak Kapur, Carsten Sinz:
[Termination Analysis of C Programs Using Compiler Intermediate Languages](http://dx.doi.org/10.4230/LIPIcs.RTA.2011.41).
RTA 2011: 41-50

Stephan Falke, Deepak Kapur, Carsten Sinz:
[Termination Analysis of Imperative Programs Using Bitvector Arithmetic](http://dx.doi.org/10.1007/978-3-642-27705-4_21).
VSTTE 2012: 261-277
