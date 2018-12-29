// dear imgui: null/dummy example application (compile and link imgui with no inputs, no outputs)
#include "imgui.h"
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <chrono>
#include <string>

#pragma warning(disable: 4996)

double seconds() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count() / 1e9;
};

int vstrprintf(std::string& dest, const char* format, va_list args) {
	int size = 0;
	va_list args2;
	va_copy(args2, args);
	size = vsnprintf(NULL, 0, format, args);
	dest.resize(size);
	if (size > 0)
		vsprintf(&dest.front(), format, args2);
	va_end(args2);
	return size;
}

int strprintf(std::string& dest, const char* format, ...) {
	int size = 0;
	va_list args;
	va_start(args, format);
	size = vstrprintf(dest, format, args);
	va_end(args);
	return size;
}

void fout(const char* frmt, ...) {
	std::string str;
	va_list args;
	va_start(args, frmt);
	vstrprintf(str, frmt, args);
	va_end(args);
	OutputDebugStringA((LPCSTR)str.c_str());
}

int main(int, char**)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Build atlas
    unsigned char* tex_pixels = NULL;
    int tex_w, tex_h;
    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

	fout("benchmark start\n");
	
	double start = seconds();
	double end = seconds();
	int iter = 0;
	size_t per_iter = 10000;
#ifndef _DEBUG 
	per_iter *= 10;
#endif
	while (end - start < 10){
		iter++;
		fout("round #%d\n", iter);
		for (size_t n = 0; n < per_iter; n++)
		{
			io.DisplaySize = ImVec2(1920, 1080);
			io.DeltaTime = 1.0f / 60.0f;
			ImGui::NewFrame();

			ImGui::ShowDemoWindow(NULL);

			ImGui::Render();
		}
		end = seconds();
	};
    fout("benchmark end\n");
	fout("time: %f us\n", (end - start)/iter/per_iter*1e6);

    ImGui::DestroyContext();
    return 0;
}
