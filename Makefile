include coolmake/main.mk
coolmake/main.mk: | coolmake
	$(MAKE)
coolmake: htmlish/coolmake
	ln -rs $< $@

htmlish/coolmake: | htmlish
	$(MAKE) -C $|

htmlish:
	sh setup.sh
