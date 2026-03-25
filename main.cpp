
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include <GL/glut.h>
using namespace std;

GLFWwindow* StartGLFW();

int main() {
    GLFWwindow* window = StartGLFW();

    while(!glfwWindowShouldClose(window)) {


        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}


GLFWwindow* StartGLFW() {
    if(!glfwInit()) {
        std::cerr << "failed to initialize glfw!" << std::endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(800, 600, "orbital_motion_sim", NULL, NULL);

    return window;
}