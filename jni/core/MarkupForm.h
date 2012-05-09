#ifndef MARKUP_FORM_H
#define MARKUP_FORM_H
//This is only a class to make jni happy
class MarkupForm {
public:
	static bool markupForm(const char* markupPath, const char* formPath, const char* outputPath, bool drawCounts = true);
};
#endif
