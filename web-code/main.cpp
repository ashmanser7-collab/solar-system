#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

#include <emscripten.h>

EM_JS(int, getProgramWidth, (), {
    return Module.programWidth;
});

EM_JS(int, getProgramHeight, (), {
    return Module.programHeight;
});

EM_JS(double, get_timeStep, (), {
    return Module.timeStep;
});

EM_JS(int, get_stepsperrender, (), {
    return Module.stepsperrender;
});

EM_JS(double, get_scale, (), {
    return Module.scale;
});

EM_JS(double, get_radiusScale, (), {
    return Module.radiusScale;
});

double scale = get_scale();
int stepsperrender = get_stepsperrender();
double timeStep = get_timeStep();
double radiusScale = get_radiusScale();
const double PI = 3.14159265358979323846;
const double gravity = 6.6743e-11;

long frames = 0;
long simulated_time;
int width = getProgramWidth();
int height = getProgramHeight();

GLFWwindow* window;

class object {
    public:
        double x;
        double y;
        double xv;
        double yv;
        double density;
        float size;
        double mass;
        double radius_scale = radiusScale;
        object(double nx, double ny, double nd, float ns, double x_v, double y_v, double nrs) {
            x = nx;
            y = ny;
            density = nd;
            size = ns;
            mass = density * size * size * size * PI * 4/3;
            xv = x_v;
            yv = y_v;
            radius_scale = nrs;
        }
};

std::pair<double, double> compute_distance_angle(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double distance = std::hypot(dx, dy);
    double angle = std::atan2(dy, dx);
    return std::make_pair(distance, angle);
}

double compute_force(double m1, double m2, double distance) {
    if (distance == 0) return 0;
    return m1 * m2 * gravity / (distance * distance);
}

const int num_ojects = 9;
object objects[num_ojects] = {
    object(0, 0, 1408, 6.9634e8, 0, 0, 4*6.9634e-12),
    object(5.79e10, 0, 5427, 2.44e6, 0, 47900, 4*2.44e-10),
    object(1.082e11, 0, 5.24e3, 6.05e6, 0, 3.50e4, 4*6.05e-10),
    object(1.496e11, 0, 5.51e3, 6.38e6, 0, 2.98e4, 4*6.38e-10),
    object(2.279e11, 0, 3.93e3, 3.40e6, 0, 2.41e4, 4*3.40e-10),
    object(7.786e11, 0, 1.33e3, 7.14e7, 0, 1.31e4, 4*7.14e-11),
    object(1.4335e12, 0, 6.87e2, 6.00e7, 0, 9.70e3, 4*6.00e-11),
    object(2.8725e12, 0, 1.27e3, 2.54e7, 0, 6.81e3, 4*2.54e-11),
    object(4.4951e12, 0, 1.64e3, 2.43e7, 0, 5.43e3, 4*2.43e-11)
};



GLFWwindow* initiate_window() {
    GLFWwindow* window = glfwCreateWindow(width, height, "Gravity Sim", NULL, NULL);
    return window;
}

void drawCircle(float cx, float cy, float r, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * PI * float(i) / float(num_segments);
        float x = r * std::cos(theta);
        float y = r * std::sin(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

void main_loop() {
    timeStep = get_timeStep();
    stepsperrender = get_stepsperrender();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    for (int step = 0; step < stepsperrender; step++){
        for (int i = 0; i < num_ojects; i++) {
            for (int j = num_ojects-1; j > i; j--) {
                auto [distance, angle] = compute_distance_angle(objects[i].x, objects[i].y, objects[j].x, objects[j].y);
                double force = compute_force(objects[i].mass, objects[j].mass, distance);
                double fx = force * std::cos(angle);
                double fy = force * std::sin(angle);
                objects[i].xv += fx / objects[i].mass * timeStep/stepsperrender;
                objects[i].yv += fy / objects[i].mass * timeStep/stepsperrender;
                objects[j].xv -= fx / objects[j].mass * timeStep/stepsperrender;
                objects[j].yv -= fy / objects[j].mass * timeStep/stepsperrender;
            }
            objects[i].x += objects[i].xv * timeStep/stepsperrender;
            objects[i].y += objects[i].yv * timeStep/stepsperrender;
        }
    }
    for (int i = 0; i < num_ojects; i++) {
        glColor3f(1.0f, 1.0f, 1.0f);
        float ndc_x = objects[i].x * scale;
        float ndc_y = objects[i].y * scale;
        float ndc_r = objects[i].size * objects[i].radius_scale;
        drawCircle(ndc_x, ndc_y, ndc_r, 50);
    }
    frames += 1;
    simulated_time = frames * timeStep / 3600;
    std::cout << "Frame: " << frames << "      simulated hours: " << simulated_time << std::endl << std::endl; 
    glfwSwapBuffers(window);
    glfwPollEvents();
}

int main() {

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    window = initiate_window();

    glfwMakeContextCurrent(window);

    emscripten_set_main_loop(main_loop, 0, 1);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
void stopProgram() {
    emscripten_cancel_main_loop();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}

}