const char* source = R"TOKEN(
    // If you want to create ImDrawList instances, pass them ImGui::GetDrawListSharedData() or create and use your own ImDrawListSharedData (so you can use ImDrawList without ImGui)
    //ImDrawList(const ImDrawListSharedData* shared_data) { _Data = shared_data; _OwnerName = NULL; Clear(); }
    //~ImDrawList() { ClearFreeMemory(); }
    IMGUI_API void  PushClipRect(ImVec2 clip_rect_min, ImVec2 clip_rect_max, bool intersect_with_current_clip_rect = false);  // Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
    IMGUI_API void  PushClipRectFullScreen();
    IMGUI_API void  PopClipRect();
    IMGUI_API void  PushTextureID(ImTextureID texture_id);
    IMGUI_API void  PopTextureID();

    // Ignored
    //inline ImVec2   GetClipRectMin() const { const ImVec4& cr = _ClipRectStack.back(); return ImVec2(cr.x, cr.y); }
    //inline ImVec2   GetClipRectMax() const { const ImVec4& cr = _ClipRectStack.back(); return ImVec2(cr.z, cr.w); }

    // Primitives
    IMGUI_API void  AddLine(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness = 1.0f);
    IMGUI_API void  AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All, float thickness = 1.0f);   // a: upper-left, b: lower-right (== upper-left + size), rounding_corners_flags: 4-bits corresponding to which corner to round
    IMGUI_API void  AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All);                     // a: upper-left, b: lower-right (== upper-left + size)
    IMGUI_API void  AddRectFilledMultiColor(const ImVec2& a, const ImVec2& b, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left);
    IMGUI_API void  AddQuad(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col, float thickness = 1.0f);
    IMGUI_API void  AddQuadFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col);
    IMGUI_API void  AddTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col, float thickness = 1.0f);
    IMGUI_API void  AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col);
    IMGUI_API void  AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12, float thickness = 1.0f);
    IMGUI_API void  AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments = 12);
    IMGUI_API void  AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL);
    IMGUI_API void  AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = NULL, float wrap_width = 0.0f, const ImVec4* cpu_fine_clip_rect = NULL);
    IMGUI_API void  AddImage(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a = ImVec2(0,0), const ImVec2& uv_b = ImVec2(1,1), ImU32 col = IM_COL32_WHITE);
    IMGUI_API void  AddImageQuad(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a = ImVec2(0,0), const ImVec2& uv_b = ImVec2(1,0), const ImVec2& uv_c = ImVec2(1,1), const ImVec2& uv_d = ImVec2(0,1), ImU32 col = IM_COL32_WHITE);
    IMGUI_API void  AddImageRounded(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, ImU32 col, float rounding, int rounding_corners = ImDrawCornerFlags_All);
    IMGUI_API void  AddPolyline(const ImVec2* points, int num_points, ImU32 col, bool closed, float thickness);
    IMGUI_API void  AddConvexPolyFilled(const ImVec2* points, int num_points, ImU32 col); // Note: Anti-aliased filling requires points to be in clockwise order.
    IMGUI_API void  AddBezierCurve(const ImVec2& pos0, const ImVec2& cp0, const ImVec2& cp1, const ImVec2& pos1, ImU32 col, float thickness, int num_segments = 0);

    // Stateful path API, add points then finish with PathFillConvex() or PathStroke()
    inline    void  PathClear()                                                 { _Path.Size = 0; }
    inline    void  PathLineTo(const ImVec2& pos)                               { _Path.push_back(pos); }
    inline    void  PathLineToMergeDuplicate(const ImVec2& pos)                 { if (_Path.Size == 0 || memcmp(&_Path.Data[_Path.Size-1], &pos, 8) != 0) _Path.push_back(pos); }
    inline    void  PathFillConvex(ImU32 col)                                   { AddConvexPolyFilled(_Path.Data, _Path.Size, col); _Path.Size = 0; }  // Note: Anti-aliased filling requires points to be in clockwise order.
    inline    void  PathStroke(ImU32 col, bool closed, float thickness = 1.0f)  { AddPolyline(_Path.Data, _Path.Size, col, closed, thickness); _Path.Size = 0; }
    IMGUI_API void  PathArcTo(const ImVec2& centre, float radius, float a_min, float a_max, int num_segments = 10);
    IMGUI_API void  PathArcToFast(const ImVec2& centre, float radius, int a_min_of_12, int a_max_of_12);                                            // Use precomputed angles for a 12 steps circle
    IMGUI_API void  PathBezierCurveTo(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, int num_segments = 0);
    IMGUI_API void  PathRect(const ImVec2& rect_min, const ImVec2& rect_max, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All);

    // Channels
    // - Use to simulate layers. By switching channels to can render out-of-order (e.g. submit foreground primitives before background primitives)
    // - Use to minimize draw calls (e.g. if going back-and-forth between multiple non-overlapping clipping rectangles, prefer to append into separate channels then merge at the end)
    IMGUI_API void  ChannelsSplit(int channels_count);
    IMGUI_API void  ChannelsMerge();
    IMGUI_API void  ChannelsSetCurrent(int channel_index);

    // Advanced
    // IMGUI_API void  AddCallback(ImDrawCallback callback, void* callback_data);  // Your rendering function must check for 'UserCallback' in ImDrawCmd and call the function instead of rendering triangles.
    // Cannot send callbacks to the client
    IMGUI_API void  AddDrawCmd();                                               // This is useful if you need to forcefully create a new draw call (to allow for dependent rendering / blending). Otherwise primitives are merged into the same draw-call as much as possible
    // Easier to manually implement
    //IMGUI_API ImDrawList* CloneOutput() const;                                  // Create a clone of the CmdBuffer/IdxBuffer/VtxBuffer.

    // Internal helpers
    // NB: all primitives needs to be reserved via PrimReserve() beforehand!
    IMGUI_API void  Clear();
    IMGUI_API void  ClearFreeMemory();
    IMGUI_API void  PrimReserve(int idx_count, int vtx_count);
    IMGUI_API void  PrimRect(const ImVec2& a, const ImVec2& b, ImU32 col);      // Axis aligned rectangle (composed of two triangles)
    IMGUI_API void  PrimRectUV(const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, ImU32 col);
    IMGUI_API void  PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col);
    inline    void  PrimWriteVtx(const ImVec2& pos, const ImVec2& uv, ImU32 col){ _VtxWritePtr->pos = pos; _VtxWritePtr->uv = uv; _VtxWritePtr->col = col; _VtxWritePtr++; _VtxCurrentIdx++; }
    inline    void  PrimWriteIdx(ImDrawIdx idx)                                 { *_IdxWritePtr = idx; _IdxWritePtr++; }
    inline    void  PrimVtx(const ImVec2& pos, const ImVec2& uv, ImU32 col)     { PrimWriteIdx((ImDrawIdx)_VtxCurrentIdx); PrimWriteVtx(pos, uv, col); }
    IMGUI_API void  UpdateClipRect();
    IMGUI_API void  UpdateTextureID();
)TOKEN";

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

enum Kind {
    NULL_TOKEN,
    VOID,
    CONST,
    REF,
    SEMICOLON,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    ASTERISK,
    ASSING,
    COMMA,
    IDENTIFIER,
    NUMBER_LITERAL,
    SKIPPED_BODY,
};

struct Token {
    Kind kind = NULL_TOKEN;
    const char* start = NULL, *end = NULL;
    int len = 0;
};

const char* cursor = source;

Token token;

void next_token() {
start:
    // Ignore whitespace
    while (isspace(*cursor))
        cursor += 1;
    assert(*cursor != 0);

    // Ignore comments
    if (cursor[0] == '/' && cursor[1] == '/') {
        while (*cursor != '\n')
            cursor += 1;
        goto start;
    }

    token.start = cursor;
    if (*cursor == '{') {
        token.kind = SKIPPED_BODY;
        cursor += 1;
        for (int level = 1; level > 0; cursor += 1) {
            if (*cursor == '{')
                level += 1;
            if (*cursor == '}')
                level -= 1;
        }
    }
    else {
        switch (*cursor) {
        case ';':
            token.kind = SEMICOLON;
            break;
        case '&':
            token.kind = REF;
            break;
        case '*':
            token.kind = ASTERISK;
            break;
        case '(':
            token.kind = BRACKET_OPEN;
            break;
        case ')':
            token.kind = BRACKET_CLOSE;
            break;
        case '=':
            token.kind = ASSING;
            break;
        case ',':
            token.kind = COMMA;
            break;
        default:
        {
            if (isdigit(*cursor)) {
                token.kind = NUMBER_LITERAL;
                // Don't evaluate the literal, just scan it in
                // Format digits[.digits[f]]
                while (isdigit(*cursor))
                    cursor += 1;
                if (*cursor == '.') {
                    cursor += 1;
                    while (isdigit(*cursor)) {
                        cursor += 1;
                        if (*cursor == 'f') {
                            cursor += 1;
                            break;
                        }
                    }
                }
            }
            else if (isalpha(*cursor)) {
                while (isalnum(*cursor) || *cursor == '_' || *cursor == ':')
                    cursor += 1;
                int len = cursor - token.start;
                if (strncmp(token.start, "void", len) == 0) {
                    token.kind = VOID;
                }
                else if (strncmp(token.start, "const", len) == 0) {
                    token.kind = CONST;
                }
                else if (strncmp(token.start, "inline", len) == 0 || strncmp(token.start, "IMGUI_API", len) == 0) {
                    // Skip these
                    goto start;
                }
                else {
                    token.kind = IDENTIFIER;
                }
            }
            cursor -= 1;
        }
        }
        cursor += 1;
    }
    token.end = cursor;
    token.len = token.end - token.start;
    assert(token.len != 0);
    printf("token: %d, %.*s\n", token.kind, token.len, token.start);
}

int main() {
    while (true) {
        next_token();
    }

    return 0;
}
