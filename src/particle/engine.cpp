#include "engine.h"

ParticleEngine::ParticleEngine(int maxParticles, int spawnRate) : m_maxParticles(maxParticles), m_spawnRate(spawnRate) {
	particles.resize(maxParticles);
	g_particle_data.resize(4 * maxParticles);

	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	glGenVertexArrays(1, &vao);
}

void ParticleEngine::add(int count) {
	int cappedCount = std::min(count, m_maxParticles - spawnedParticles);

	spawnedParticles += cappedCount;

	for (int i = 0; i < cappedCount; i++) {
		int unused = unusedIdx();
		particles[unused].reseed();
	}
}

void ParticleEngine::update(float frametime, float emittingRate) {
	add((frametime > 16.0f ? 16.0f : frametime) * m_spawnRate * emittingRate);

	for (auto& p : particles)
		if (p.life > 0.0f) {
			p.life -= frametime;
			if (p.life > 0.0f) {
				p.position += p.velocity * frametime;
				p.position += glm::normalize(glm::vec3(0.0f, 0.0f, p.position.z) - p.position) * centerAttraction * frametime;
			}
			else
				spawnedParticles--;
		}

	int newestParticle = (oldestParticle - 1 + m_maxParticles) % m_maxParticles;
	while (particles[oldestParticle].life <= 0.0f && spawnedParticles > 0)
		oldestParticle = (oldestParticle + 1) % m_maxParticles;

	int copied = 0;
	for (int i = oldestParticle; i != newestParticle; i = (i + 1) % m_maxParticles)
		if (particles[i].life > 0.0f) {
			updateBuffer(copied, particles[i]);
			copied++;
		}
	if (particles[newestParticle].life > 0.0f) {
		updateBuffer(copied, particles[newestParticle]);
		copied++;
	}
	
	copyToGpu();
}

void ParticleEngine::updateBuffer(int bufferIdx, const Particle& p) {
	g_particle_data[4 * bufferIdx + 0] = p.position.x;
	g_particle_data[4  *bufferIdx + 1] = p.position.y;
	g_particle_data[4 * bufferIdx + 2] = p.position.z;
	g_particle_data[4 * bufferIdx + 3] = p.life;
}

int ParticleEngine::unusedIdx() {
	for (int i = unusedParticle; i < m_maxParticles; i++)
		if (particles[i].life <= 0.0f) {
			unusedParticle = i;
			return i;
		}

	for (int i = 0; i < unusedParticle; i++)
		if (particles[i].life <= 0.0f) {
			unusedParticle = i;
			return i;
		}
	return 0;
}

void ParticleEngine::copyToGpu() {
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, spawnedParticles * sizeof(GLfloat) * 4, &g_particle_data[0]);
}
