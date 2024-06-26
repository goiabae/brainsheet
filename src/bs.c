#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Vec2 {
	long x;
	long y;
} Vec2;

typedef struct Selection {
	Vec2 beg;
	Vec2 end;
} Selection;

typedef struct Shape {
	size_t len;
	long* dimensions;
} Shape;

typedef struct SelectionNode {
	struct SelectionNode* next;
	Selection sel;
} SelectionNode;

typedef struct SelectionChain {
	bool is_selecting;
	SelectionNode* list;
	SelectionNode head;
} SelectionChain;

typedef int Number;
typedef char Character;

typedef enum Operation {
	GOTO,
	RUN,
	RUN_UP,
	RUN_LEFT,
	RUN_DOWN,
	RUN_RIGHT,
	SELECT,
	PRINT,
	HALT,
	ADD,
} Operation;

typedef enum Type {
	NIL,
	OP,
	NUM,
	CHAR,
} Type;

typedef struct Cell {
	Type type;
	union {
		Operation op;
		Number number;
		Character character;
	};
} Cell;

typedef struct Table {
	long h;
	long w;
	Vec2 cur;
	bool is_halt;
	SelectionChain sels;
	Vec2 run;
	Cell* cells;
} Table;

long table_at(long w, long x, long y) { return w * y + x; }

Table table_init(long h, long w) {
	Table t = {
		.h = h,
		.w = w,
		.cur = (Vec2) {0, 0},
		.is_halt = false,
		.sels =
			(SelectionChain) {
				.is_selecting = false,
				.list = NULL,
			},
		.run = (Vec2) {0, 0},
		.cells = malloc(sizeof(Cell) * h * w),
	};

	assert(t.cells);

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			t.cells[table_at(t.w, i, j)].type = NIL;
		}
	}

	return t;
}

void table_deinit(Table t) {
	// free selection list
	while (t.sels.list) {
		SelectionNode* tail = t.sels.list->next;
		free(t.sels.list);
		t.sels.list = tail;
	}

	free(t.cells);
}

void begin_selection(Table* t) {
	long x = t->cur.x + t->run.x;
	long y = t->cur.y + t->run.y;
	t->sels.is_selecting = true;
	t->sels.head.sel.beg = (Vec2) {x, y};
}

void push_selection(SelectionChain* sels, Selection sel) {
	sels->head.next = sels->list;
	sels->list = malloc(sizeof(SelectionNode));
	sels->list->next = sels->head.next;
	sels->list->sel = sel;
	return;
}

Selection pop_selection(SelectionChain* sels) {
	Selection last = sels->list->sel;
	SelectionNode* next = sels->list->next;
	free(sels->list);
	sels->list = next;
	return last;
}

void end_selection(Table* t) {
	Selection s = t->sels.head.sel;
	s.end = (Vec2) {
		t->cur.x - t->run.x,
		t->cur.y - t->run.y,
	};

	// normalize selection with beg always smaller than end

	// . . E
	// . . .
	// B . .
	if (s.end.x >= s.beg.x && s.end.y < s.beg.y) {
		long tmp = s.end.y;
		s.end.y = s.beg.y;
		s.beg.y = tmp;

		// E . .
		// . . .
		// . . B
	} else if (s.end.x < s.beg.x && s.end.y < s.beg.y) {
		long tmp = s.end.y;
		s.end.y = s.beg.y;
		s.beg.y = tmp;

		tmp = s.end.x;
		s.end.x = s.beg.x;
		s.beg.x = tmp;

		// . . B
		// . . .
		// E . .
	} else if (s.end.x < s.beg.x && s.end.y >= s.beg.y) {
		long tmp = s.end.x;
		s.end.x = s.beg.x;
		s.beg.x = tmp;
	}

	t->sels.head.sel = s;
	push_selection(&t->sels, s);
	t->sels.is_selecting = false;
}

Shape selection_shape(Selection sel) {
	Shape shape;
	if (sel.beg.y == sel.end.y && sel.beg.x == sel.end.x) {
		shape.len = 0;
		shape.dimensions = NULL;
	} else if (sel.beg.y == sel.end.y) {
		shape.len = 1;
		shape.dimensions = malloc(sizeof(long) * 1);
		shape.dimensions[0] = sel.end.x - sel.beg.x + 1;
	} else {
		shape.len = 2;
		shape.dimensions = malloc(sizeof(long) * 2);
		shape.dimensions[0] = sel.end.y - sel.beg.y + 1;
		shape.dimensions[1] = sel.end.x - sel.beg.x + 1;
	}
	return shape;
}

void handle_goto(Table* t) {
	Selection last = pop_selection(&t->sels);
	Shape shape = selection_shape(last);

	assert(shape.dimensions[1] == 2 && "ERROR GOTO: Current selection is not a 1 dimensional vector of length 2\n");
	t->cur.x = t->cells[table_at(t->w, last.beg.x, last.beg.y)].number;
	t->cur.y = t->cells[table_at(t->w, last.beg.x + 1, last.beg.y)].number;

	free(shape.dimensions);
}

void handle_run(Table* t) {
	Selection last = pop_selection(&t->sels);
	Shape shape = selection_shape(last);

	assert(
		shape.dimensions[1] == 2 && shape.dimensions[0] == 1
		&& "ERROR GOTO: Current selection is not a vector of shape 2\n"
	);
	t->run.x = t->cells[table_at(t->w, last.beg.x, last.beg.y)].number;
	t->run.y = t->cells[table_at(t->w, last.beg.x + 1, last.beg.y)].number;

	free(shape.dimensions);
}

Selection selection_at(Selection sel, size_t i) {
	if (sel.beg.y == sel.end.y)
		return (Selection) {
			.beg.x = sel.beg.x + i,
			.beg.y = sel.beg.y,
			.end.x = sel.beg.x + i,
			.end.y = sel.end.y,
		};
	else
		return (Selection) {
			.beg.x = sel.beg.x,
			.beg.y = sel.beg.y + i,
			.end.x = sel.end.x,
			.end.y = sel.beg.y + i};
}

void replicate(
	void (*func)(Table*, Selection*, size_t), Table* t, Selection* args,
	size_t arg_count, size_t* ranks
) {
	size_t outer_shape = 0;
	size_t max_rank = 0;
	bool should_apply = true;

	for (size_t i = 0; i < arg_count; i++) {
		Shape shape = selection_shape(args[i]);
		if (shape.len != ranks[i]) should_apply = false;
		if (shape.len > max_rank) {
			max_rank = shape.len;
			outer_shape = shape.dimensions[0];
		}
		free(shape.dimensions); // shape_deinit
	}

	if (should_apply)
		func(t, args, arg_count);
	else {
		Selection* rep_args = malloc(sizeof(Selection) * arg_count);

		for (size_t i = 0; i < outer_shape; i++) {
			for (size_t j = 0; j < arg_count; j++) {
				Shape shape = selection_shape(args[j]);
				if (shape.len > ranks[j] && shape.len == max_rank)
					rep_args[j] = selection_at(args[j], i);
				else {
					assert(shape.len == ranks[j]);
					rep_args[j] = args[j];
				}
				free(shape.dimensions); // shape_deinit
			}
			replicate(func, t, rep_args, arg_count, ranks);
		}

		free(rep_args);
	}
}

void add(Table* t, Selection* args, size_t arg_count) {
	assert(arg_count == 3);
	Cell* z = &t->cells[table_at(t->w, args[0].beg.x, args[0].beg.y)];
	Cell y = t->cells[table_at(t->w, args[1].beg.x, args[1].beg.y)];
	Cell x = t->cells[table_at(t->w, args[2].beg.x, args[2].beg.y)];
	z->number = x.number + y.number;
}

// z y x -- z
// x + y = z
void handle_add(Table* t) {
	Selection args[3];
	args[0] = pop_selection(&t->sels); // z rank 0
	args[1] = pop_selection(&t->sels); // y rank 0
	args[2] = pop_selection(&t->sels); // x rank 0

	replicate(add, t, args, 3, (size_t[3]) {0, 0, 0});
	push_selection(&t->sels, args[0]); // push back z
}

char* op_to_str(Operation op) {
	switch (op) {
		case GOTO: return "goto";
		case RUN_UP: return "run_up";
		case RUN_LEFT: return "run_left";
		case RUN_DOWN: return "run_down";
		case RUN_RIGHT: return "run_right";
		case RUN: return "run";
		case SELECT: return "select";
		case PRINT: return "print";
		case HALT: return "halt";
		case ADD: return "add";
	}
	assert(false);
}

void cell_print(Cell c) {
	if (c.type == OP) {
		printf("ERR PRINT: Operation <%s> cannot be printed\n", op_to_str(c.op));
	} else if (c.type == NUM) {
		printf("%d", c.number);
	} else if (c.type == CHAR) {
		printf("%c", c.character);
	}
}

void print_selection(Table* t) {
	Selection last = t->sels.list->sel;
	bool x_increasing = last.beg.x <= last.end.x;
	bool y_increasing = last.beg.x <= last.end.y;

	if (!x_increasing) {
		long tmp = last.beg.x;
		last.beg.x = last.end.x;
		last.end.x = tmp;
	}

	if (!y_increasing) {
		long tmp = last.beg.y;
		last.beg.y = last.end.y;
		last.end.y = tmp;
	}

	for (int i = last.beg.y; i <= last.end.y; i++) {
		for (int j = last.beg.x; j <= last.end.x; j++) {
			cell_print(t->cells[table_at(t->w, j, i)]);
		}
	}

	pop_selection(&t->sels);
	return;
}

void run_op(Table* t) {
	switch (t->cells[table_at(t->w, t->cur.x, t->cur.y)].op) {
		case GOTO: handle_goto(t); break;

		case RUN: handle_run(t); break;

		case RUN_UP: t->run = (Vec2) {0, -1}; break;
		case RUN_LEFT: t->run = (Vec2) {-1, 0}; break;
		case RUN_DOWN: t->run = (Vec2) {0, 1}; break;
		case RUN_RIGHT: t->run = (Vec2) {1, 0}; break;

		case SELECT:
			if (t->sels.is_selecting)
				end_selection(t);
			else
				begin_selection(t);
			break;
		case PRINT: print_selection(t); break;
		case HALT: t->is_halt = true; break;
		case ADD: handle_add(t); break;
	}
	return;
}

void table_run(Table* t) {
	while (!t->is_halt) {
		long i = table_at(t->w, t->cur.x, t->cur.y);
		if (t->cells[i].type == OP) {
			run_op(t);
			if (t->cells[i].op == GOTO) continue;
		}

		t->cur.x += t->run.x;
		t->cur.y += t->run.y;
	}
}

void usage() {
	char const help[] =
		"Usage:\n"
		"\tbs [OPTION...] HEIGHT WIDTH FILE\n"
		"Options:\n"
		"\t-h    show this help message\n";
	printf("%s", help);
}

void set_op_cell(Table* t, long x, long y, Operation op) {
	t->cells[table_at(t->w, x, y)] = (Cell) {.type = OP, .op = op};
}

void set_nil_cell(Table* t, long x, long y) {
	t->cells[table_at(t->w, x, y)] = (Cell) {NIL};
}

void set_number_cell(Table* t, long x, long y, Number num) {
	t->cells[table_at(t->w, x, y)] = (Cell) {.type = NUM, .number = num};
}

void set_character_cell(Table* t, long x, long y, Character car) {
	t->cells[table_at(t->w, x, y)] = (Cell) {.type = CHAR, .character = car};
}

void parse_character(Cell* cell, char buf[50]) {
	char c;

	if (buf[1] == '\\') {
		if (buf[2] == 'n')
			c = '\n';
		else if (buf[2] == 's')
			c = ' ';
		else {
			printf("ERROR PARSE: Unknown escape sequence \"\\%c\"\n", buf[2]);
			c = '\0';
		}
	} else
		c = buf[1];

	cell->type = CHAR;
	cell->character = c;
}

void parse_number(Cell* cell, char buf[50]) {
	char* endptr;
	long n = strtol(buf, &endptr, 10);
	if (buf == endptr) printf("ERROR PARSE: Couldn't parse number\n");
	cell->type = NUM;
	cell->number = n;
}

void parse_op(Cell* cell, char buf[50]) {
	Operation op;
	if (!strcmp(buf, "select"))
		op = SELECT;
	else if (!strcmp(buf, "print"))
		op = PRINT;
	else if (!strcmp(buf, "run"))
		op = RUN;
	else if (!strcmp(buf, "up"))
		op = RUN_UP;
	else if (!strcmp(buf, "right"))
		op = RUN_RIGHT;
	else if (!strcmp(buf, "down"))
		op = RUN_DOWN;
	else if (!strcmp(buf, "left"))
		op = RUN_LEFT;
	else if (!strcmp(buf, "goto"))
		op = GOTO;
	else if (!strcmp(buf, "halt"))
		op = HALT;
	else if (!strcmp(buf, "add"))
		op = ADD;
	else {
		printf("ERROR PARSE: Couldn't parse operation \"%s\"\n", buf);
		assert(false);
	}
	cell->type = OP;
	cell->op = op;
}

bool table_from_fd(Table* t, FILE* fd) {
	char buf[50];
	long x, y;
	char id_buf[50];

	while (!feof(fd)) {
		char* res = fgets(buf, 50, fd);
		if (!res) return true;
		sscanf(buf, "%ld %ld %s\n", &x, &y, id_buf);

		if (x > (t->w - 1)) {
			printf("ERROR PARSE: index (%ld, %ld) exceeds table height\n", x, y);
			return false;
		}
		if (y > (t->h - 1)) {
			printf("ERROR PARSE: index (%ld, %ld) exceeds table height\n", x, y);
			return false;
		}

		if (id_buf[0] == '\'')
			parse_character(&t->cells[table_at(t->w, x, y)], id_buf);
		else if ('0' <= id_buf[0] && id_buf[0] <= '9')
			parse_number(&t->cells[table_at(t->w, x, y)], id_buf);
		else
			parse_op(&t->cells[table_at(t->w, x, y)], id_buf);
	}
	return true;
}

long read_num(char* str, int* success) {
	char* endptr;
	long res = strtol(str, &endptr, 10);
	if (str == endptr) {
		printf("ERROR PARSE: couldn't read number from string \"%s\"", str);
		*success = 1;
	}
	return res;
}

/*** BEGIN OPTIONS ***/
typedef struct {
	bool help;
} Options;

Options parse_opts(int argc, char** argv) {
	Options res = (Options) {
		.help = false,
	};
	int opt;

	if ((argc - 1) < 3) res.help = true;

	while (-1 != (opt = getopt(argc, argv, "h"))) {
		switch (opt) {
			case 'h': res.help = true; break;
		}
	}
	return res;
}
/*** END OPTIONS ***/

int main(int argc, char* argv[argc]) {
	int code = 0;
	int* exit = &code;
	Options opts = parse_opts(argc, argv);
	if (opts.help) {
		usage();
		goto exit;
	}

	long h = read_num(argv[1], exit);
	if (*exit != 0) {
		printf("ERROR PARSE: couldn't read number from string \"%s\"", argv[1]);
		goto exit;
	}

	long w = read_num(argv[2], exit);
	if (*exit != 0) {
		printf("ERROR PARSE: couldn't read number from string \"%s\"", argv[2]);
		goto exit;
	}

	Table t = table_init(h, w);

	FILE* fd = fopen(argv[3], "r");
	if (!fd) {
		printf("ERROR IO: Could not open file \"%s\"\n", argv[3]);
		*exit = 1;
		goto deinit_table;
	}

	if (!table_from_fd(&t, fd)) {
		*exit = 1;
		goto close_fd;
	}

	table_run(&t);

	// defer
close_fd:
	fclose(fd);
deinit_table:
	table_deinit(t);
exit:
	return *exit;
}
