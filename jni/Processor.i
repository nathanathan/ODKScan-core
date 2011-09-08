/*
 * include the headers required by the generated cpp code
 */
%{
#include "core/Processor.h"
%}

%typemap(javaimports) Processor "
/** This class provides an interface to the image processing pipeline and handles most of the JSON parsing.
*/"
%include "core/Processor.h"
