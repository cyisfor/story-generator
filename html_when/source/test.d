unittest {
	import html_when: process_when;
	import html: createDocument;
	import std.stdio: writeln;
	import std.process: environment;
	writeln(import("test-example.html"));
	void doit() {
		writeln("=============================================");
		auto doc = createDocument(import("test-example.html"));
		doc.process_when();
		writeln(doc.root.html);
	}
	doit();
	environment["censored"] = "1";
	doit();
	environment["something"] = "serious";
	doit();
	environment["something"] = "silly";
	doit();
	environment["foo"] = "bar";
	doit();
}
