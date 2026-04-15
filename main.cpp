#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>


const double G = 6.6743e-11; // m^3 kg^-1 s^-2
const float c = 299792458.0;


class Vector3 {
    double x, y, z;

public:
    
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
        double mass;
        Vector3 position;
        Vector3 velocity;

        Body(double mass, const Vector3& position, const Vector3& velocity) 
            : mass(mass), position(position), velocity(velocity) {}

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

int main() {
    Body earth(5.972e24, Vector3(0, 0, 0), Vector3(0, 0, 0));
    Body satellite(1000, Vector3(0, 0, 7000000), Vector3(0, 7500, 0));
    // moon later

    // container
    std::vector<Body> bodies = {earth, satellite};
    // matching container for forces
    std::vector<Vector3> forces(bodies.size(), Vector3(0, 0, 0));


    double dt = 1; // timestep in seconds
    for (int step = 0; step < 10000; ++step) {
        // compute forces

        for (int iBody = 0; iBody < bodies.size(); ++iBody) {
            forces[iBody] = Vector3(); // reset force

            for (int jBody = 0; jBody < bodies.size(); ++jBody) {
                if (iBody == jBody) continue;
                forces[iBody] += bodies[iBody].gravitationalForceFrom(bodies[jBody]);
            }
        }

        // update bodies
        for (int i = 0; i < bodies.size(); ++i) {
            bodies[i].update(forces[i], dt);
        }

        if (step % 100 == 0) {
            std::cout << "Step " << step << ": " << std::endl;
            for (const auto& body : bodies) {
                body.position.print();
            }
            Vector3 relativePosition = bodies[1].position - bodies[0].position;
            std::cout << "Distance from Earth: " << relativePosition.magnitude() << " meters" << std::endl;
        }
    }

}