# Texture sub-module unit tests (seconds): make run
CFLAGS = -O2 -g -Wall -Wextra
# SAN: extra compiler flags, O: the output folder (with a trailing slash) of a sanitizer or coverage build (make asan and make coverage below set both); both empty otherwise
SAN ?=
O ?=
CLANG = clang $(SAN)
$(O)t_tex: t_tex.c ../nvn_deswizzle.c ../nvn_deswizzle.h ../../nvn/nvn_tex.c ../../nvn/nvn_tex.h
	$(CLANG) $(CFLAGS) -o $@ t_tex.c ../nvn_deswizzle.c ../../nvn/nvn_tex.c -lm
$(O)t_fmt_mtl: t_fmt_mtl.m ../nvn_fmt_mtl.c ../nvn_fmt_mtl.h
	$(CLANG) $(CFLAGS) -fobjc-arc -o $@ t_fmt_mtl.m ../nvn_fmt_mtl.c -framework Metal -framework Foundation
ASTCENC=../../../third_party/astcenc
ASTCENC_SRC=$(sort $(wildcard $(ASTCENC)/*.cpp))
ASTCENC_OBJ=$(patsubst $(ASTCENC)/%.cpp,astcenc/%.o,$(ASTCENC_SRC))
ASTCENC_VEC=-DASTCENC_SSE=20 -DASTCENC_AVX=0 -DASTCENC_NEON=0 -DASTCENC_SVE=0 -DASTCENC_POPCNT=0 -DASTCENC_F16C=0
ASTCENC_CXXFLAGS=-O2 -g -std=c++11 $(ASTCENC_VEC) -I$(ASTCENC)
astcenc/%.o: $(ASTCENC)/%.cpp
	@mkdir -p astcenc
	g++ $(ASTCENC_CXXFLAGS) -c $< -o $@
libastcenc.a: $(ASTCENC_OBJ)
	ar rcs $@ $(ASTCENC_OBJ)
$(O)t_astcenc: t_astcenc.c ../nvn_astcenc.cpp libastcenc.a
	g++ -O2 -g -std=c++11 $(ASTCENC_VEC) -I.. -I$(ASTCENC) -o $@ t_astcenc.c ../nvn_astcenc.cpp libastcenc.a
run: t_tex t_fmt_mtl t_astcenc
	./t_fmt_mtl && ./t_tex
	./t_astcenc
.PHONY: run
# ASan and UBSan (../../../sanitizers/sanitizers.mk): the CPU test t_tex, built into asan/ (t_fmt_mtl creates Metal textures and stays local)
include ../../../sanitizers/sanitizers.mk
.PHONY: asan
asan:
	@$(call san_fresh,asan,$(ASAN_FLAGS))
	$(MAKE) O=asan/ SAN="$(ASAN_FLAGS)" asan/t_tex
	$(ASAN_ENV) ./asan/t_tex
# Line coverage (../../../sanitizers/coverage.mk): t_tex built into cov/ with COV_FLAGS and run; writes cov/coverage.lcov
include ../../../sanitizers/coverage.mk
.PHONY: coverage
coverage:
	@$(call san_fresh,cov,$(COV_FLAGS))
	rm -rf cov/prof
	$(MAKE) O=cov/ SAN="$(COV_FLAGS)" cov/t_tex
	-LLVM_PROFILE_FILE="$(call cov_profile,cov)" ./cov/t_tex
	@$(call cov_export,cov)
