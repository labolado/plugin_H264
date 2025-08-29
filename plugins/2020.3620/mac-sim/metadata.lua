local metadata =
{
    plugin =
    {
        format = 'staticLibrary',
        staticLibs = { 'plugin_h264' },
        frameworks = { 
            'VideoToolbox',
            'AudioToolbox', 
            'AVFoundation',
            'CoreMedia',
            'CoreVideo'
        },
        frameworksOptional = {},
    },
}

return metadata