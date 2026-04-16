#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>

const double G = 6.6743e-11; // m^3 kg^-1 s^-2

void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

class Vector3 {
    double x, y, z;
public:

    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }

    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3& operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(double scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    double magnitude() const {
        return std::sqrt(x*x + y*y + z*z);
    }

    Vector3 normalize() const {
        double mag = magnitude();
        if (mag == 0) return Vector3(0, 0, 0);

        return Vector3(x / mag, y / mag, z / mag);
    }

    Vector3() : x(0), y(0), z(0) {}

    void print() const {
        std::cout << "(" << x << ", " << y << ", " << z << ")" << std::endl;
    }

};


class Body {
    public:
        std::string name;
        double mass;
        Vector3 position;
        Vector3 velocity;

        Body(const std::string& name, double mass, const Vector3& position, const Vector3& velocity) 
            : name(name), mass(mass), position(position), velocity(velocity) {}

        void update(const Vector3& force, double dt) {
            Vector3 acceleration = force * (1.0 / mass);
            velocity += acceleration * dt;
            position += velocity * dt;
        }

        Vector3 gravitationalForceFrom(const Body& other) const {
            Vector3 pointingDirection = other.position - position;
            double distance = pointingDirection.magnitude();
            if (distance == 0) return Vector3(0, 0, 0);

            Vector3 forceDirection = pointingDirection.normalize();
            double forceMagnitude = G * mass * other.mass / (distance * distance);
            Vector3 force = forceDirection * forceMagnitude;
            return force;
        }   
};


// function performs simulation step
void simulateStep(std::vector<Body>& bodies, double dt) {
    std::vector<Vector3> forces(bodies.size());
    for (std::size_t iBody = 0; iBody < bodies.size(); ++iBody) {
        forces[iBody] = Vector3(); // reset force

        for (std::size_t jBody = 0; jBody < bodies.size(); ++jBody) {
            if (iBody == jBody) continue;
            forces[iBody] += bodies[iBody].gravitationalForceFrom(bodies[jBody]);
        }
    }
    // update bodies
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].update(forces[i], dt);
    }
}

// Render Helper: converts simulation coordinates to render coordinates
glm::vec2 toRenderCoordinates(const Body& body, const Body& earth) {
    const double renderScale = 4e8; // scale down positions for rendering
    double x = (body.position.getX() - earth.position.getX()) / renderScale;
    double y = (body.position.getY() - earth.position.getY()) / renderScale;
    return glm::vec2(x, y);
}

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // white color
}
)";


GLuint createShaderProgram() {
    // vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Compilation Failed:\n" << infoLog << std::endl;
    }

    // fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Compilation Failed:\n" << infoLog << std::endl;
    }
    
    // shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader Program Linking Failed:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}


int main() {

    //-------------- GLFW and GLEW Initialization ----------------
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Orbital Motion Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    glewExperimental = GL_TRUE;

    // initialize glew
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "GLEW Initialization failed: " << glewGetErrorString(glewStatus) << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "GLEW initialized successfully, OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    GLuint shaderProgram = createShaderProgram();

    // VAO and VBO setup


    //-------------- Simulation Objects ----------------

    Body earth("Earth", 5.972e24, Vector3(0, 0, 0), Vector3(0, 0, 0));
    Body satellite("Satellite", 1000, Vector3(0, 0, 7000000), Vector3(0, 7500, 0));
    Body moon("Moon", 7.342e22, Vector3(3.844e8, 0, 0), Vector3(0, 1022, 0));
    std::vector<Body> bodies = {earth, satellite, moon};  // container
    const int earthIndex = 0;
    const int satelliteIndex = 1;
    const int moonIndex = 2;

    double minSatDist = std::numeric_limits<double>::max();
    double maxSatDist = 0;

    double dt = 1; // timestep in seconds

    //-------------- Render Loop ----------------

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        simulateStep(bodies, dt);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPointSize(10.0f);
        // draw

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //cleanup
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;


        /*

        if (step % 100 == 0) {
            std::cout << "Step " << step << ": " << std::endl;
            for (const auto& body : bodies) {
                std::cout << body.name << " position: ";
                body.position.print();
            }
            Vector3 satelliteRelPosition = bodies[satelliteIndex].position - bodies[earthIndex].position;
            double satDist = satelliteRelPosition.magnitude();
            if (satDist < minSatDist) {
                minSatDist = satDist;
            }
            if (satDist > maxSatDist) {
                maxSatDist = satDist;
            }
            Vector3 moonRelPosition = bodies[moonIndex].position - bodies[earthIndex].position;
            double moonDist = moonRelPosition.magnitude();
            std::cout << "Satellite distance from Earth: " << satDist << " meters" << std::endl;
            std::cout << "Moon distance from Earth: " << moonDist << " meters" << std::endl;
            
            Vector3 satelliteRelVelocity = bodies[satelliteIndex].velocity - bodies[earthIndex].velocity;
            Vector3 moonRelVelocity = bodies[moonIndex].velocity - bodies[earthIndex].velocity;
            std::cout << "Satellite speed relative to Earth: " << satelliteRelVelocity.magnitude() << " m/s" << std::endl;
            std::cout << "Moon speed relative to Earth: " << moonRelVelocity.magnitude() << " m/s" << std::endl;
        }
    }
    std::cout << "Minimum satellite distance from Earth: " << minSatDist << " meters" << std::endl;
    std::cout << "Maximum satellite distance from Earth: " << maxSatDist << " meters" << std::endl;
    */
}