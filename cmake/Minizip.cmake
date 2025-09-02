include(FetchContent)

function(checkout_minizip)
	set(FETCHCONTENT_QUIET OFF)

	FetchContent_Declare(
	  minizip
	  EXCLUDE_FROM_ALL
	  GIT_REPOSITORY https://github.com/zlib-ng/minizip-ng
	  GIT_TAG        4.0.10
	)

	# we only need .zip handling (zlib) for now
	set(MZ_BZIP2 OFF)
	set(MZ_ZSTD OFF)
	set(MZ_LZMA OFF)
	set(MZ_OPENSSL OFF)
	set(MZ_LIBBSD OFF)
	set(MZ_PKCRYPT OFF)
	set(MZ_WZAES OFF)
	set(MZ_ICONV OFF)
	set(MZ_FORCE_FETCH_LIBS ON)
	set(MZ_FETCH_LIBS ON)
	set(BUILD_SHARED_LIBS OFF)
	
	FetchContent_MakeAvailable(minizip)
endfunction()
