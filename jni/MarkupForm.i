%{
#include "MarkupForm.h"
%}

%typemap(javaimports) MarkupForm "

/** MarkupForm - for marking up form
*/"

class MarkupForm {
public:
bool markupForm(const char* markupPath, const char* formPath, const char* outputPath);
};
