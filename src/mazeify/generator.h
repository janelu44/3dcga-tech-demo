#pragma once

#include <iostream>
#include <vector>

struct MazeGenerator {
	MazeGenerator(int width, int height, int seed = 0xFFAAFFBB);

	std::vector<std::vector<int>> generate();
	void dig(int cx, int cy, std::vector<std::vector<int>>& grid);

	int m_width, m_height;
	
	// West North East South
	//    1     2    4     8
	int dx[4] = { -1, 0, 1, 0 };
	int dy[4] = { 0, -1, 0, 1 };

	int op[4] = {2, 3, 0, 1};
};
