{
	"includes": [
		"../../common.gypi"
	],
	"variables": {
		"shared_geos%": "false",
		"deps_dir": "../",
		"endianness": "<!(python -c \"import sys;print(sys.byteorder.upper())\")",
	},
	"target_defaults": {
		"include_dirs": [
			"./gdal/alg",
			"./gdal/gcore",
			"./gdal/port",
			"./gdal/bridge",
			"./gdal/frmts",
			"./gdal/frmts/gtiff",
			"./gdal/ogr",
			"./gdal/ogr/ogrsf_frmts",
			"./gdal/ogr/ogrsf_frmts/mem",
			"./gdal/ogr/ogrsf_frmts/generic",
			"./gdal/ogr/ogrsf_frmts/shape",
			"./gdal/ogr/ogrsf_frmts/avc",
			"./gdal/ogr/ogrsf_frmts/geojson/libjson"
		],
		"defines": [
			"_LARGEFILE_SOURCE",
			"_FILE_OFFSET_BITS=64",
			"PAM_ENABLED=1",
			"OGR_ENABLED=1",
			"HAVE_EXPAT=1",
			"HAVE_LIBPROJ=1",
			"HAVE_GEOS=1",
			"PROJ_STATIC=1",
			"CPU_<(endianness)_ENDIAN=1"
		],
		"dependencies": [
			"<(deps_dir)/libexpat/libexpat.gyp:libexpat",
			"<(deps_dir)/libproj/libproj.gyp:libproj"
		],
		"cflags_cc!": ["-fno-rtti", "-fno-exceptions"],
		"cflags!": ["-fno-rtti", "-fno-exceptions"],
		"conditions": [
			["OS == 'win'", {
				"include_dirs": ["./arch/win"],
				"VCCLCompilerTool": {
					"RuntimeLibrary": "0", # 0:/MT, 1:/MTd, 2:/MD, 3:/MDd
					"DebugInformationFormat": "0"
				},
				"VCLinkerTool": {
					"GenerateDebugInformation": "false",
				},
			}],
			["OS == 'freebsd'", {
				"include_dirs": ["./arch/bsd"]
			}],
			["OS != 'freebsd' and OS != 'win'", {
				"include_dirs": ["./arch/unix"]
			}],
			["OS == 'mac'", {
				"xcode_settings": {
					"GCC_ENABLE_CPP_RTTI": "YES",
					"GCC_ENABLE_CPP_EXCEPTIONS": "YES"
				}
			}],
			["shared_geos == 'false'", {
				"dependencies": [
					"<(deps_dir)/libgeos/libgeos.gyp:libgeos"
				]
			}, {
				"libraries": ["<!@(geos-config --libs)"],
				"cflags_cc": ["<!@(geos-config --cflags)"],
				"xcode_settings": {
					"OTHER_CPLUSPLUSFLAGS":[
						"<!@(geos-config --cflags)"
					]
				}
			}]
		],
	}
}