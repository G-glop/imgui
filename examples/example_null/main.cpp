// dear imgui: null/dummy example application (compile and link imgui with no inputs, no outputs)
#include "imgui.h"
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <chrono>
#include <string>

// Program to test performace of imgui
// runs tests from an array specifing an implementation (as a parameter to switch statement) and a name
// results are printed both to stdout and OutputDebugString for conviniece
// output can be visualized using https://repl.it/@G_glop/ViolentMellowNanocad

// NOTICE: ImDrawList::AddPolyline and ImFont::RenderText are stubbed out

#pragma warning(disable: 4996) // MSVC begin MSVC

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
	//OutputDebugStringA((LPCSTR)str.c_str());
	printf("%s", str.c_str());
}

int convexpoly_impl_num = 0;

int main(int, char**)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Build atlas
    unsigned char* tex_pixels = NULL;
    int tex_w, tex_h;
    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

#ifndef _WIN64
    fout("32 ");
#else
    fout("64 ");
#endif
#ifdef _DEBUG
    fout("debug\n");
#else
    fout("release\n");
#endif

	struct Test {
		int num;
		const char *name;
	};

	Test tests[] = {
		0, "warmup",
		0, "initial",
		5, "initial inline",
		4, "joined",
		1, "vtx reuse safe",
		3, "idx reuse",
		2, "vtx reuse",
		0, "initial sanity",
	};
	
	for (Test t : tests) {
		convexpoly_impl_num = t.num;
		fout("#%d ", t.num);

		//warmup
		for (int i = 0; i < 10; i++) {
			io.DisplaySize = ImVec2(1920, 1080);
			io.DeltaTime = 1.0f / 60.0f;
			ImGui::NewFrame();

			ImGui::ShowDemoWindow(NULL);
			ImGui::ShowDemoWindow(NULL);
			ImGui::ShowDemoWindow(NULL);

			ImGui::Render();
		}

		double start = seconds();
		double end = seconds();
		int iter = 0;

		// adjust these parameters so timing functions aren't called too often (indicated by dots in the output)
#ifndef _DEBUG 
		size_t per_iter = 30000;
#else
		size_t per_iter = 3000;
#endif

		while (end - start < 5) {
			iter++;
			fout(".", iter);
			for (size_t n = 0; n < per_iter; n++)
			{
				io.DisplaySize = ImVec2(1920, 1080);
				io.DeltaTime = 1.0f / 60.0f;
				ImGui::NewFrame();

				ImGui::ShowDemoWindow(NULL);
				ImGui::ShowDemoWindow(NULL);
				ImGui::ShowDemoWindow(NULL);

				ImGui::Render();
			}
			end = seconds();
		};
		assert(convexpoly_impl_num >= 0);
		fout("\n%-18s%f us\n", t.name, (end - start) / iter / per_iter * 1e6);
	}

    ImGui::DestroyContext();
    return 0;
}
