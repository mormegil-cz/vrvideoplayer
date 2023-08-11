pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        flatDir {
            dirs("${rootDir}/app/libs/cardboard-sdk/aar")
        }

        google()
        mavenCentral()
    }
}

rootProject.name = "VR Video Player"
include(":app")
