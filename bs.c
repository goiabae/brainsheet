#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

typedef struct Dimension {
	long height;
	long width;
} Dimension;

typedef struct Selection {
	long x1;
	long y1;
	long x2;
	long y2;
} Selection;

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
	RUN_UP,
	RUN_LEFT,
	RUN_DOWN,
	RUN_RIGHT,
	SELECT,
	PRINT,
	HALT,
} Operation;

typedef enum Direction {
	UP,
	LEFT,
	DOWN,
	RIGHT,
	STILL,
} Direction;

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
	long cx;
	long cy;
	bool is_halt;
	SelectionChain sels;
	Direction run;
	Cell* cells;
} Table;

long matrix_at(long w, long x, long y) {
	return w * y + x;
}

Table new_table(long h, long w) {
	Table t = {
		.h = h,
		.w = w,
		.cx = 0,
		.cy = 0,
		.is_halt = false,
		.sels = (SelectionChain) {
			.is_selecting = false,
			.list = NULL,
		},
		.run = STILL,
		.cells = malloc(sizeof(Cell) * h * w),
	};

	assert(t.cells);

	for (int i = 0; i < h; i ++) {
		for (int j = 0; j < w; j ++) {
			t.cells[matrix_at(t.w, i, j)].type = NIL;
		}
	}

	return t;
}

void delete_table(Table t) {
	// free selection list
	while (t.sels.list) {
		SelectionNode* tail = t.sels.list->next;
		free(t.sels.list);
		t.sels.list = tail;
	}

	free(t.cells);
}

void begin_selection(Table* t) {
	long x;
	long y;
	switch (t->run) {
	case UP:
		x = t->cx;
		y = t->cy - 1;
		break;
	case LEFT:
		x = t->cx - 1;
		y = t->cy;
		break;
	case DOWN:
		x = t->cx;
		y = t->cy + 1;
		break;
	case RIGHT:
		x = t->cx + 1;
		y = t->cy;
		break;
	case STILL:
		assert(t->run != STILL);
		break;
	}
	t->sels.is_selecting = true;
	t->sels.head.sel.x1 = x;
	t->sels.head.sel.y1 = y;
}

void push_selection(SelectionChain* sels) {
	sels->head.next = sels->list;
	sels->list = malloc(sizeof(SelectionNode));
	sels->list->next  = sels->head.next;
	sels->list->sel = sels->head.sel;
	return;
}

void pop_selection(SelectionChain* sels) {
	SelectionNode* next = sels->list->next;
	free(sels->list);
	sels->list = next;
}

Dimension selection_dimensions(Selection s) {
	return (Dimension) {
		.height = (s.y2 > s.y1) ? (s.y2 - s.y1) : (s.y1 - s.y2),
		.width = (s.x2 > s.x1) ? (s.x2 - s.x1) : (s.x1 - s.x2),
	};
}

void end_selection(Table* t) {
	Direction dir = t->run;
	bool x_increasing = t->sels.head.sel.x1 <= t->cx;
	bool y_increasing = t->sels.head.sel.y1 <= t->cy;

	Selection s = t->sels.head.sel;
	s.x2 = t->cx;
	s.y2 = t->cy;

	Dimension d = selection_dimensions(s);

	if (d.height == 0) {
		(x_increasing) ? s.x2-- : s.x2++;
		goto ret;
	}

	if (d.width == 0) {
		(y_increasing) ? s.y2-- : s.y2++;
		goto ret;
	}

	if (dir == UP || dir == DOWN)
		(x_increasing) ? s.x2-- : s.y2++;
	else if (dir == LEFT || dir == RIGHT)
		(y_increasing) ? s.y2-- : s.y2++;

ret:
	t->sels.head.sel = s;
	push_selection(&t->sels);
	t->sels.is_selecting = false;
}

void handle_goto(Table* t) {
	long x, y;
	Selection last = t->sels.list->sel;
	Dimension last_dm = selection_dimensions(last);
	bool len2_vec =
		   last_dm.height == 0 && last_dm.width == 2
		|| last_dm.height == 2 && last_dm.width
	;

	if (len2_vec) {
		t->cx = t->cells[matrix_at(t->w, last.x1, last.y1)].number;
		t->cy = t->cells[matrix_at(t->w, last.x2, last.y2)].number;

		pop_selection(&t->sels);
	} else {
		printf("ERROR GOTO: Current selection is not a 1 dimensional vector of length 2\n");
	}
}

void cell_print(Cell c) {
	if (c.type == OP) {
		printf("ERR PRINT: <OP> cannot be printed\n");
	} else if (c.type == NUM) {
		printf("%d", c.number);
	} else if (c.type == CHAR) {
		printf("%c", c.character);
	}
}

void print_selection(Table* t) {
	Selection last = t->sels.list->sel;
	Dimension last_dm = selection_dimensions(last);
	bool x_increasing = last.x1 <= last.x2;
	bool y_increasing = last.x1 <= last.y2;

	if (! x_increasing) {
		long tmp = last.x1;
	  last.x1 = last.x2;
		last.x2 = tmp;
	}

	if (! y_increasing) {
		long tmp = last.y1;
	  last.y1 = last.y2;
		last.y2 = tmp;
	}

	for (int i = last.y1; i <= last.y2; i++) {
		for (int j = last.x1; j <= last.x2; j++) {
			cell_print(t->cells[matrix_at(t->w, j, i)]);
		}
	}

	pop_selection(&t->sels);
	return;
}

void run_op(Table* t) {
	switch (t->cells[matrix_at(t->w, t->cx, t->cy)].op) {
	case GOTO:
		handle_goto(t);
		break;

	case RUN_UP:    t->run = UP;    break;
	case RUN_LEFT:  t->run = LEFT;  break;
	case RUN_DOWN:  t->run = DOWN;  break;
	case RUN_RIGHT: t->run = RIGHT; break;

	case SELECT:
		if (t->sels.is_selecting)
			end_selection(t);
		else
		  begin_selection(t);
		break;
	case PRINT:
		print_selection(t);
		break;
	case HALT:
		t->is_halt = true;
		break;
	}
	return;
}

void run_table(Table* t) {
	while (! t->is_halt) {
		if (t->cells[matrix_at(t->w, t->cx, t->cy)].type == OP) {
			run_op(t);
		}

		if      (t->run == UP)    t->cy--;
		else if (t->run == LEFT)  t->cx--;
		else if (t->run == DOWN)  t->cy++;
		else if (t->run == RIGHT) t->cx++;
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
	t->cells[matrix_at(t->w, x, y)] = (Cell) { .type = OP, .op = op };
}

void set_nil_cell(Table* t, long x, long y) {
	t->cells[matrix_at(t->w, x, y)] = (Cell) { NIL };
}

void set_number_cell(Table* t, long x, long y, Number num) {
	t->cells[matrix_at(t->w, x, y)] = (Cell) { .type = NUM, .number = num };
}

void set_character_cell(Table* t, long x, long y, Character car) {
	t->cells[matrix_at(t->w, x, y)] = (Cell) { .type = CHAR, .character = car };
}

void parse_character(Cell* cell, char buf[50]) {
	char c;

	if (buf[1] == '\\') {
		if (buf[2] == 'n')
			c = '\n';
		else if (buf[2] == 's')
			c = ' ';
	} else c = buf[1];

	cell->type = CHAR;
	cell->character = c;
}

void parse_number(Cell* cell, char buf[50]) {
	char* endptr;
	long n = strtol(buf, &endptr, 10);
	if (buf == endptr)
		printf("ERROR PARSE: Couldn't parse number\n");
	cell->type = NUM;
	cell->number = n;
}

void parse_op(Cell* cell, char buf[50]) {
	Operation op;
	if (!strcmp(buf, "select"))
		op = SELECT;
	else if (!strcmp(buf, "print"))
		op = PRINT;
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
	else
		printf("ERROR PARSE: Couldn't parse operation\n");
	cell->type = OP;
	cell->op = op;
}

void parse_table(Table* t, FILE* fd) {
	char buf[50];
	long x, y;
	char id_buf[50];

	while (!feof(fd)) {
		fgets(buf, 50, fd);
		sscanf(buf, "%ld %ld %s\n", &x, &y, id_buf);

		if (id_buf[0] == '\'')
			parse_character(&t->cells[matrix_at(t->w, x, y)], id_buf);
		else if ('0' >= id_buf[0] && id_buf[0] <= 0)
			parse_number(&t->cells[matrix_at(t->w, x, y)], id_buf);
		else
			parse_op(&t->cells[matrix_at(t->w, x, y)], id_buf);
	}
}

int main(int argc, char* argv[argc]) {
	if ((argc - 1) < 3) {
		usage();
		return 1;
	}

	int opt;
	while (-1 != (opt = getopt(argc, argv, "h"))) {
		switch (opt) {
		case 'h':
			usage();
			goto exit;
			break;
		}
	}

	char* endptr;

	long h = strtol(argv[1], &endptr, 10);
	if (argv[1] == endptr) return 1;

	long w = strtol(argv[2], &endptr, 10);
	if (argv[1] == endptr) return 1;

	Table t = new_table(h, w);

	FILE* fd = fopen(argv[3], "r");
	parse_table(&t, fd);
	fclose(fd);

	run_table(&t);
	delete_table(t);
exit:
	return 0;
}
