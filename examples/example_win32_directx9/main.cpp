#include <stdio.h>
#include <vector>

#include "prolouge.h"

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

struct GuySlim {
    float fitness;
    int n_node, n_muscle;
};

struct Generation {
    Guy worst, avg, best;
    std::vector<GuySlim> guys;
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

void tick_guy_life(Guy& guy) {
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
}

void tick_guy(Guy& guy) {
    tick_guy_life(guy);
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
    auto& draw = *ImGui::GetWindowDrawList();
    float ground_pos = 0.8f;

    float average_x = 0;
    for (Node& n : guy.nodes)
        average_x += n.pos.x / guy.nodes.size();
    guy.camera_x += (average_x - guy.camera_x) * 0.1f;

    auto tran = [&](float2 vec) -> float2 {
        return float2{ (vec.x - guy.camera_x) * scale + size / 2, ground_pos * size - vec.y * scale } + pos;
    };

    draw.PushClipRect(pos, pos + size);
    // Background
    draw.AddRectFilled(pos, pos + size, guy.t_simulation < 900 ? ImColor(120, 200, 255) : ImColor(60, 100, 128));
    // Draw posts
    ImFont& font = *ImGui::GetFont();
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

    //ImGui::SliderFloat("ground pos", &ground_pos, 0, 1);
    //ImGui::DragFloat("cam x", &guy.camera_x);
}

void draw_fitness() {
    if (generations.size() == 0)
        return;

    std::vector<float> line(generations[0].guys.size());

}

void draw_species() {

}

void draw_histogram() {

}

// Main code
int main(int, char**) {
    if (!init())
        return false;

    //ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    while (start_frame()) {
        fullscreen_dockspace();
        ImGuiIO& io = ImGui::GetIO();

        using namespace ImGui;

        ImGui::ShowDemoWindow();

        ImGui::Begin("performance graph");
        SetWindowFontScale(3);
        Text("Generation %d", 1);
        SetWindowFontScale(1);

        ImGui::End();

        ImGui::Begin("species graph");
        ImGui::End();

        ImGui::Begin("control");
        static float scale = 2.0f / 0.015f;
        srand(10);
        static Guy test_guy = gen_guy();
        tick_guy(test_guy);
        draw_guy(test_guy,
            GetCursorScreenPos(),
            linalg::minelem((float2)GetContentRegionAvail()),
            scale);
        ImGui::DragFloat("scale", &scale, 1);

        ImGui::End();

        ImGui::End();

        end_frame();
    }

    shutdown();
    return 0;
}
