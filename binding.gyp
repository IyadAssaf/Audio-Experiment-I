{
	"targets": [{
		"target_name": "tones",
		"sources": ["src/main.cpp"]
	}],
	"xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
    },
    "link_settings": {
        "libraries": [
            "-framework",
            "CoreAudio",
			"-framework",
			"AudioToolbox",
			"-framework",
			"AudioUnit",
			"-framework",
			"CoreFoundation"
        ],
        "configurations": {
            "Debug": {
                "xcode_settings": {
                    "OTHER_LDFLAGS": [
                        "-Lexternal/thelibrary/lib/debug"
                    ]
                }
            },
            "Release": {
                "xcode_settings": {
                    "OTHER_LDFLAGS": [
                        "-Lexternal/thelibrary/lib/release"
                    ]
                }
            }
        }
    }
}
