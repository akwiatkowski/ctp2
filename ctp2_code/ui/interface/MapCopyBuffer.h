#ifndef MAP_COPY_BUFFER_H__
#define MAP_COPY_BUFFER_H__

#include <vector>

class Cell;
class MapPoint;

struct CellInfo {
	uint8 m_terrain;
	uint32 m_env;
};

// Clipboard for map regions in the scenario editor: a w×h grid of cell
// snapshots stored flat (column-major to match the original m_cells[x][y]).
class MapCopyBuffer {
  private:
	sint32 m_width = 0;
	sint32 m_height = 0;
	std::vector<CellInfo> m_cells;

	CellInfo &At(sint32 x, sint32 y)
	{
		return m_cells[static_cast<size_t>(x) * static_cast<size_t>(m_height) + static_cast<size_t>(y)];
	}

  public:
	MapCopyBuffer();

	void SetSize(sint32 w, sint32 h);
	void Copy(MapPoint &pos, sint32 w, sint32 h);
	void Paste(MapPoint &pos);

	void Load(const MBCHAR *fileName);
	void Save(const MBCHAR *filename);

};

#endif
