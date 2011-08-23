/*
 * include the headers required by the generated cpp code
 */
%{
#include "core/Processor.h"
%}

//make sure to import the image_pool as it is 
//referenced by the Processor java generated
//class
%typemap(javaimports) Processor "

/** Processor - for processing images that are stored in an image pool
*/"
%include "core/Processor.h"
