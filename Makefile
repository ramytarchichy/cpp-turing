OUT=out

$(OUT): Makefile *.cpp *.hpp
	g++ -g --std=c++20 -O2 -o $(OUT) *.cpp -lncurses -pthread

.PHONY: clean run

clean:
	-rm $(OUT)

run: $(OUT)
	./$(OUT)
