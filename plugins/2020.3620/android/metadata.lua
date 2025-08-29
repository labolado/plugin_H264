local metadata =
{
    plugin =
    {
        format = 'jar',
        manifest = 
        {
            permissions = {},
            usesPermissions =
            {
                "android.permission.WRITE_EXTERNAL_STORAGE",
                "android.permission.READ_EXTERNAL_STORAGE",
            },
            usesFeatures = {},
            applicationChildElements = 
            {
            },
        },
    },
    coronaManifest = {
        dependencies = {
            -- Add any required dependencies here
        },
    },
}

return metadata