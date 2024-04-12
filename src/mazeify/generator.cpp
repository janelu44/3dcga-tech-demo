#include "generator.h"

MazeGenerator::MazeGenerator(int width, int height, int seed) : m_width(width), m_height(height) {
	srand(seed);
}

std::vector<std::vector<int>> MazeGenerator::generate() {
	std::vector<std::vector<int>> grid(m_width, std::vector<int>(m_height, 0));
	std::vector<std::vector<int>> wall(2 * m_width + 1, std::vector<int>(2 * m_height + 1, 1));

	dig(0, 0, grid);

	for (int x = 0; x < m_width; x++)
		for (int y = 0; y < m_height; y++) {
			int lx = x * 2 + 1;
			int ly = y * 2 + 1;
			wall[lx][ly] = 0;
			if (grid[x][y] & 4)
				wall[lx + 1][ly] = 0;
			if (grid[x][y] & 8)
				wall[lx][ly + 1] = 0;
		}

	return wall;
}

void MazeGenerator::dig(int cx, int cy, std::vector<std::vector<int>>& grid) {
	bool dug[4] = { false, false, false, false };
	for (int i = 0; i < 4; i++) {
		int dr = rand() % (4 - i);
		int di = 0;
		for (int j = 0; j < 4; j++)
			if (!dug[j]) {
				if (dr > 0)
					dr--;
				else {
					di = j;
					dug[di] = true;
					break;
				}
			}

		int nx = cx + dx[di];
		int ny = cy + dy[di];

		if (nx < 0 || nx >= m_width || ny < 0 || ny >= m_height || grid[nx][ny] != 0)
			continue;

		grid[cx][cy] |= 1 << di;
		grid[nx][ny] |= 1 << op[di];

		dig(nx, ny, grid);
	}
}
