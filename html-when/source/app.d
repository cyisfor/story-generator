import std.stdio;
import html_when: process_when;
import html.dom;
void main()
{
	char[0x1000] buf;
	auto d = createDocument(stdin.rawRead(buf));
	auto root = d.root;
	process_when(root);
	stdout.write(root.html);
}
