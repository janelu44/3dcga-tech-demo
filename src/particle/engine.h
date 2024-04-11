#pragma once

#include "particle.h"
#include <vector>
#include <glad/glad.h>
#include <framework/shader.h>

struct ParticleEngine {
	ParticleEngine(int maxParticles, int spawnRate);

	void add(int count);
	void update(float frametime, float emittingRate);
	void updateBuffer(int bufferIdx, const Particle& p);
	void copyToGpu();
	int unusedIdx();

	int m_maxParticles;
	int m_spawnRate;
	int spawnedParticles{ 0 };
	int unusedParticle{ 0 };
	int oldestParticle{ 0 };
	std::vector<Particle> particles;

	float centerAttraction{ 0.01f };

	GLfloat g_vertex_buffer_data[12] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	std::vector<GLfloat> g_particle_data;

	GLuint billboard_vertex_buffer;
	GLuint particles_position_buffer;
	GLuint vao;
};
