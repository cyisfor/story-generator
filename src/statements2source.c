<?C
#include "mmapfile.h"
#include "db.h"

int main(int argc, char *argv[])
{
#define output_literal(lit) write(1,lit,sizeof(lit)-1)
#define output_buf(s,l) write(1,s,l)

	size_t datalen = 0;
	const char* data = mmapfd(STDIN_FILENO, &datalen);

	return 0;
}
void statements_init() {
