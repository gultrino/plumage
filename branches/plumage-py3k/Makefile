PYTHON=/home/gpolo/python-dev/py3k/python
INSTALL_DIR=.
TCL_BASE_DIR=/usr/share/tcltk/tcl8.5
TK_BASE_DIR=/usr/share/tcltk/tk8.5
TCL_CONFIG_DIR=$(TCL_BASE_DIR)#/lib
TK_CONFIG_DIR=$(TK_BASE_DIR)#/lib

TCL_CONFIG=$(TCL_CONFIG_DIR)/tclConfig.sh
TK_CONFIG=$(TK_CONFIG_DIR)/tkConfig.sh

SRC_DIR=src
TEST_DIR=test

build: $(SRC_DIR)/plumage.c $(SRC_DIR)/utils.c $(TEST_DIR)/_utils_bridge.c
	TCL_CONFIG=$(TCL_CONFIG) \
	TK_CONFIG=$(TK_CONFIG) \
	$(PYTHON) setup.py install_lib --install-dir $(INSTALL_DIR)

# Warning!
clean:
	rm -rf build
	rm -rf plumage.so _utils_bridge.so
	find . -path "*.pyc" -delete
