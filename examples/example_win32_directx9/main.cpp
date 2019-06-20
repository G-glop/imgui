#include <stdio.h>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <vector>

#include "prolouge.h"

namespace im = ImGui;

typedef unsigned int uint;

float random(float min, float max) {
    return min + float(rand()) / float(RAND_MAX)*(max - min);
}
float random(float max) {
    return random(0, max);
}

int irandom(int min, int max) {
    return min + rand() % (max - min + 1);
}
int irandom(int max) {
    return irandom(0, max);
}

float clamp(float value, float min, float max) {
    return fmax(fmin(value, max), min);
}

float gauss() {
    float x = random(1),
        y = random(1),
        z = sqrtf(-2 * logf(x)) * cosf(2 * (float)M_PI * y);
    return z;
}

struct Node {
    float2 pos, vel;
    float mass_diameter, friction_c;
};

struct Muscle {
    uint node1, node2;
    float period;
    float contract_time, contract_length, extend_time, extend_length;
    //float thru_period;
    float length;
    float rigidity;
};

struct Guy {
    std::vector<Node> nodes;
    std::vector<Muscle> muscles;
    float heartbeat;
    float t_simulation = 0;
    float camera_x = 0;
};

struct Species {
    int n_nodes, n_muscles, count;
};

bool cmp_species(Species a, Species b) {
    if (a.n_nodes != b.n_nodes)
        return a.n_nodes < b.n_nodes;
    else
        return a.n_muscles < b.n_muscles;
}

struct Generation {
    Guy worst, avg, best;
    std::vector<float> fit;
    std::vector<Species> species;
};

std::vector<Generation> generations;

const float SORT_ANIMATION_SPEED = 5.0f; // Determines speed of sorting animation.  Higher number is faster.
const float MINIMUM_NODE_SIZE = 0.4f; // Note: all units are 20 cm.  Meaning, a value of 1 equates to a 20 cm node.
const float MAXIMUM_NODE_SIZE = 0.4f;
const float MINIMUM_NODE_FRICTION = 0.0f;
const float MAXIMUM_NODE_FRICTION = 1.0f;
const float2 GRAVITY = { 0, -0.005f }; // higher = more friction.
const float AIR_FRICTION = 0.95f; // The lower the number, the more friction.  1 = no friction.  Above 1 = chaos.
const float MUTABILITY_FACTOR = 1.0f; // How fast the creatures mutate.  1 is normal.
const float FRICTION = 4.0f;

void tick_guy_physics(Guy& guy) {
    // Apply muscle forces
    for (Muscle& m : guy.muscles) {
        Node& a = guy.nodes[m.node1];
        Node& b = guy.nodes[m.node2];
        float2 diff = a.pos - b.pos;
        float len = linalg::length(diff);
        float force = clamp(1 - len / m.length, -0.4f, 0.4f);
        float2 ndiff = diff / len * (force * m.rigidity);
        a.vel += ndiff / a.mass_diameter;
        b.vel -= ndiff / b.mass_diameter;
    }
    for (Node& n : guy.nodes) {
        // Node physics
        n.vel += GRAVITY;
        n.vel *= AIR_FRICTION;
        n.pos += n.vel;
        // Collision with ground
        float dif = n.pos.y - n.mass_diameter / 2;
        if (dif < 0) {
            n.pos.y = n.mass_diameter / 2;
            n.vel.y = 0;
            n.pos.x -= n.vel.x * n.friction_c;
            n.vel.x -= copysignf(dif * n.friction_c * FRICTION, n.vel.x);
        }
    }
}

void tick_guy(Guy& guy) {
    for (Muscle& m : guy.muscles) {
        float phase = fmod(guy.t_simulation / (guy.heartbeat * m.period), 1.0f);
        if (
            phase <= m.extend_time && m.extend_time <= m.contract_time ||
            m.contract_time <= phase && phase <= m.extend_time ||
            m.extend_time <= m.contract_time && m.contract_time <= phase)
            m.length = m.contract_length;
        else
            m.length = m.extend_length;
    }
    tick_guy_physics(guy);
    guy.t_simulation += 1;
}

void move_until_stable(Guy& guy) {
    for (int i = 0; i < 200; i++) {
        tick_guy_physics(guy);
    }
    float average_x = 0;
    for (Node& n : guy.nodes)
        average_x += n.pos.x / guy.nodes.size();
    for (Node& n : guy.nodes) {
        n.pos.x -= average_x;
        n.vel = { 0, 0 };
    }
}

void add_random_muscle_from_to(Guy& guy, int node1, int node2) {
    guy.muscles.resize(guy.muscles.size() + 1);
    Muscle& m = guy.muscles.back();

    m.node1 = node1;
    m.node2 = node2;

    m.period = random(1, 3);
    m.rigidity = random(0.02f, 0.08f);

    float rlen1 = random(0.5, 1.5);
    float rlen2 = random(0.5, 1.5);
    m.contract_length = fmin(rlen1, rlen2);
    m.extend_length = fmax(rlen1, rlen2);
    m.contract_time = random(0, 1);
    m.extend_time = random(0, 1);
    m.length = int(m.contract_time / m.extend_time) ? m.contract_length : m.extend_length;
}

//void add_random_muscle(Guy& guy) {
//
//}

void check_and_fix_guy(Guy& guy) {
    // Remove double muscles
    auto& mus = guy.muscles;
    for (uint i = 0; i < mus.size(); i++) {
        Muscle& a = mus[i];
        for (uint j = i + 1; j < mus.size(); j++) {
            Muscle& b = mus[j];
            if (
                a.node1 == b.node1 && a.node2 == b.node2 ||
                a.node1 == b.node2 && a.node2 == b.node1 ||
                b.node1 == b.node2) {
                mus.erase(mus.begin() + j);
                j -= 1;
            }
        }
    }
    // Connect lone nodes
    for (uint i = 0; i < guy.nodes.size(); i++) {
        uint first_edge = 0;
        int edges = 0;
        for (Muscle& m : guy.muscles) {
            if (m.node1 == i)
                first_edge = m.node2;
            if (m.node2 == i)
                first_edge = m.node1;
            if (m.node1 == i || m.node2 == i)
                edges += 1;
        }

        if (edges < 2) {
            uint to;
            do
                to = irandom(0, guy.nodes.size());
            while (to == i || to == first_edge);
            add_random_muscle_from_to(guy, i, to);
        }
    }
}

Guy gen_guy() {
    Guy guy;
    int n_node = irandom(3, 6);
    int n_muscle = irandom(n_node - 1, n_node * 3 - 6);

    guy.nodes.resize(n_node);
    for (Node& n : guy.nodes) {
        n.pos = { random(-1, 1), random(0, 2) };
        n.mass_diameter = random(MINIMUM_NODE_SIZE, MAXIMUM_NODE_SIZE);
        n.friction_c = random(MINIMUM_NODE_FRICTION, MAXIMUM_NODE_FRICTION);
    }

    guy.muscles.reserve(n_muscle);
    for (int i = 0; i < n_muscle; i++) {
        int node1, node2;
        if (i < n_node - 1) {
            node1 = i;
            node2 = i + 1;
        }
        else {
            node1 = irandom(n_node);
            do
                node2 = irandom(n_node - 1);
            while (node2 == node1);
        }
        add_random_muscle_from_to(guy, node1, node2);
    }

    guy.heartbeat = random(40, 80);
    check_and_fix_guy(guy);
    move_until_stable(guy);
    return std::move(guy);
}

void draw_guy(Guy& guy, float2 pos, float size, float scale) {
    auto& draw = *im::GetWindowDrawList();
    float ground_pos = 0.8f;

    float average_x = 0;
    for (Node& n : guy.nodes)
        average_x += n.pos.x / guy.nodes.size();
    guy.camera_x += (average_x - guy.camera_x) * 0.1f;

    auto tran = [&](float2 vec) -> float2 {
        return float2{ (vec.x - guy.camera_x) * scale + size / 2, ground_pos * size - vec.y * scale } +pos;
    };

    draw.PushClipRect(pos, pos + size);
    // Background
    draw.AddRectFilled(pos, pos + size, guy.t_simulation < 900 ? ImColor(120, 200, 255) : ImColor(60, 100, 128));
    // Draw posts
    ImFont& font = *im::GetFont();
    int spacing = 5;
    for (int i = int((guy.camera_x - size / 2 / scale) / spacing) * spacing; i < int(guy.camera_x + size / 2 / scale) + spacing; i += spacing) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d m", (int)i);
        float line_h = 0.5f;
        float post_h = 2.0f;
        float2 text_size = font.CalcTextSizeA(line_h, FLT_MAX, 0.0f, buffer);
        float2 text_center = { float(i), post_h };
        draw.AddLine(tran({ (float)i, 0 }), tran({ (float)i, post_h }), ImColor(255, 255, 255), 0.1f * scale);
        draw.AddRectFilled(tran(text_center + text_size * 0.6f), tran(text_center - text_size * 0.6f), ImColor(255, 255, 255));
        draw.AddText(&font, line_h * scale, tran({ float(i) - text_size.x / 2, post_h + text_size.y / 2 }), ImColor(120, 120, 120), buffer);
    }
    // Draw muscles
    for (Muscle& m : guy.muscles) {
        draw.AddLine(
            tran(guy.nodes[m.node1].pos),
            tran(guy.nodes[m.node2].pos),
            ImColor(70, 35, 0, int(m.rigidity * 3000)),
            (m.length == m.contract_length ? 0.1f : 0.2f) * scale);
    }
    // Draw nodes
    for (Node& n : guy.nodes) {
        draw.AddCircleFilled(
            tran(n.pos),
            n.mass_diameter / 2 * scale,
            n.friction_c <= 0.5 ?
            ImColor(255, 255 - int(n.friction_c * 512), 255 - int(n.friction_c * 512)) :
            ImColor(512 - int(n.friction_c * 512), 0, 0),
            20);
    }
    draw.AddRectFilled(pos + float2(0, size * ground_pos), pos + size, ImColor(0, 130, 0));
    draw.PopClipRect();

    //im::SliderFloat("ground pos", &ground_pos, 0, 1);
    //im::DragFloat("cam x", &guy.camera_x);
}

void draw_fitness() {

}

ImU32 species_color(Species species, bool is_label) {
    int num = species.n_nodes * 10 + species.n_muscles;
    float col = fmodf(float(num) * 1.618034f, 1);
    if (num == 46) {
        col = 0.083333f;
    }
    else if (num == 44) {
        col = 0.1666666f;
    }
    else if (num == 57) {
        col = 0.5f;
    }
    float light = 1.0f;
    if (fabsf(col - 0.333f) <= 0.18f && is_label) {
        light = 0.7f;
    }
    ImColor ret;
    ret.SetHSV(col, 1.0f, light);
    return ret;
};

void draw_species() {

}

void draw_fitness_and_species() {
    const int gencount = generations.size();
    const int gensize = gencount != 0 ? generations[0].fit.size() : 0;
    auto& draw = *im::GetOverlayDrawList();
    float tickwidth = im::CalcTextSize(" 999 m").x;
    float2 max_label = im::CalcTextSize("S99: 999 ");

    { // Draw fitness
        struct Line {
            int pos;
            float thickness = 3;
            ImU32 color = ImColor(0, 0, 0);

        };
        const Line lines[] = {
            {0},
            {1, 1},
            {2, 1},
            {3, 1},
            {4, 1},
            {5, 1},
            {6, 1},
            {7, 1},
            {8, 1},
            {9, 1},
            {10},
            {20},
            {30},
            {40},
            {50, 5, ImColor(255, 0, 0)},
            {60},
            {70},
            {80},
            {90},
            {91, 1},
            {92, 1},
            {93, 1},
            {94, 1},
            {95, 1},
            {96, 1},
            {97, 1},
            {98, 1},
            {99, 1},
            {100},
        };

        im::Begin("performance graph");
        im::SetWindowFontScale(3);
        im::Text("Generation %d", 1);
        im::SetWindowFontScale(1);

        float2 pos = im::GetCursorScreenPos(), size = im::GetContentRegionAvail();

        if (gencount == 0)
            return;

        float padding = 5;
        size.x -= max_label.x;
        draw.AddRectFilled(pos, pos + size, ImColor(220, 220, 220));
        size -= float2(tickwidth, 0) + padding * 2;
        pos += float2(tickwidth, 0) + padding;

        float min = FLT_MAX, max = -FLT_MAX;
        for (Generation& g : generations) {
            min = fminf(min, g.fit.front());
            max = fmaxf(max, g.fit.back());
        }
        float range = max - min;

        auto tran = [&](float2 p) {
            return float2(p.x / (float)(gencount - 1), 1 - (p.y - min) / range) * size + pos;
        };

        static int max_ticks = 10;
        float maxstep = range / (float)max_ticks;
        float upstep = powf(10, ceilf(log10f(maxstep)));
        float remstep = maxstep / upstep;
        float step = (remstep < 0.2f ? 0.2f : (remstep < 0.5f ? 0.5f : 1)) * upstep;

        for (
            float y = ceilf(min / step) * step;
            y <= floorf(max / step) * step;
            y += step)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d m", (int)y);
            float2 ts = im::CalcTextSize(buf);

            draw.AddLine(tran({ 0, y }), tran({ (float)gencount - 1, y }), ImColor(150, 150, 150), 2);
            draw.AddText(tran({ 0, y }) - float2(ts.x + 5, ts.y / 2), ImColor(150, 150, 150), buf);
        }

        std::vector<ImVec2> line(gencount);
        for (Line l : lines) {
            for (int i = 0; i < gencount; i++) {
                line[i] = tran({ (float)i, generations[i].fit[std::min(l.pos * gensize / 100, gensize - 1)] });
            }
            draw.AddPolyline(&line.front(), line.size(), l.color, false, l.thickness);
        }
        //im::DragInt("max_ticks", &max_ticks, 0.05f);
        im::End();
    }

    { // Draw spcies
        im::Begin("species graph");
        float2 pos = im::GetCursorScreenPos(), size = im::GetContentRegionAvail();

        float padding = 1.0f;
        size.x -= tickwidth + max_label.x + padding * 4;
        pos.x += tickwidth;

        auto tran = [&](int x, int y) -> float2 {
            return float2((float)x / (float)(gencount - 1), (float)y / (float)gensize) * size + pos;
        };

        for (int i = 0; i < gencount - 1; i++) {
            auto& in_gen_a = generations[i].species;
            auto& in_gen_b = generations[i + 1].species;

            int counta = 0, countb = 0;

            // @POLYNOMIC: add 'int index_in_next_gen' to Species and set it at append time
            for (Species a : in_gen_a) {
                for (Species b : in_gen_b) {
                    if (a.n_nodes == b.n_nodes &&
                        a.n_muscles == b.n_muscles) {
                        float2 pa = tran(i, counta), pb = tran(i + 1, countb);
                        float2 pc = tran(i + 1, countb + b.count), pd = tran(i, counta + a.count); // swapped to avoid bowties 
                        ImU32 col = species_color(a, false);
                        draw.AddQuad(pa, pb, pc, pd, col, 0.5f);
                        draw.AddQuadFilled(pa, pb, pc, pd, col);
                        counta += a.count;
                        countb += b.count;
                    }
                }
            }
        }

        // Background to all labels
        //draw.AddRectFilled(tran(gencount - 1, 0) + float2(padding * 3, 0), pos + (float2)im::GetContentRegionAvail() + float2(padding, 0), ImColor(240, 240, 240));

        struct Spec {
            Species s;
            int offset;
        };
        auto& species = generations.back().species;
        std::vector<Spec> spec(species.size());
        int offset = 0;
        for (uint i = 0; i < spec.size(); i++) {
            Species s = species[i];
            spec[i] = { s, offset };
            offset += s.count;
        }
        std::sort(spec.begin(), spec.end(), [](Spec a, Spec b) {return a.s.count > b.s.count; });
        for (uint i = 0; i < spec.size() && i < uint(size.y / max_label.y); i++) {
            Spec s = spec[i];
            if ((float)s.s.count / (float)gensize * 2 < max_label.y / size.y)
                break;
            char buf[32];
            snprintf(buf, sizeof(buf), "S%d%d:", s.s.n_nodes % 10, s.s.n_muscles % 10);
            float2 p = tran(gencount - 1, s.offset + s.s.count / 2) + float2(padding * 3, -padding - max_label.y / 2.0f);
            ImU32 col = species_color(s.s, false);
            // Background to single label
            //draw.AddRectFilled(p, p + max_label + padding * 2, ImColor(240, 240, 240));
            draw.AddText(p + padding, col, buf);
            snprintf(buf, sizeof(buf), "%d", s.s.count);
            draw.AddText(p + padding + float2(max_label.x - im::CalcTextSize(buf).x, 0), col, buf);
        }
        im::End();
    }
}

void draw_histogram() {

}

// Main code
int main(int, char**) {
    if (!init())
        return false;

    rand();
    for (int i = 0; i < 4; i++) {
        Generation gen;
        gen.fit.resize(1000);
        for (int j = 0; j < 1000; j++) {
            gen.fit[j] = (j - 150) / 10.0f * log(i + 1);
        }
        gen.species.resize(20);
        for (int i = 0; i < gen.species.size(); i++) {
            Species& s = gen.species[i];
            s.n_nodes = i / 2 + 1;
            s.n_muscles = i + 1;
            //s.count = fabs(gauss()) * 10000;
            s.count = rand();
        }
        int total = 0;
        for (Species& s : gen.species)
            total += s.count;
        for (Species& s : gen.species)
            s.count = s.count * 1000 / total;
        generations.push_back(std::move(gen));
    }

    while (start_frame()) {
        fullscreen_dockspace();
        ImGuiIO& io = im::GetIO();

        using namespace ImGui;

        im::ShowDemoWindow();

        draw_fitness_and_species();

        im::Begin("control");
        static float scale = 2.0f / 0.015f;
        srand(10);
        static Guy test_guy = gen_guy();
        tick_guy(test_guy);
        draw_guy(test_guy,
            GetCursorScreenPos(),
            linalg::minelem((float2)GetContentRegionAvail()),
            scale);
        im::DragFloat("scale", &scale, 1);

        im::End();

        im::End();

        end_frame();
    }

    shutdown();
    return 0;
}
