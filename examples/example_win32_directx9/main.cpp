#include <stdio.h>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <vector>

#include <immintrin.h>

#include "prolouge.h"

namespace im = ImGui;

typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t uint;

#pragma warning(disable : 4267)

//#define MYRAND_MAX (u32(1 << 31) - 1)
//u32 myrand() {
//    static u32 state = 1;
//    return state = (state * 1103515245 + 12345) & MYRAND_MAX;
//}

#define MYRAND_MAX (u64(-1))
u64 myrand() {
    static u64 state = 12904;
    state ^= state >> 12; // a
    state ^= state << 25; // b
    state ^= state >> 27; // c
    return state * UINT64_C(0x2545F4914F6CDD1D);
}


float random(float min, float max) {
    return min + float(myrand()) / float(MYRAND_MAX)*(max - min);
}
float random(float max) {
    return random(0, max);
}

int irandom(int min, int max) {
    return min + myrand() % (max - min);
}
int irandom(int max) {
    return myrand() % max;
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

float r() {
    float r = powf(random(-1, 1), 19);
    if (isnan(r)) {
        r = 0;
        printf("r is nan\n");
    }
    return r;
}

struct Node {
    float2 pos, vel;
    float mass_diameter, friction_c;
};

struct Muscle {
    u32 node1, node2;
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
    float mutatibility = 1;
    float fitness = 0;
    bool alive = true;
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
        //assert(!isnan(a.vel.x));
        //assert(!isnan(b.vel.x));
    }
    for (Node& n : guy.nodes) {
        // Node physics
        n.vel *= AIR_FRICTION;
        n.vel += GRAVITY;
        n.pos += n.vel;
        // Collision with ground
        float dif = n.pos.y - n.mass_diameter / 2;
        if (dif < 0) {
            n.pos.y = n.mass_diameter / 2;
            n.vel.y = 0;
            //n.vel.y = -n.vel.y * 0.9;
            n.pos.x -= n.vel.x * n.friction_c;
            //n.vel.x = copysignf(fmaxf(0, fabs(n.vel.x) + dif * n.friction_c * FRICTION), n.vel.x);
            //n.vel.x -= copysignf(dif * FRICTION, n.vel.x);

            if (n.vel.x > 0) {
                n.vel.x -= n.friction_c * -dif * FRICTION;
                if (n.vel.x < 0)
                    n.vel.x = 0;
            }
            else {
                n.vel.x += n.friction_c * -dif * FRICTION;
                if (n.vel.x > 0)
                    n.vel.x = 0;
            }
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

void add_random_muscle_between(Guy& guy, u32 node1, u32 node2) {
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

void check_and_fix_guy(Guy& guy) {
    // Remove double muscles
    auto& mus = guy.muscles;
    for (uint i = 0; i < mus.size(); i++) {
        Muscle& a = mus[i];
        assert(a.node1 < guy.nodes.size());
        assert(a.node2 < guy.nodes.size());
        if (a.node1 == a.node2) {
            mus.erase(mus.begin() + i);
            i -= 1;
            continue;
        }
        for (uint j = i + 1; j < mus.size(); j++) {
            Muscle& b = mus[j];
            if (
                a.node1 == b.node1 && a.node2 == b.node2 ||
                a.node1 == b.node2 && a.node2 == b.node1) {
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
                to = irandom(guy.nodes.size());
            while (to == i || to == first_edge);
            add_random_muscle_between(guy, i, to);
        }
    }
    for (Node& n : guy.nodes) {
        n.pos = linalg::clamp(n.pos, float2(-3, MINIMUM_NODE_SIZE), float2(3, 3));
    }
}

Guy gen_guy() {
    Guy guy;
    int n_node = irandom(3, 6 + 1);
    int n_muscle = irandom(n_node - 1, n_node * 3 - 6 + 1);

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
                node2 = irandom(n_node);
            while (node2 == node1);
        }
        add_random_muscle_between(guy, node1, node2);
    }

    guy.heartbeat = random(40, 80);

    check_and_fix_guy(guy);
    move_until_stable(guy);
    return std::move(guy);
}

Guy reproduce_guy(const Guy& guy) {
    Guy g = guy; // copy
    float cm = g.mutatibility * MUTABILITY_FACTOR;
    static int cnt = 1;
    // Modify nodes
    for (Node& n : g.nodes) {
        n.pos += float2(r() * 0.5f * cm, r() * 0.5f * cm);
        n.mass_diameter = clamp(n.mass_diameter + r() * 0.1f * cm, MINIMUM_NODE_SIZE, MAXIMUM_NODE_SIZE);
        n.friction_c = clamp(n.friction_c + r() * 0.1f * cm, MINIMUM_NODE_FRICTION, MAXIMUM_NODE_FRICTION);
    }
    // Modify muscles
    for (Muscle& m : g.muscles) {
        if (random(1) < 0.02f * cm)
            m.node1 = irandom(g.nodes.size());
        if (random(1) < 0.02f * cm)
            m.node2 = irandom(g.nodes.size());
        m.rigidity = clamp(m.rigidity * (1 + r() * 0.9f * cm), 0.01f, 0.08f);
        float contract_length = clamp(m.contract_length + r() * cm, 0.5f, 2);
        float extend_length = clamp(m.extend_length + r() * cm, 0.5f, 2);
        m.contract_length = fminf(contract_length, extend_length);
        m.extend_length = fminf(fmaxf(contract_length, extend_length), m.contract_length * (1 + 0.025f * m.rigidity));
        if (irandom(2) < 1)
            m.contract_time = fmodf(m.contract_time + (m.contract_time - m.extend_time) * r() * cm, 1);
        else
            m.extend_time = fmodf(m.extend_time + (m.extend_time - m.contract_time) * r() * cm, 1);
        m.period += r();
        m.length = int(m.contract_time / m.extend_time) ? m.contract_length : m.extend_length;
    }
    // Remove random node
    if (random(1) < 0.04f * cm) {
        uint rm = irandom(g.nodes.size());
        g.nodes.erase(g.nodes.begin() + rm);
        for (uint i = 0; i < g.muscles.size(); i++) {
            Muscle& m = g.muscles[i];
            if (m.node1 == rm || m.node2 == rm) {
                g.muscles.erase(g.muscles.begin() + i);
                i -= 1;
            }
            else {
                if (m.node1 > rm)
                    m.node1 -= 1;
                if (m.node2 > rm)
                    m.node2 -= 1;
            }
        }
    }
    // Add random node
    if (random(1) < 0.04f * cm || g.nodes.size() <= 2) {
        Node n;
        uint parent = irandom(g.nodes.size());
        n.pos = g.nodes[parent].pos + linalg::rot(random(0, float(2 * M_PI)), float2(sqrtf(random(0, 1)), 0)); // random angle and distance
        n.mass_diameter = random(MINIMUM_NODE_SIZE, MAXIMUM_NODE_SIZE);
        n.friction_c = random(MINIMUM_NODE_FRICTION, MAXIMUM_NODE_FRICTION);
        int closest = -1;
        float min_dist = FLT_MAX;
        for (uint i = 0; i < g.nodes.size(); i++) {
            float dist = linalg::distance(n.pos, g.nodes[i].pos);
            if (i != parent && dist < min_dist) {
                closest = i;
                min_dist = dist;
            }
        }
        assert(closest != -1);
        g.nodes.push_back(n);
        add_random_muscle_between(g, parent, g.nodes.size() - 1);
        add_random_muscle_between(g, closest, g.nodes.size() - 1);
    }
    // Add random muscle
    if (random(1) < 0.04f * cm)
        add_random_muscle_between(g, irandom(g.nodes.size()), irandom(g.nodes.size()));
    // Remove random muscle
    if (random(1) < 0.04f * cm && g.muscles.size() > 0)
        g.muscles.erase(g.muscles.begin() + irandom(g.muscles.size()));

    g.heartbeat += r() * 16 * cm;
    g.mutatibility = fminf(cm * random(0.8f, 1.25f), 2);
    g.fitness = 0;

    check_and_fix_guy(g);
    move_until_stable(g);
    return std::move(g);
}

Generation do_generation(std::vector<Guy>& pop) {
    Generation gen;

    // Evaluate (skip survivors from previous generation)
    for (Guy& guy : pop) {
        if (guy.fitness == 0) {
            Guy eval = guy;
            for (int i = 0; i < 15 * 60; i++)
                tick_guy(eval);
            float avg = 0;
            for (Node& n : eval.nodes)
                avg += n.pos.x / (float)eval.nodes.size();
            guy.fitness = !isnan(avg) ? avg : 0; // kill creatures which break the physics engine
        }
    }

    std::sort(pop.begin(), pop.end(), [](const Guy& a, const Guy& b) {return a.fitness < b.fitness; });

    // Fill in gen
    gen.fit.reserve(pop.size());
    gen.species.reserve(pop.size());
    for (Guy& g : pop) {
        gen.fit.push_back(g.fitness);
        gen.species.push_back({ (int)g.nodes.size(), (int)g.muscles.size(), 0 });
    }
    gen.best = pop.back();
    gen.avg = pop[pop.size() / 2];
    gen.worst = pop.front();

    {
        std::sort(gen.species.begin(), gen.species.end(), cmp_species);
        int j = 0;
        for (uint i = 0; i < gen.species.size(); i++) {
            Species &a = gen.species[j], b = gen.species[i];
            if (a.n_nodes == b.n_nodes && a.n_muscles == b.n_muscles)
                a.count += 1;
            else {
                j += 1;
                gen.species[j] = { b.n_nodes, b.n_muscles, 1 };
            }
        }
        gen.species.resize(j + 1);
    }

    // Kill half
    for (uint i = 0; i < pop.size() / 2; i++) {
        uint j1 = i, j2 = pop.size() - i - 1;
        if (i / float(pop.size()) <= (powf(random(-1, 1), 3) + 1) / 2)
            std::swap(j1, j2);
        pop[j1].alive = true;
        pop[j2].alive = false;
    }
    // Reproduce
    for (uint i = 0, j = 0; i < pop.size(); i++) {
        if (pop[i].alive) {
            while (pop[j].alive)
                j += 1;
            pop[j] = reproduce_guy(pop[i]);
            pop[j].alive = false; // avoid reproducing offspring
            pop[j].fitness = 0; // set fitness exactly to zero to actually test them
            j += 1;
        }
    }
    for (Guy& g : pop)
        g.alive = true;

    struct Shuff {
        typedef u64 result_type;
        static u64 min() {
            return 0;
        }
        static u64 max() {
            return u64(-1);
        }
        u64 operator()() {
            return myrand();
        }
    };

    std::shuffle(pop.begin(), pop.end(), Shuff());

    return std::move(gen);
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

void draw_fitness_and_species() {
    const uint gencount = generations.size();
    const uint gensize = gencount != 0 ? generations[0].fit.size() : 0;
    float padding = 5;
    float label_scale = 1;

    im::Begin("performance graph");
    float tickwidth = im::CalcTextSize(" 999 m").x;
    im::SetWindowFontScale(label_scale);
    float2 max_label = (float2)im::CalcTextSize("S99: 999");

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
        auto& draw = *im::GetWindowDrawList();

        im::SetWindowFontScale(3);
        im::Text("Generation %d", generations.size());
        if (gencount != 0) {
            char buf[32];
            snprintf(buf, 32, "%.2f m", generations.back().avg.fitness);
            im::SameLine(im::GetContentRegionAvailWidth() - max_label.x - im::CalcTextSize(buf).x);
            im::Text(buf);
        }
        im::SetWindowFontScale(1);

        float2 pos = im::GetCursorScreenPos(), size = im::GetContentRegionAvail();

        if (gencount != 0) {
            size.x -= max_label.x + padding;
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
                draw.AddPolyline(&line.front(), (u32)line.size(), l.color, false, l.thickness);
            }
            //im::DragInt("max_ticks", &max_ticks, 0.05f);
        }
        im::End();
    }

    { // Draw spcies
        im::Begin("species graph");
        im::SetWindowFontScale(label_scale);
        auto& draw = *im::GetWindowDrawList();
        float2 pos = im::GetCursorScreenPos(), size = im::GetContentRegionAvail();

        if (gencount < 2) {
            im::End();
            return;
        }

        size.x -= tickwidth + max_label.x + padding * 3;
        pos.x += tickwidth + padding;

        auto tran = [&](int x, int y) -> float2 {
            return float2((float)x / (float)(gencount - 1), (float)y / (float)gensize) * size + pos;
        };

        for (int i = 0; i < gencount - 1; i++) {
            auto& in_gen_a = generations[i].species;
            auto& in_gen_b = generations[i + 1].species;

            int counta = 0, countb = 0;

            // @POLYNOMIC: use cmp_species and two indexes
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
        // Lines along the sides to avoid AA
        draw.Flags &= ~ImDrawListFlags_AntiAliasedLines;
        draw.AddRect(tran(0, 0) - 1, tran(gencount - 1, gensize) + 1, ImColor(0, 0, 0), 0, 0, 2);

        // Background to all labels
        draw.AddRectFilled(tran(gencount - 1, 0) + float2(padding, 0), tran(gencount - 1, gensize) + float2(max_label.x + padding * 3, 0), ImColor(0, 0, 0));

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
            float2 p = tran(gencount - 1, s.offset + s.s.count / 2) + float2(padding * 2, -max_label.y / 2.0f);
            ImU32 col = species_color(s.s, false);
            // Background to single label
            //draw.AddRectFilled(p, p + max_label + padding * 2, ImColor(240, 240, 240));
            draw.AddText(p, col, buf);
            snprintf(buf, sizeof(buf), "%d", s.s.count);
            draw.AddText(p + float2(max_label.x - im::CalcTextSize(buf).x, 0), col, buf);
        }
        draw.Flags |= ImDrawListFlags_AntiAliasedLines;

        im::End();
    }
}

void draw_histogram() {

}

// Main code
int main(int, char**) {
    if (!init())
        return false;

    std::vector<Guy> population(1000);
    for (Guy& g : population)
        g = gen_guy();

    while (start_frame()) {
        fullscreen_dockspace();
        ImGuiIO& io = im::GetIO();


        im::ShowDemoWindow();

        im::Begin("control");
        //static float scale = 2.0f / 0.015f;
        static float scale = 60;

        //generations.push_back(do_generation(population));

        im::Button("Run");
        if (im::IsItemHovered() && io.MouseDown[0])
            generations.push_back(do_generation(population));



        static Guy guy = gen_guy();
        //tick_guy(guy);
        //draw_guy(guy,
        //    im::GetCursorScreenPos(),
        //    linalg::minelem((float2)im::GetContentRegionAvail()),
        //    scale);
        im::DragFloat("scale", &scale, 1, 1e-3f, 1e3f);

        if (generations.size() > 0) {
            Generation& gen = generations.back();
            float padding = 10;
            float size = im::GetContentRegionAvailWidth() / 3 - padding;

            auto disp = [&](Guy& guy) {
                static Guy* orig = NULL;
                static Guy moving;

                float2 pos = (float2)im::GetCursorScreenPos();
                im::Dummy((float2)size); im::SameLine();

                if (im::IsItemHovered()) {
                    if (!orig) {
                        moving = guy;
                        orig = &guy;
                    }
                    if (orig == &guy) {
                        if (io.MouseDown[0])
                            moving = reproduce_guy(guy);
                        tick_guy(moving);
                    }
                }
                else if (orig == &guy)
                    orig = NULL;

                draw_guy(orig == &guy ? moving : guy, pos + padding, size - padding, scale);
            };

            disp(gen.worst);
            disp(gen.avg);
            disp(gen.best);
            im::NewLine();
        }

        im::End();

        draw_fitness_and_species();

        im::End();

        end_frame();
    }

    shutdown();
    return 0;
}
