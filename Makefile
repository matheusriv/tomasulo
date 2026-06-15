build/processor: processor.cpp $(SRC) | build
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

run-processor: build/processor
	$(LDLIBPATH) $<

run: build/loader
	@if [ -z "$(PROG)" ]; then \
		echo "Erro: Especifique o programa com PROG. Exemplo: make run PROG=programs/add_1.txt"; \
		exit 1; \
	fi
	./build/loader $(PROG)
	$(MAKE) run-processor