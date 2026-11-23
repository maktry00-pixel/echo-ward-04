extends RefCounted
## The logical map is the single source of truth. X is column, Y is row.
## Rendered positions use X/Z; all movement stays on this grid.

const CELL_SIZE: float = 1.25
const START: Vector2i = Vector2i(1, 1)
const ROWS: Array[String] = [
	"###############",
	"#P....#.......#",
	"#.###.#.###.#.#",
	"#...#...#...#.#",
	"###.#####.###.#",
	"#...#.....#...#",
	"#.###.###.#.#.#",
	"#.....#...#.#.#",
	"#.###.#.###.#.#",
	"#.#...#.....#.#",
	"#.#.#######.#.#",
	"#...#.....#...#",
	"#.###.###.###.#",
	"#.............#",
	"###############",
]


func width() -> int:
	return ROWS[0].length()


func height() -> int:
	return ROWS.size()


func is_walkable(cell: Vector2i) -> bool:
	if cell.x < 0 or cell.y < 0 or cell.x >= width() or cell.y >= height():
		return false
	return ROWS[cell.y][cell.x] != "#"


func neighbors(cell: Vector2i) -> Array[Vector2i]:
	var result: Array[Vector2i] = []
	for step: Vector2i in [Vector2i.UP, Vector2i.RIGHT, Vector2i.DOWN, Vector2i.LEFT]:
		var next_cell: Vector2i = cell + step
		if is_walkable(next_cell):
			result.append(next_cell)
	return result


func world_position(cell: Vector2i, elevation: float = 0.53) -> Vector3:
	return Vector3(cell.x * CELL_SIZE, elevation, cell.y * CELL_SIZE)

