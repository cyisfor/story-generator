unittest {
	import html_when: process_when;
	import htmld: createDocument;
	import print: print;
	import std.process: environment;
	print(import("test-example.html"));
	function doit() {
		print("=============================================");
		auto doc = process_when(createDocument(import("test-example.html")));
		print(doc.root.html);
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
