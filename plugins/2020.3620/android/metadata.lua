local metadata =
{
    plugin =
    {
        format = 'aar',
        manifest = 
        {
            permissions = {},
            usesPermissions =
            {
                "android.permission.READ_EXTERNAL_STORAGE",
                "android.permission.WRITE_EXTERNAL_STORAGE",
            },
            usesFeatures = {},
            applicationChildElements = 
            {
                [[
                <service android:name="com.plugin.h264.H264DecoderService"
                         android:exported="false" />
                ]]
            },
        },
    },
    coronaManifest = {
        dependencies = {
            -- OpenH264 and FDK-AAC native libraries included in AAR
        },
    },
}

return metadata