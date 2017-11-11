#include "mystring.h"
#include "db.h"
#include "mmapfile.h"

#include <sqlite3.h> 

#include <stdio.h>
#include <unistd.h> // STDIN_FILENO
#include <assert.h>
#include <ctype.h> // isspace

#include "db-private.h"


#define output_literal(lit) fwrite(LITLEN(lit), 1, stdout)
#define output_buf(s,l) fwrite(s, l, 1, stdout)

static
void output_sql(string sql) {
	int j = 0;
	for(j=0;j<sql.l;++j) {
		if(j == sql.l - 1 && sql.s[j] == ';') break;
		switch(sql.s[j]) {
		case '"':
			output_literal("\\\"");
		case '\n':
			output_literal("\\n\"\n\t\""); // logically break up the lines
		default:
			fputc(sql.s[j],stdout);
		}
	}
}

struct stmt {
	string name;
	string sql;
};

int main(int argc, char *argv[]) {
	size_t datalen = 0;
	const char* data = mmapfd(STDIN_FILENO, &datalen);

	struct stmt *stmts = NULL;
	size_t nstmts = 0;

	db_open(":memory:");

	const char* cur = data;
	size_t left = datalen;

	string name = {};
	if(isspace(cur[0])) {
		if(argc == 2) {
			name = ((string){.s = argv[1], .l = strlen(argv[1])});
		}
	} else {
		int i;
		for(i=0;cur[i]!='\n';++i) {
			if(i == left) {
				fprintf(stderr, "only one line for the name...\n");
				exit(3);
			}
		}
		name.s = cur;
		name.l = i;
		cur += i;
		left -= i;
	}

	while(left != 0) {
		while(isspace(cur[0])) {
			if(left == 0) break;
			++cur;
			--left;
		}
		if(!left) break;
		size_t i;
		for(i=0;;++i) {
			if(i == left) {
				fprintf(stderr,"no sql?\n");
				exit(1);
			}
			if(isspace(cur[i])) break;
		}
		++nstmts;
		stmts = realloc(stmts,sizeof(*stmts)*nstmts);
		stmts[nstmts-1].name = ((string){.s = cur, .l = i});
		left -= i;
		cur += i;

		while(isspace(*cur)) {
			++cur;
			if(left == 0) {
				fprintf(stderr,"no sql?\n");
				exit(2);
			}
			--left;
		}

		// now parse the SQL statement
		sqlite3_stmt* stmt = db_preparen(cur,left);
		sqlite3_finalize(stmt);
		
		stmts[nstmts-1].sql = ((string){.s = cur, .l = db_next-cur});

		assert(left >= db_next-cur-1);
		left -= db_next-cur + 1;
		cur = db_next + 1;
	}
	int i;
#include "o/template/statements2source.c.c"
}
